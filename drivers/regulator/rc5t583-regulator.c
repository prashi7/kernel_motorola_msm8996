/*
 * Regulator driver for RICOH RC5T583 power management chip.
 *
 * Copyright (c) 2011-2012, NVIDIA CORPORATION.  All rights reserved.
 * Author: Laxman dewangan <ldewangan@nvidia.com>
 *
 * based on code
 *      Copyright (C) 2011 RICOH COMPANY,LTD
 *
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/gpio.h>
#include <linux/mfd/rc5t583.h>

struct rc5t583_regulator_info {
	int			deepsleep_id;

	/* Regulator register address.*/
	uint8_t			reg_disc_reg;
	uint8_t			disc_bit;
	uint8_t			deepsleep_reg;

	/* Chip constraints on regulator behavior */
	int			max_uV;

	/* Regulator specific turn-on delay  and voltage settling time*/
	int			enable_uv_per_us;
	int			change_uv_per_us;

	/* Used by regulator core */
	struct regulator_desc	desc;
};

struct rc5t583_regulator {
	struct rc5t583_regulator_info *reg_info;

	/* Devices */
	struct device		*dev;
	struct rc5t583		*mfd;
	struct regulator_dev	*rdev;
};

static int rc5t583_set_voltage(struct regulator_dev *rdev,
			       int min_uV, int max_uV, unsigned *selector)
{
	struct rc5t583_regulator *reg = rdev_get_drvdata(rdev);
	int sel, ret;

	if (min_uV < rdev->desc->min_uV)
		min_uV = rdev->desc->min_uV;

	sel = DIV_ROUND_UP(min_uV - rdev->desc->min_uV, rdev->desc->uV_step);

	if (sel >= rdev->desc->n_voltages) {
		dev_err(&rdev->dev, "Invalid selector 0x%02x\n", sel);
		return -EINVAL;
	}

	*selector = sel;

	ret = rc5t583_update(reg->mfd->dev, rdev->desc->vsel_reg, sel,
			     rdev->desc->vsel_mask);
	if (ret < 0)
		dev_err(&rdev->dev, "Error in update voltage register 0x%02x\n",
			rdev->desc->vsel_reg);
	return ret;
}

static int rc5t583_regulator_enable_time(struct regulator_dev *rdev)
{
	struct rc5t583_regulator *reg = rdev_get_drvdata(rdev);
	int vsel = regulator_get_voltage_sel_regmap(rdev);
	int curr_uV = regulator_list_voltage_linear(rdev, vsel);

	return DIV_ROUND_UP(curr_uV, reg->reg_info->enable_uv_per_us);
}

static int rc5t583_set_voltage_time_sel(struct regulator_dev *rdev,
		unsigned int old_selector, unsigned int new_selector)
{
	struct rc5t583_regulator *reg = rdev_get_drvdata(rdev);
	int old_uV, new_uV;
	old_uV = regulator_list_voltage_linear(rdev, old_selector);

	if (old_uV < 0)
		return old_uV;

	new_uV = regulator_list_voltage_linear(rdev, new_selector);
	if (new_uV < 0)
		return new_uV;

	return DIV_ROUND_UP(abs(old_uV - new_uV),
				reg->reg_info->change_uv_per_us);
}


static struct regulator_ops rc5t583_ops = {
	.is_enabled		= regulator_is_enabled_regmap,
	.enable			= regulator_enable_regmap,
	.disable		= regulator_disable_regmap,
	.enable_time		= rc5t583_regulator_enable_time,
	.get_voltage_sel	= regulator_get_voltage_sel_regmap,
	.set_voltage		= rc5t583_set_voltage,
	.list_voltage		= regulator_list_voltage_linear,
	.set_voltage_time_sel	= rc5t583_set_voltage_time_sel,
};

#define RC5T583_REG(_id, _en_reg, _en_bit, _disc_reg, _disc_bit, \
		_vout_mask, _min_mv, _max_mv, _step_uV, _enable_mv) \
{								\
	.reg_disc_reg	= RC5T583_REG_##_disc_reg,		\
	.disc_bit	= _disc_bit,				\
	.deepsleep_reg	= RC5T583_REG_##_id##DAC_DS,		\
	.max_uV		= _max_mv * 1000,			\
	.enable_uv_per_us = _enable_mv * 1000,			\
	.change_uv_per_us = 40 * 1000,				\
	.deepsleep_id	= RC5T583_DS_##_id,			\
	.desc = {						\
		.name = "rc5t583-regulator-"#_id,		\
		.id = RC5T583_REGULATOR_##_id,			\
		.n_voltages = (_max_mv - _min_mv) * 1000 / _step_uV + 1, \
		.ops = &rc5t583_ops,				\
		.type = REGULATOR_VOLTAGE,			\
		.owner = THIS_MODULE,				\
		.vsel_reg = RC5T583_REG_##_id##DAC,		\
		.vsel_mask = _vout_mask,			\
		.enable_reg = RC5T583_REG_##_en_reg,		\
		.enable_mask = BIT(_en_bit),			\
		.min_uV	= _min_mv * 1000,			\
		.uV_step = _step_uV,				\
	},							\
}

