/*
 * Copyright (C) 2007-2008 Advanced Micro Devices, Inc.
 * Author: Joerg Roedel <joerg.roedel@amd.com>
 *         Leo Duran <leo.duran@amd.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#include <linux/pci.h>
#include <linux/gfp.h>
#include <linux/bitops.h>
#include <linux/scatterlist.h>
#include <linux/iommu-helper.h>
#include <asm/proto.h>
#include <asm/gart.h>
#include <asm/amd_iommu_types.h>

#define CMD_SET_TYPE(cmd, t) ((cmd)->data[1] |= ((t) << 28))

#define to_pages(addr, size) \
	 (round_up(((addr) & ~PAGE_MASK) + (size), PAGE_SIZE) >> PAGE_SHIFT)

static DEFINE_RWLOCK(amd_iommu_devtable_lock);

struct command {
	u32 data[4];
};

static int dma_ops_unity_map(struct dma_ops_domain *dma_dom,
			     struct unity_map_entry *e);

static int __iommu_queue_command(struct amd_iommu *iommu, struct command *cmd)
{
	u32 tail, head;
	u8 *target;

	tail = readl(iommu->mmio_base + MMIO_CMD_TAIL_OFFSET);
	target = (iommu->cmd_buf + tail);
	memcpy_toio(target, cmd, sizeof(*cmd));
	tail = (tail + sizeof(*cmd)) % iommu->cmd_buf_size;
	head = readl(iommu->mmio_base + MMIO_CMD_HEAD_OFFSET);
	if (tail == head)
		return -ENOMEM;
	writel(tail, iommu->mmio_base + MMIO_CMD_TAIL_OFFSET);

	return 0;
}

static int iommu_queue_command(struct amd_iommu *iommu, struct command *cmd)
{
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&iommu->lock, flags);
	ret = __iommu_queue_command(iommu, cmd);
	spin_unlock_irqrestore(&iommu->lock, flags);

	return ret;
}

static int iommu_completion_wait(struct amd_iommu *iommu)
{
	int ret;
	struct command cmd;
	volatile u64 ready = 0;
	unsigned long ready_phys = virt_to_phys(&ready);

	memset(&cmd, 0, sizeof(cmd));
	cmd.data[0] = LOW_U32(ready_phys) | CMD_COMPL_WAIT_STORE_MASK;
	cmd.data[1] = HIGH_U32(ready_phys);
	cmd.data[2] = 1; /* value written to 'ready' */
	CMD_SET_TYPE(&cmd, CMD_COMPL_WAIT);

	iommu->need_sync = 0;

	ret = iommu_queue_command(iommu, &cmd);

	if (ret)
		return ret;

	while (!ready)
		cpu_relax();

	return 0;
}

static int iommu_queue_inv_dev_entry(struct amd_iommu *iommu, u16 devid)
{
	struct command cmd;

	BUG_ON(iommu == NULL);

	memset(&cmd, 0, sizeof(cmd));
	CMD_SET_TYPE(&cmd, CMD_INV_DEV_ENTRY);
	cmd.data[0] = devid;

	iommu->need_sync = 1;

	return iommu_queue_command(iommu, &cmd);
}

static int iommu_queue_inv_iommu_pages(struct amd_iommu *iommu,
		u64 address, u16 domid, int pde, int s)
{
	struct command cmd;

	memset(&cmd, 0, sizeof(cmd));
	address &= PAGE_MASK;
	CMD_SET_TYPE(&cmd, CMD_INV_IOMMU_PAGES);
	cmd.data[1] |= domid;
	cmd.data[2] = LOW_U32(address);
	cmd.data[3] = HIGH_U32(address);
	if (s)
		cmd.data[2] |= CMD_INV_IOMMU_PAGES_SIZE_MASK;
	if (pde)
		cmd.data[2] |= CMD_INV_IOMMU_PAGES_PDE_MASK;

	iommu->need_sync = 1;

	return iommu_queue_command(iommu, &cmd);
}

