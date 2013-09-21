#include <linux/sensor/asp01.h>
#include <linux/gpio.h>
#include <asm/intel-mid.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include "platform_asp01.h"

#undef ASP01_DEBUG_DATA

#define GPIO_RF_TOUCH get_gpio_by_name("RF_TOUCH")
#define GPIO_ADJ_DET get_gpio_by_name("ADJ_DET_AP")

static int asp01_grip_sensor_gpio(void);

static struct asp01_platform_data asp01_pdata = {
	.request_gpio = asp01_grip_sensor_gpio,
};

static int asp01_grip_sensor_gpio(void)
{
	int err;

	pr_info("%s is called.\n", __func__);

	err = gpio_request(asp01_pdata.t_out, "GRIP_SENSOR_RESET");
	if (err) {
		pr_err("failed to request gpio, %d\n", err);
		return err;
	}

	asp01_pdata.irq = gpio_to_irq(asp01_pdata.t_out);
	err = gpio_request(asp01_pdata.adj_det, "asp01_adj_det");
	if (err) {
		pr_err("failed to request GPIO_ADJ_DET, %d\n", err);
		return err;
	}
	return 0;
}

void __init *asp01_platform_data(void *info)
{
	struct i2c_board_info *i2c_info = info;
	int i;
	u8 init_data[SET_REG_NUM] = {/* 29 - 3 = 26 */
		/*{ REG_PROM_EN1, 0x00}, RST */
		0x5a,/* REG_UNLOCK 0x11 */
		0x33,/* REG_RST_ERR 0x12 */
		0x30,/* REG_PROX_PER 0x13 */
		0x30,/* REG_PAR_PER 0x14 */
		0x3c,/* REG_TOUCH_PER 0x15 */
		0x18,/* REG_HI_CAL_PER 0x19 */
		0x30,/* REG_BSMFM_SET 0x1A */
		0x33,/* REG_ERR_MFM_CYC 0x1B */
		0x37,/* REG_TOUCH_MFM_CYC 0x1C */
		0x19,/* REG_HI_CAL_SPD 0x1D 10th */
		0x03,/* REG_CAL_SPD 0x1E */
		/* { REG_INIT_REF, 0x00}, */
		0x40,/* REG_BFT_MOT 0x20 */
		0x00,/* REG_TOU_RF_EXT 0x22 */
		0x10,/* REG_SYS_FUNC 0x23 */
		0x30,/* REG_OFF_TIME 0x39 */
		0x48,/* REG_SENSE_TIME 0x3A */
		0x50,/* REG_DUTY_TIME 0x3B */
		0x78,/* REG_HW_CON1 0x3C */
		0x27,/* REG_HW_CON2 0x3D */
		0x20,/* REG_HW_CON3 0x3E 20th */
		0x27,/* REG_HW_CON4 0x3F */
		0x83,/* REG_HW_CON5 0x40 */
		0x3f,/* REG_HW_CON6 0x41 */
		0x08,/* REG_HW_CON7 0x42 */
		/* { REG_HW_CON8, 0x70}, RST */
		0x27,/* REG_HW_CON10 0x45*/
		0x00/* REG_HW_CON11 0x46*/
	};

	pr_debug("%s addr : %x\n", __func__, i2c_info->addr);

	asp01_pdata.t_out = GPIO_RF_TOUCH;
	asp01_pdata.adj_det = GPIO_ADJ_DET;

	asp01_pdata.cr_divsr = 10;
	asp01_pdata.cr_divnd = 13;/* 1.3 */
	asp01_pdata.cs_divsr = 10;
	asp01_pdata.cs_divnd = 13;/* 1.3 */

	for (i = 0; i < SET_REG_NUM; i++) {
		asp01_pdata.init_code[i] = init_data[i];
#ifdef ASP01_DEBUG_DATA
		pr_info("asp01_initcode[%d] = 0x%x\n", i,
			asp01_pdata.init_code[i]);
#endif
	}

	return &asp01_pdata;
}
