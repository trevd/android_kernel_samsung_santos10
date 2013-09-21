#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/atomisp_platform.h>
#include <asm/intel_scu_ipcutil.h>
#include <asm/intel-mid.h>
#include <media/v4l2-subdev.h>
#include <media/db8131m_platform.h>
#include <linux/regulator/consumer.h>
#include "../device_libs/platform_camera.h"

#define CAM_CHECK_ERR_RET(x, msg)   \
	{if (unlikely((x) < 0)) { \
		printk(KERN_ERR "\nfail to %s: err = %d\n", msg, x); \
		return x; \
	} }
#define CAM_CHECK_ERR(x, msg) \
	{if (unlikely((x) < 0)) { \
		printk(KERN_ERR "\nfail to %s: err = %d\n", msg, x); \
	} }

#define CAM_CHECK_ERR_GOTO(x, out, fmt, ...) \
	{if (unlikely((x) < 0)) { \
		printk(KERN_ERR fmt, ##__VA_ARGS__); \
		goto out; \
	} }

enum {
	GPIO_3M_nSTBY = 0,
	GPIO_3M_nRST,
	GPIO_VT_CAM_nSTBY,
	GPIO_VT_CAM_nRST,
};

static struct gpio cam_gpios[] = {
	[GPIO_3M_nSTBY] = {
		.flags	= GPIOF_OUT_INIT_LOW,
		.label	= "3M_nSTBY",
	},
	[GPIO_3M_nRST] = {
		.flags	= GPIOF_OUT_INIT_LOW,
		.label	= "3M_nRST",
	},
	[GPIO_VT_CAM_nSTBY] = {
		.flags	= GPIOF_OUT_INIT_LOW,
		.label	= "VT_CAM_nSTBY",
	},
	[GPIO_VT_CAM_nRST] = {
		.flags	= GPIOF_OUT_INIT_LOW,
		.label	= "VT_CAM_nRST",
	},
};

static struct device *db8131m_dev;

static int cam_gpio_init(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(cam_gpios); i++)
		cam_gpios[i].gpio =
				get_gpio_by_name(cam_gpios[i].label);

	if (gpio_request_array(cam_gpios, ARRAY_SIZE(cam_gpios)) < 0) {
		pr_info("db8131m gpio request error\n");
		return -1;
	}

	return 0;
}

static void cam_gpio_deinit(void)
{
	gpio_free_array(cam_gpios, ARRAY_SIZE(cam_gpios));
}

static int db8131m_flisclk_ctrl(int clk_type, int flag)
{
	static const unsigned int clock_khz = 19200;

	return intel_scu_ipc_osc_clk(clk_type, flag ? clock_khz : 0);
}

static int db8131m_power_on(void)
{
	struct regulator *regulator;
	int ret = 0;

	cam_gpio_init();

	/* CAM_IO_1.8V, LDO5 */
	regulator = regulator_get(db8131m_dev, "cam_io_1.8v");
	if (IS_ERR(regulator))
		return -ENODEV;
	ret = regulator_enable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR_RET(ret, "cam_io_1.8v");
	udelay(1); /* >=0us */

	/* CAM_A2.8V, LDO3 */
	regulator = regulator_get(db8131m_dev, "cam_a2.8v");
	if (IS_ERR(regulator))
		return -ENODEV;
	ret = regulator_enable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR_RET(ret, "cam_a2.8v");
	udelay(1); /* >=0us */

	/* 1.3M_CORE_1.8V, LDO2 */
	regulator = regulator_get(db8131m_dev, "vt_core_1.5v");
	if (IS_ERR(regulator))
		return -ENODEV;
	ret = regulator_enable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR_RET(ret, "vt_core_1.5v");
	udelay(1); /* >=0us */

	/* 3MP_CORE_1.2V, BUCK */
	regulator = regulator_get(db8131m_dev, "3mp_core_1.2v");
	if (IS_ERR(regulator))
		return -ENODEV;
	ret = regulator_enable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR_RET(ret, "3mp_core_1.2v");
	udelay(1); /* >=0us */

	/* 1.3M_CEN */
	gpio_set_value(cam_gpios[GPIO_VT_CAM_nSTBY].gpio, 1);
	udelay(1); /* >=0us */

	/* CAM_MCLK */
	ret = db8131m_flisclk_ctrl(OSC_CLK_CAM1, 1); /* Secondary */
	/* HW team request */
	/*ret = db8131m_flisclk_ctrl(OSC_CLK_CAM0, 1);*/ /* Primary */
	udelay(100); /* >20us */

	/* 1.3M_RESET */
	gpio_set_value(cam_gpios[GPIO_VT_CAM_nRST].gpio, 1);

	/* 3M_nSTBY, STAY LOW */
	gpio_set_value(cam_gpios[GPIO_3M_nSTBY].gpio, 0);

	/* 3M_nRST, STAY LOW */
	gpio_set_value(cam_gpios[GPIO_3M_nRST].gpio, 0);
	msleep(10); /* > 70000 cycle */
	return ret;
}