static int iommu_flush_pages(struct amd_iommu *iommu, u16 domid,
		u64 address, size_t size)
{
	int i;
	unsigned pages = to_pages(address, size);

	address &= PAGE_MASK;

	for (i = 0; i < pages; ++i) {
		iommu_queue_inv_iommu_pages(iommu, address, domid, 0, 0);
		address += PAGE_SIZE;
	}

	return 0;
}

static int iommu_map(struct protection_domain *dom,
		     unsigned long bus_addr,
		     unsigned long phys_addr,
		     int prot)
{
	u64 __pte, *pte, *page;

	bus_addr  = PAGE_ALIGN(bus_addr);
	phys_addr = PAGE_ALIGN(bus_addr);

	/* only support 512GB address spaces for now */
	if (bus_addr > IOMMU_MAP_SIZE_L3 || !(prot & IOMMU_PROT_MASK))
		return -EINVAL;

	pte = &dom->pt_root[IOMMU_PTE_L2_INDEX(bus_addr)];

	if (!IOMMU_PTE_PRESENT(*pte)) {
		page = (u64 *)get_zeroed_page(GFP_KERNEL);
		if (!page)
			return -ENOMEM;
		*pte = IOMMU_L2_PDE(virt_to_phys(page));
	}

	pte = IOMMU_PTE_PAGE(*pte);
	pte = &pte[IOMMU_PTE_L1_INDEX(bus_addr)];

	if (!IOMMU_PTE_PRESENT(*pte)) {
		page = (u64 *)get_zeroed_page(GFP_KERNEL);
		if (!page)
			return -ENOMEM;
		*pte = IOMMU_L1_PDE(virt_to_phys(page));
	}

	pte = IOMMU_PTE_PAGE(*pte);
	pte = &pte[IOMMU_PTE_L0_INDEX(bus_addr)];

	if (IOMMU_PTE_PRESENT(*pte))
		return -EBUSY;

	__pte = phys_addr | IOMMU_PTE_P;
	if (prot & IOMMU_PROT_IR)
		__pte |= IOMMU_PTE_IR;
	if (prot & IOMMU_PROT_IW)
		__pte |= IOMMU_PTE_IW;

	*pte = __pte;

	return 0;
}

static int iommu_for_unity_map(struct amd_iommu *iommu,
			       struct unity_map_entry *entry)
{
	u16 bdf, i;

	for (i = entry->devid_start; i <= entry->devid_end; ++i) {
		bdf = amd_iommu_alias_table[i];
		if (amd_iommu_rlookup_table[bdf] == iommu)
			return 1;
	}

	return 0;
}

static int iommu_init_unity_mappings(struct amd_iommu *iommu)
{
	struct unity_map_entry *entry;
	int ret;

	list_for_each_entry(entry, &amd_iommu_unity_map, list) {
		if (!iommu_for_unity_map(iommu, entry))
			continue;
		ret = dma_ops_unity_map(iommu->default_dom, entry);
		if (ret)
			return ret;
	}

	return 0;
}

static int dma_ops_unity_map(struct dma_ops_domain *dma_dom,
			     struct unity_map_entry *e)
{
	u64 addr;
	int ret;

	for (addr = e->address_start; addr < e->address_end;
	     addr += PAGE_SIZE) {
		ret = iommu_map(&dma_dom->domain, addr, addr, e->prot);
		if (ret)
			return ret;
		/*
		 * if unity mapping is in aperture range mark the page
		 * as allocated in the aperture
		 */
		if (addr < dma_dom->aperture_size)
			__set_bit(addr >> PAGE_SHIFT, dma_dom->bitmap);
	}

	return 0;
}

static int init_unity_mappings_for_device(struct dma_ops_domain *dma_dom,
					  u16 devid)
{
	struct unity_map_entry *e;
	int ret;

	list_for_each_entry(e, &amd_iommu_unity_map, list) {
		if (!(devid >= e->devid_start && devid <= e->devid_end))
			continue;
		ret = dma_ops_unity_map(dma_dom, e);
		if (ret)
			return ret;
	}

	return 0;
}