static struct rc5t583_regulator_info rc5t583_reg_info[RC5T583_REGULATOR_MAX] = {
	RC5T583_REG(DC0, DC0CTL, 0, DC0CTL, 1, 0x7F, 700, 1500, 12500, 4),
	RC5T583_REG(DC1, DC1CTL, 0, DC1CTL, 1, 0x7F, 700, 1500, 12500, 14),
	RC5T583_REG(DC2, DC2CTL, 0, DC2CTL, 1, 0x7F, 900, 2400, 12500, 14),
	RC5T583_REG(DC3, DC3CTL, 0, DC3CTL, 1, 0x7F, 900, 2400, 12500, 14),
	RC5T583_REG(LDO0, LDOEN2, 0, LDODIS2, 0, 0x7F, 900, 3400, 25000, 160),
	RC5T583_REG(LDO1, LDOEN2, 1, LDODIS2, 1, 0x7F, 900, 3400, 25000, 160),
	RC5T583_REG(LDO2, LDOEN2, 2, LDODIS2, 2, 0x7F, 900, 3400, 25000, 160),
	RC5T583_REG(LDO3, LDOEN2, 3, LDODIS2, 3, 0x7F, 900, 3400, 25000, 160),
	RC5T583_REG(LDO4, LDOEN2, 4, LDODIS2, 4, 0x3F, 750, 1500, 12500, 133),
	RC5T583_REG(LDO5, LDOEN2, 5, LDODIS2, 5, 0x7F, 900, 3400, 25000, 267),
	RC5T583_REG(LDO6, LDOEN2, 6, LDODIS2, 6, 0x7F, 900, 3400, 25000, 133),
	RC5T583_REG(LDO7, LDOEN2, 7, LDODIS2, 7, 0x7F, 900, 3400, 25000, 233),
	RC5T583_REG(LDO8, LDOEN1, 0, LDODIS1, 0, 0x7F, 900, 3400, 25000, 233),
	RC5T583_REG(LDO9, LDOEN1, 1, LDODIS1, 1, 0x7F, 900, 3400, 25000, 133),
};

static int __devinit rc5t583_regulator_probe(struct platform_device *pdev)
{
	struct rc5t583 *rc5t583 = dev_get_drvdata(pdev->dev.parent);
	struct rc5t583_platform_data *pdata = dev_get_platdata(rc5t583->dev);
	struct regulator_init_data *reg_data;
	struct regulator_config config = { };
	struct rc5t583_regulator *reg = NULL;
	struct rc5t583_regulator *regs;
	struct regulator_dev *rdev;
	struct rc5t583_regulator_info *ri;
	int ret;
	int id;

	if (!pdata) {
		dev_err(&pdev->dev, "No platform data, exiting...\n");
		return -ENODEV;
	}

	regs = devm_kzalloc(&pdev->dev, RC5T583_REGULATOR_MAX *
			sizeof(struct rc5t583_regulator), GFP_KERNEL);
	if (!regs) {
		dev_err(&pdev->dev, "Memory allocation failed exiting..\n");
		return -ENOMEM;
	}


	for (id = 0; id < RC5T583_REGULATOR_MAX; ++id) {
		reg_data = pdata->reg_init_data[id];

		/* No need to register if there is no regulator data */
		if (!reg_data)
			continue;

		reg = &regs[id];
		ri = &rc5t583_reg_info[id];
		reg->reg_info = ri;
		reg->mfd = rc5t583;
		reg->dev = &pdev->dev;

		if (ri->deepsleep_id == RC5T583_DS_NONE)
			goto skip_ext_pwr_config;

		ret = rc5t583_ext_power_req_config(rc5t583->dev,
				ri->deepsleep_id,
				pdata->regulator_ext_pwr_control[id],
				pdata->regulator_deepsleep_slot[id]);
		/*
		 * Configuring external control is not a major issue,
		 * just give warning.
		 */
		if (ret < 0)
			dev_warn(&pdev->dev,
				"Failed to configure ext control %d\n", id);

skip_ext_pwr_config:
		config.dev = &pdev->dev;
		config.init_data = reg_data;
		config.driver_data = reg;
		config.regmap = rc5t583->regmap;

		rdev = regulator_register(&ri->desc, &config);
		if (IS_ERR(rdev)) {
			dev_err(&pdev->dev, "Failed to register regulator %s\n",
						ri->desc.name);
			ret = PTR_ERR(rdev);
			goto clean_exit;
		}
		reg->rdev = rdev;
	}
	platform_set_drvdata(pdev, regs);
	return 0;

clean_exit:
	while (--id >= 0)
		regulator_unregister(regs[id].rdev);

	return ret;
}

static int __devexit rc5t583_regulator_remove(struct platform_device *pdev)
{
	struct rc5t583_regulator *regs = platform_get_drvdata(pdev);
	int id;

	for (id = 0; id < RC5T583_REGULATOR_MAX; ++id)
		regulator_unregister(regs[id].rdev);
	return 0;
}

static struct platform_driver rc5t583_regulator_driver = {
	.driver	= {
		.name	= "rc5t583-regulator",
		.owner	= THIS_MODULE,
	},
	.probe		= rc5t583_regulator_probe,
	.remove		= __devexit_p(rc5t583_regulator_remove),
};

static int __init rc5t583_regulator_init(void)
{
	return platform_driver_register(&rc5t583_regulator_driver);
}
subsys_initcall(rc5t583_regulator_init);

static void __exit rc5t583_regulator_exit(void)
{
	platform_driver_unregister(&rc5t583_regulator_driver);
}
module_exit(rc5t583_regulator_exit);

MODULE_AUTHOR("Laxman Dewangan <ldewangan@nvidia.com>");
MODULE_DESCRIPTION("RC5T583 regulator driver");
MODULE_ALIAS("platform:rc5t583-regulator");
MODULE_LICENSE("GPL v2");
