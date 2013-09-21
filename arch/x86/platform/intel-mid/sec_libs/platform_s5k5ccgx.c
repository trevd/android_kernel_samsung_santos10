#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/atomisp_platform.h>
#include <asm/intel_scu_ipcutil.h>
#include <asm/intel-mid.h>
#include <media/v4l2-subdev.h>
#include <media/s5k5ccgx_platform.h>
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

static struct device *s5k5ccgx_dev;

static int cam_gpio_init(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(cam_gpios); i++)
		cam_gpios[i].gpio =
				get_gpio_by_name(cam_gpios[i].label);

	if (gpio_request_array(cam_gpios, ARRAY_SIZE(cam_gpios)) < 0) {
		pr_info("s5k5ccgx gpio request error\n");
		return -1;
	}

	return 0;
}

static void cam_gpio_deinit(void)
{
	gpio_free_array(cam_gpios, ARRAY_SIZE(cam_gpios));
}


static int s5k5ccgx_flisclk_ctrl(int clk_type, int flag)
{
	static const unsigned int clock_khz = 19200;

	return intel_scu_ipc_osc_clk(clk_type, flag ? clock_khz : 0);
}

static int s5k5ccgx_power_on(void)
{
	struct regulator *regulator;
	int ret = 0;

	cam_gpio_init();

	/* 1.3M CEN (VT_CAM_nSTBY) */
	gpio_set_value(cam_gpios[GPIO_VT_CAM_nSTBY].gpio, 0);

	/* 1.3M Reset (VT_CAM_nRST) */
	gpio_set_value(cam_gpios[GPIO_VT_CAM_nRST].gpio, 0);

	/* VDDIO 1.8v (CAM_IO_1.8V) */
	regulator = regulator_get(s5k5ccgx_dev, "cam_io_1.8v");
	if (IS_ERR(regulator))
		return -ENODEV;
	ret = regulator_enable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR_RET(ret, "cam_io_1.8v");
	udelay(100); /* >0us */

	/* AVDD 2.8v (CAM_A2.8V) */
	regulator = regulator_get(s5k5ccgx_dev, "cam_a2.8v");
	if (IS_ERR(regulator))
		return -ENODEV;
	ret = regulator_enable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR_RET(ret, "cam_a2.8v");
	udelay(100); /* >0us */

	/* 1.3M Core 1.8v (VT_CORE_1.8V) */
	regulator = regulator_get(s5k5ccgx_dev, "vt_core_1.5v");
	if (IS_ERR(regulator))
		return -ENODEV;
	ret = regulator_enable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR_RET(ret, "vt_core_1.5v");
	udelay(100); /* >0us */

	/* 3M Core 1.2v (3MP_CORE_1.2V) */
	regulator = regulator_get(s5k5ccgx_dev, "3mp_core_1.2v");
	if (IS_ERR(regulator))
		return -ENODEV;
	ret = regulator_enable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR_RET(ret, "3mp_core_1.2v");

	/* MCLK (CAM_MCLK) */
	ret = s5k5ccgx_flisclk_ctrl(OSC_CLK_CAM0, 1); /* Primary */
	/* HW team request */
	/*ret = s5k5ccgx_flisclk_ctrl(OSC_CLK_CAM1, 1);*/ /* Secondary */
	udelay(100); /* >0us */

	/* 3M_nSTBY */
	gpio_set_value(cam_gpios[GPIO_3M_nSTBY].gpio, 1);
	udelay(100); /* >15us */

	/* 3M_nRST */
	gpio_set_value(cam_gpios[GPIO_3M_nRST].gpio, 1);
	msleep(20); /* >10ms */

	return ret;
}