static unsigned long dma_mask_to_pages(unsigned long mask)
{
	return (mask >> PAGE_SHIFT) +
		(PAGE_ALIGN(mask & ~PAGE_MASK) >> PAGE_SHIFT);
}

static unsigned long dma_ops_alloc_addresses(struct device *dev,
					     struct dma_ops_domain *dom,
					     unsigned int pages)
{
	unsigned long limit = dma_mask_to_pages(*dev->dma_mask);
	unsigned long address;
	unsigned long size = dom->aperture_size >> PAGE_SHIFT;
	unsigned long boundary_size;

	boundary_size = ALIGN(dma_get_seg_boundary(dev) + 1,
			PAGE_SIZE) >> PAGE_SHIFT;
	limit = limit < size ? limit : size;

	if (dom->next_bit >= limit)
		dom->next_bit = 0;

	address = iommu_area_alloc(dom->bitmap, limit, dom->next_bit, pages,
			0 , boundary_size, 0);
	if (address == -1)
		address = iommu_area_alloc(dom->bitmap, limit, 0, pages,
				0, boundary_size, 0);

	if (likely(address != -1)) {
		set_bit_string(dom->bitmap, address, pages);
		dom->next_bit = address + pages;
		address <<= PAGE_SHIFT;
	} else
		address = bad_dma_address;

	WARN_ON((address + (PAGE_SIZE*pages)) > dom->aperture_size);

	return address;
}

static void dma_ops_free_addresses(struct dma_ops_domain *dom,
				   unsigned long address,
				   unsigned int pages)
{
	address >>= PAGE_SHIFT;
	iommu_area_free(dom->bitmap, address, pages);
}

static u16 domain_id_alloc(void)
{
	unsigned long flags;
	int id;

	write_lock_irqsave(&amd_iommu_devtable_lock, flags);
	id = find_first_zero_bit(amd_iommu_pd_alloc_bitmap, MAX_DOMAIN_ID);
	BUG_ON(id == 0);
	if (id > 0 && id < MAX_DOMAIN_ID)
		__set_bit(id, amd_iommu_pd_alloc_bitmap);
	else
		id = 0;
	write_unlock_irqrestore(&amd_iommu_devtable_lock, flags);

	return id;
}

static void dma_ops_reserve_addresses(struct dma_ops_domain *dom,
				      unsigned long start_page,
				      unsigned int pages)
{
	unsigned int last_page = dom->aperture_size >> PAGE_SHIFT;

	if (start_page + pages > last_page)
		pages = last_page - start_page;

	set_bit_string(dom->bitmap, start_page, pages);
}

static void dma_ops_free_pagetable(struct dma_ops_domain *dma_dom)
{
	int i, j;
	u64 *p1, *p2, *p3;

	p1 = dma_dom->domain.pt_root;

	if (!p1)
		return;

	for (i = 0; i < 512; ++i) {
		if (!IOMMU_PTE_PRESENT(p1[i]))
			continue;

		p2 = IOMMU_PTE_PAGE(p1[i]);
		for (j = 0; j < 512; ++i) {
			if (!IOMMU_PTE_PRESENT(p2[j]))
				continue;
			p3 = IOMMU_PTE_PAGE(p2[j]);
			free_page((unsigned long)p3);
		}

		free_page((unsigned long)p2);
	}

	free_page((unsigned long)p1);
}

static void dma_ops_domain_free(struct dma_ops_domain *dom)
{
	if (!dom)
		return;

	dma_ops_free_pagetable(dom);

	kfree(dom->pte_pages);

	kfree(dom->bitmap);

	kfree(dom);
}

