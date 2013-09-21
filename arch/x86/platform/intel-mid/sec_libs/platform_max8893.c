
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/gpio_event.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/max8893.h>

#include <linux/lnw_gpio.h>
#include <asm/intel-mid.h>

#define MAX8893_BUCK_NAME "3mp_core_1.2v"
#define MAX8893_LDO1_NAME "3mp_af_2.8v"
#define MAX8893_LDO2_NAME "vt_core_1.5v"
#define MAX8893_LDO3_NAME "cam_a2.8v"
#define MAX8893_LDO4_NAME "max8893_ldo4"
#define MAX8893_LDO5_NAME "cam_io_1.8v"

#define S5K5CCGX_DEV_ID "4-002d" /* bus : 4, slave addr : 0x2d */
#define DB8131M_DEV_ID "4-0045" /* bus : 4, slave addr : 0x45 */

static struct regulator_consumer_supply buck_consumer[] = {
	REGULATOR_SUPPLY(MAX8893_BUCK_NAME, S5K5CCGX_DEV_ID),
	REGULATOR_SUPPLY(MAX8893_BUCK_NAME, DB8131M_DEV_ID)
};
static struct regulator_consumer_supply ldo1_consumer[] = {
	REGULATOR_SUPPLY(MAX8893_LDO1_NAME, S5K5CCGX_DEV_ID),
	REGULATOR_SUPPLY(MAX8893_LDO1_NAME, DB8131M_DEV_ID)
};
static struct regulator_consumer_supply ldo2_consumer[] = {
	REGULATOR_SUPPLY(MAX8893_LDO2_NAME, S5K5CCGX_DEV_ID),
	REGULATOR_SUPPLY(MAX8893_LDO2_NAME, DB8131M_DEV_ID)
};
static struct regulator_consumer_supply ldo3_consumer[] = {
	REGULATOR_SUPPLY(MAX8893_LDO3_NAME, S5K5CCGX_DEV_ID),
	REGULATOR_SUPPLY(MAX8893_LDO3_NAME, DB8131M_DEV_ID)
};
static struct regulator_consumer_supply ldo4_consumer[] = {
	REGULATOR_SUPPLY(MAX8893_LDO4_NAME, S5K5CCGX_DEV_ID),
	REGULATOR_SUPPLY(MAX8893_LDO4_NAME, DB8131M_DEV_ID)
};
static struct regulator_consumer_supply ldo5_consumer[] = {
	REGULATOR_SUPPLY(MAX8893_LDO5_NAME, S5K5CCGX_DEV_ID),
	REGULATOR_SUPPLY(MAX8893_LDO5_NAME, DB8131M_DEV_ID)
};

static struct regulator_init_data platform_max8893_buck_data = {
	.constraints	= {
		.name		= MAX8893_BUCK_NAME,
		.min_uV		= 1200000,
		.max_uV		= 1200000,
		.apply_uV	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE |
				  REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	=	ARRAY_SIZE(buck_consumer),
	.consumer_supplies	=	buck_consumer,
};

static struct regulator_init_data platform_max8893_ldo1_data = {
	.constraints	= {
		.name		= MAX8893_LDO1_NAME,
		.min_uV		= 2800000,
		.max_uV		= 2800000,
		.apply_uV	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE |
				  REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	=	ARRAY_SIZE(ldo1_consumer),
	.consumer_supplies	=	ldo1_consumer,
};

static struct regulator_init_data platform_max8893_ldo2_data = {
	.constraints	= {
		.name		= MAX8893_LDO2_NAME,
		.min_uV		= 1500000,
		.max_uV		= 1500000,
		.apply_uV	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE |
				  REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	=	ARRAY_SIZE(ldo2_consumer),
	.consumer_supplies	=	ldo2_consumer,
};

static struct regulator_init_data platform_max8893_ldo3_data = {
	.constraints	= {
		.name		= MAX8893_LDO3_NAME,
		.min_uV		= 2800000,
		.max_uV		= 2800000,
		.apply_uV	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE |
				  REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	=	ARRAY_SIZE(ldo3_consumer),
	.consumer_supplies	=	ldo3_consumer,
};

static struct regulator_init_data platform_max8893_ldo4_data = {
	.constraints	= {
		.name		= MAX8893_LDO4_NAME,
		.min_uV		= 2900000,
		.max_uV		= 2900000,
		.apply_uV	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE |
				  REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	=	ARRAY_SIZE(ldo4_consumer),
	.consumer_supplies	=	ldo4_consumer,
};

static struct regulator_init_data platform_max8893_ldo5_data = {
	.constraints	= {
		.name		= MAX8893_LDO5_NAME,
		.min_uV		= 1800000,
		.max_uV		= 1800000,
		.apply_uV	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE |
				  REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	=	ARRAY_SIZE(ldo5_consumer),
	.consumer_supplies	=	ldo5_consumer,
};

static struct max8893_subdev_data platform_max8893_subdev_data[] = {
	{
		.id = MAX8893_BUCK,
		.initdata = &platform_max8893_buck_data,
	},
	{
		.id = MAX8893_LDO1,
		.initdata = &platform_max8893_ldo1_data,
	},
	{
		.id = MAX8893_LDO2,
		.initdata = &platform_max8893_ldo2_data,
	},
	{
		.id = MAX8893_LDO3,
		.initdata = &platform_max8893_ldo3_data,
	},
	{
		.id = MAX8893_LDO4,
		.initdata = &platform_max8893_ldo4_data,
	},
	{
		.id = MAX8893_LDO5,
		.initdata = &platform_max8893_ldo5_data,
	},
};

static struct max8893_platform_data platform_max8893_pdata = {
	.num_subdevs = ARRAY_SIZE(platform_max8893_subdev_data),
	.subdevs = platform_max8893_subdev_data,
};

void *max8893_platform_data(void *info)
{
	return &platform_max8893_pdata;
}
