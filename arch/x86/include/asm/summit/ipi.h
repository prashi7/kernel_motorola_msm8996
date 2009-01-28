#ifndef __ASM_SUMMIT_IPI_H
#define __ASM_SUMMIT_IPI_H

void default_send_IPI_mask_sequence(const cpumask_t *mask, int vector);
void default_send_IPI_mask_allbutself(const cpumask_t *mask, int vector);

static inline void summit_send_IPI_mask(const cpumask_t *mask, int vector)
{
	default_send_IPI_mask_sequence(mask, vector);
}

static inline void summit_send_IPI_allbutself(int vector)
{
	cpumask_t mask = cpu_online_map;
	cpu_clear(smp_processor_id(), mask);

	if (!cpus_empty(mask))
		summit_send_IPI_mask(&mask, vector);
}

static inline void summit_send_IPI_all(int vector)
{
	summit_send_IPI_mask(&cpu_online_map, vector);
}

#endif /* __ASM_SUMMIT_IPI_H */