static struct dma_ops_domain *dma_ops_domain_alloc(struct amd_iommu *iommu,
						   unsigned order)
{
	struct dma_ops_domain *dma_dom;
	unsigned i, num_pte_pages;
	u64 *l2_pde;
	u64 address;

	/*
	 * Currently the DMA aperture must be between 32 MB and 1GB in size
	 */
	if ((order < 25) || (order > 30))
		return NULL;

	dma_dom = kzalloc(sizeof(struct dma_ops_domain), GFP_KERNEL);
	if (!dma_dom)
		return NULL;

	spin_lock_init(&dma_dom->domain.lock);

	dma_dom->domain.id = domain_id_alloc();
	if (dma_dom->domain.id == 0)
		goto free_dma_dom;
	dma_dom->domain.mode = PAGE_MODE_3_LEVEL;
	dma_dom->domain.pt_root = (void *)get_zeroed_page(GFP_KERNEL);
	dma_dom->domain.priv = dma_dom;
	if (!dma_dom->domain.pt_root)
		goto free_dma_dom;
	dma_dom->aperture_size = (1ULL << order);
	dma_dom->bitmap = kzalloc(dma_dom->aperture_size / (PAGE_SIZE * 8),
				  GFP_KERNEL);
	if (!dma_dom->bitmap)
		goto free_dma_dom;
	/*
	 * mark the first page as allocated so we never return 0 as
	 * a valid dma-address. So we can use 0 as error value
	 */
	dma_dom->bitmap[0] = 1;
	dma_dom->next_bit = 0;

	if (iommu->exclusion_start &&
	    iommu->exclusion_start < dma_dom->aperture_size) {
		unsigned long startpage = iommu->exclusion_start >> PAGE_SHIFT;
		int pages = to_pages(iommu->exclusion_start,
				iommu->exclusion_length);
		dma_ops_reserve_addresses(dma_dom, startpage, pages);
	}

	num_pte_pages = dma_dom->aperture_size / (PAGE_SIZE * 512);
	dma_dom->pte_pages = kzalloc(num_pte_pages * sizeof(void *),
			GFP_KERNEL);
	if (!dma_dom->pte_pages)
		goto free_dma_dom;

	l2_pde = (u64 *)get_zeroed_page(GFP_KERNEL);
	if (l2_pde == NULL)
		goto free_dma_dom;

	dma_dom->domain.pt_root[0] = IOMMU_L2_PDE(virt_to_phys(l2_pde));

	for (i = 0; i < num_pte_pages; ++i) {
		dma_dom->pte_pages[i] = (u64 *)get_zeroed_page(GFP_KERNEL);
		if (!dma_dom->pte_pages[i])
			goto free_dma_dom;
		address = virt_to_phys(dma_dom->pte_pages[i]);
		l2_pde[i] = IOMMU_L1_PDE(address);
	}

	return dma_dom;

free_dma_dom:
	dma_ops_domain_free(dma_dom);

	return NULL;
}

static struct protection_domain *domain_for_device(u16 devid)
{
	struct protection_domain *dom;
	unsigned long flags;

	read_lock_irqsave(&amd_iommu_devtable_lock, flags);
	dom = amd_iommu_pd_table[devid];
	read_unlock_irqrestore(&amd_iommu_devtable_lock, flags);

	return dom;
}

static void set_device_domain(struct amd_iommu *iommu,
			      struct protection_domain *domain,
			      u16 devid)
{
	unsigned long flags;

	u64 pte_root = virt_to_phys(domain->pt_root);

	pte_root |= (domain->mode & 0x07) << 9;
	pte_root |= IOMMU_PTE_IR | IOMMU_PTE_IW | IOMMU_PTE_P | 2;

	write_lock_irqsave(&amd_iommu_devtable_lock, flags);
	amd_iommu_dev_table[devid].data[0] = pte_root;
	amd_iommu_dev_table[devid].data[1] = pte_root >> 32;
	amd_iommu_dev_table[devid].data[2] = domain->id;

	amd_iommu_pd_table[devid] = domain;
	write_unlock_irqrestore(&amd_iommu_devtable_lock, flags);

	iommu_queue_inv_dev_entry(iommu, devid);

	iommu->need_sync = 1;
}