static int s5k5ccgx_power_down(void)
{
	struct regulator *regulator;
	int ret = 0;

	/* 1.3M CEN (VT_CAM_nSTBY) */
	gpio_set_value(cam_gpios[GPIO_VT_CAM_nSTBY].gpio, 0);

	/* 1.3M Reset (VT_CAM_nRST) */
	gpio_set_value(cam_gpios[GPIO_VT_CAM_nRST].gpio, 0);

	/* 3M_nRST Low*/
	gpio_set_value(cam_gpios[GPIO_3M_nRST].gpio, 0);
	udelay(50);

	/* CAM_MCLK */
	ret = s5k5ccgx_flisclk_ctrl(OSC_CLK_CAM0, 0); /* Primary */
	if (ret != 0)
		CAM_CHECK_ERR(ret, "OSC_CLK_CAM0");

	/* HW team request */
	/*ret = s5k5ccgx_flisclk_ctrl(OSC_CLK_CAM1, 0);*/ /* Secondary */

	/* 3M_nSTBY */
	gpio_set_value(cam_gpios[GPIO_3M_nSTBY].gpio, 0);
	udelay(10); /* >10us */

	/* 1.3M Core 1.8v (VT_CORE_1.8V) */
	regulator = regulator_get(s5k5ccgx_dev, "vt_core_1.5v");
	if (IS_ERR(regulator))
		return -ENODEV;
	if (regulator_is_enabled(regulator)) {
		ret = regulator_disable(regulator);
		if (ret)
			ret = regulator_force_disable(regulator);
	}
	regulator_put(regulator);
	CAM_CHECK_ERR(ret, "vt_core_1.5v");

	/* VDDIO 1.8v (CAM_IO_1.8V) */
	regulator = regulator_get(s5k5ccgx_dev, "cam_io_1.8v");
	if (IS_ERR(regulator))
		return -ENODEV;
	if (regulator_is_enabled(regulator)) {
		ret = regulator_disable(regulator);
		if (ret)
			ret = regulator_force_disable(regulator);
	}
	regulator_put(regulator);
	CAM_CHECK_ERR(ret, "cam_io_1.8v");

	/* AVDD 2.8v (CAM_A2.8V) */
	regulator = regulator_get(s5k5ccgx_dev, "cam_a2.8v");
	if (IS_ERR(regulator))
		return -ENODEV;
	if (regulator_is_enabled(regulator)) {
		ret = regulator_disable(regulator);
		if (ret)
			ret = regulator_force_disable(regulator);
	}
	regulator_put(regulator);
	CAM_CHECK_ERR(ret, "cam_a2.8v");

	/* 3M Core 1.2v (3MP_CORE_1.2V) */
	regulator = regulator_get(s5k5ccgx_dev, "3mp_core_1.2v");
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

static int s5k5ccgx_power_ctrl(struct v4l2_subdev *sd, int flag)
{
	int reg_err;

	pr_info("%s %s\n", __func__, flag ? "on" : "down");

	if (flag)
		reg_err = s5k5ccgx_power_on();
	else
		reg_err = s5k5ccgx_power_down();

	return reg_err;
}

static int s5k5ccgx_csi_configure(struct v4l2_subdev *sd, int flag)
{
	static const int LANES = 1;

	/* soc sensor, there is no raw bayer order (set to -1) */
	return camera_sensor_csi(sd, ATOMISP_CAMERA_PORT_PRIMARY, LANES,
		ATOMISP_INPUT_FORMAT_YUV422_8, -1, flag);
}

static int s5k5ccgx_platform_init(struct i2c_client *client)
{
	int ret = 0;

	s5k5ccgx_dev = &client->dev;

	return ret;
}

static int s5k5ccgx_platform_deinit(void)
{
	return 0;
}

static struct s5k5ccgx_platform_data s5k5ccgx_sensor_platform_data = {
	.power_ctrl     = s5k5ccgx_power_ctrl,
	.csi_cfg        = s5k5ccgx_csi_configure,
	.platform_init = s5k5ccgx_platform_init,
	.platform_deinit = s5k5ccgx_platform_deinit,
	.default_width = 1024,
	.default_height = 768,
	.pixelformat = V4L2_PIX_FMT_UYVY,
	.freq = 24000000,
	.is_mipi = 1,
	.streamoff_delay = S5K5CCGX_STREAMOFF_DELAY,
	.dbg_level = CAMDBG_LEVEL_DEFAULT,
};

void *s5k5ccgx_platform_data(void *data)
{
	return &s5k5ccgx_sensor_platform_data;
}