static int db8131m_power_down(void)
{
	struct regulator *regulator;
	int ret = 0;

	/* 3M_nSTBY, STAY LOW */
	gpio_set_value(cam_gpios[GPIO_3M_nSTBY].gpio, 0);

	/* 3M_nRST, STAY_LOW */
	gpio_set_value(cam_gpios[GPIO_3M_nRST].gpio, 0);

	/* 1.3M_CEN */
	gpio_set_value(cam_gpios[GPIO_VT_CAM_nSTBY].gpio, 0);
	udelay(1); /* >=0us */

	/* 1.3M_RESET */
	gpio_set_value(cam_gpios[GPIO_VT_CAM_nRST].gpio, 0);
	udelay(1); /* >=0us */

	/* CAM_MCLK */
	ret = db8131m_flisclk_ctrl(OSC_CLK_CAM1, 0); /* Secondary */
	if (ret != 0)
		CAM_CHECK_ERR(ret, "OSC_CLK_CAM1");
	/* HW team request */
	/*ret = db8131m_flisclk_ctrl(OSC_CLK_CAM0, 0);*/ /* Primary */
	udelay(1); /* >=0us */

	/* 1.3M_CORE_1.8V, LDO2 */
	regulator = regulator_get(db8131m_dev, "vt_core_1.5v");
	if (IS_ERR(regulator))
		return -ENODEV;
	if (regulator_is_enabled(regulator)) {
		ret = regulator_disable(regulator);
		if (ret)
			ret = regulator_force_disable(regulator);
	}
	regulator_put(regulator);
	CAM_CHECK_ERR(ret, "vt_core_1.5v");
	udelay(1); /* >=0us */

	/* CAM_A2.8V, LDO3 */
	regulator = regulator_get(db8131m_dev, "cam_a2.8v");
	if (IS_ERR(regulator))
		return -ENODEV;
	if (regulator_is_enabled(regulator)) {
		ret = regulator_disable(regulator);
		if (ret)
			ret = regulator_force_disable(regulator);
	}
	regulator_put(regulator);
	CAM_CHECK_ERR(ret, "cam_a2.8v");
	udelay(1); /* >=0us */

	/* CAM_IO_1.8V, LDO5 */
	regulator = regulator_get(db8131m_dev, "cam_io_1.8v");
	if (IS_ERR(regulator))
		return -ENODEV;
	if (regulator_is_enabled(regulator)) {
		ret = regulator_disable(regulator);
		if (ret)
			ret = regulator_force_disable(regulator);
	}
	regulator_put(regulator);
	CAM_CHECK_ERR(ret, "cam_io_1.8v");
	udelay(1); /* >=0us */

	/* 3MP_CORE_1.2V, BUCK */
	regulator = regulator_get(db8131m_dev, "3mp_core_1.2v");
	if (IS_ERR(regulator))
		return -ENODEV;
	if (regulator_is_enabled(regulator)) {
		ret = regulator_disable(regulator);
		if (ret)
			ret = regulator_force_disable(regulator);
	}
	regulator_put(regulator);
	CAM_CHECK_ERR(ret, "3mp_core_1.2v");

	cam_gpio_deinit();

	return ret;
}

static int db8131m_power_ctrl(struct v4l2_subdev *sd, int flag)
{
	int reg_err;

	pr_info("%s %s\n", __func__, flag ? "on" : "down");

	if (flag)
		reg_err = db8131m_power_on();
	else
		reg_err = db8131m_power_down();

	return reg_err;
}

static int db8131m_csi_configure(struct v4l2_subdev *sd, int flag)
{
	static const int LANES = 1;

	/* soc sensor, there is no raw bayer order (set to -1) */
	return camera_sensor_csi(sd, ATOMISP_CAMERA_PORT_SECONDARY, LANES,
		ATOMISP_INPUT_FORMAT_YUV422_8, -1, flag);
}

static int db8131m_platform_init(struct i2c_client *client)
{
	int ret = 0;

	db8131m_dev = &client->dev;

	return ret;
}

static int db8131m_platform_deinit(void)
{
	return 0;
}

static struct db8131m_platform_data db8131m_sensor_platform_data = {
	.power_ctrl     = db8131m_power_ctrl,
	.csi_cfg        = db8131m_csi_configure,
	.platform_init = db8131m_platform_init,
	.platform_deinit = db8131m_platform_deinit,
	.default_width = 1024,
	.default_height = 768,
	.pixelformat = V4L2_PIX_FMT_YUYV,
	.freq = 24000000,
	.is_mipi = 1,
	.streamoff_delay = DB8131M_STREAMOFF_DELAY,
	.dbg_level = CAMDBG_LEVEL_DEFAULT,
};

void *db8131m_platform_data(void *data)
{
	return &db8131m_sensor_platform_data;
}