static int get_device_resources(struct device *dev,
				struct amd_iommu **iommu,
				struct protection_domain **domain,
				u16 *bdf)
{
	struct dma_ops_domain *dma_dom;
	struct pci_dev *pcidev;
	u16 _bdf;

	BUG_ON(!dev || dev->bus != &pci_bus_type || !dev->dma_mask);

	pcidev = to_pci_dev(dev);
	_bdf = (pcidev->bus->number << 8) | pcidev->devfn;

	if (_bdf >= amd_iommu_last_bdf) {
		*iommu = NULL;
		*domain = NULL;
		*bdf = 0xffff;
		return 0;
	}

	*bdf = amd_iommu_alias_table[_bdf];

	*iommu = amd_iommu_rlookup_table[*bdf];
	if (*iommu == NULL)
		return 0;
	dma_dom = (*iommu)->default_dom;
	*domain = domain_for_device(*bdf);
	if (*domain == NULL) {
		*domain = &dma_dom->domain;
		set_device_domain(*iommu, *domain, *bdf);
		printk(KERN_INFO "AMD IOMMU: Using protection domain %d for "
				"device ", (*domain)->id);
		print_devid(_bdf, 1);
	}

	return 1;
}

static dma_addr_t dma_ops_domain_map(struct amd_iommu *iommu,
				     struct dma_ops_domain *dom,
				     unsigned long address,
				     phys_addr_t paddr,
				     int direction)
{
	u64 *pte, __pte;

	WARN_ON(address > dom->aperture_size);

	paddr &= PAGE_MASK;

	pte  = dom->pte_pages[IOMMU_PTE_L1_INDEX(address)];
	pte += IOMMU_PTE_L0_INDEX(address);

	__pte = paddr | IOMMU_PTE_P | IOMMU_PTE_FC;

	if (direction == DMA_TO_DEVICE)
		__pte |= IOMMU_PTE_IR;
	else if (direction == DMA_FROM_DEVICE)
		__pte |= IOMMU_PTE_IW;
	else if (direction == DMA_BIDIRECTIONAL)
		__pte |= IOMMU_PTE_IR | IOMMU_PTE_IW;

	WARN_ON(*pte);

	*pte = __pte;

	return (dma_addr_t)address;
}

static void dma_ops_domain_unmap(struct amd_iommu *iommu,
				 struct dma_ops_domain *dom,
				 unsigned long address)
{
	u64 *pte;

	if (address >= dom->aperture_size)
		return;

	WARN_ON(address & 0xfffULL || address > dom->aperture_size);

	pte  = dom->pte_pages[IOMMU_PTE_L1_INDEX(address)];
	pte += IOMMU_PTE_L0_INDEX(address);

	WARN_ON(!*pte);

	*pte = 0ULL;
}

static dma_addr_t __map_single(struct device *dev,
			       struct amd_iommu *iommu,
			       struct dma_ops_domain *dma_dom,
			       phys_addr_t paddr,
			       size_t size,
			       int dir)
{
	dma_addr_t offset = paddr & ~PAGE_MASK;
	dma_addr_t address, start;
	unsigned int pages;
	int i;

	pages = to_pages(paddr, size);
	paddr &= PAGE_MASK;

	address = dma_ops_alloc_addresses(dev, dma_dom, pages);
	if (unlikely(address == bad_dma_address))
		goto out;

	start = address;
	for (i = 0; i < pages; ++i) {
		dma_ops_domain_map(iommu, dma_dom, start, paddr, dir);
		paddr += PAGE_SIZE;
		start += PAGE_SIZE;
	}
	address += offset;

out:
	return address;
}

static void __unmap_single(struct amd_iommu *iommu,
			   struct dma_ops_domain *dma_dom,
			   dma_addr_t dma_addr,
			   size_t size,
			   int dir)
{
	dma_addr_t i, start;
	unsigned int pages;

	if ((dma_addr == 0) || (dma_addr + size > dma_dom->aperture_size))
		return;

	pages = to_pages(dma_addr, size);
	dma_addr &= PAGE_MASK;
	start = dma_addr;

	for (i = 0; i < pages; ++i) {
		dma_ops_domain_unmap(iommu, dma_dom, start);
		start += PAGE_SIZE;
	}

	dma_ops_free_addresses(dma_dom, dma_addr, pages);
}

