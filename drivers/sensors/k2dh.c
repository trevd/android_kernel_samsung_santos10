/*
 *  STMicroelectronics k2dh acceleration sensor driver
 *
 *  Copyright (C) 2010 Samsung Electronics Co.Ltd
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/sensor/yas.h>
#include <linux/sensor/sensors_core.h>
#include <linux/sensor/k2dh.h>
#include "k2dh_reg.h"
#include <linux/input.h>

#define k2dh_infomsg(str, args...) pr_info("%s: " str, __func__, ##args)

#define VENDOR		"STM"
#define CHIP_ID		"K2DH"

/* The default settings when sensor is on is for all 3 axis to be enabled
 * and output data rate set to 400Hz.  Output is via a ioctl read call.
 */
#define DEFAULT_POWER_ON_SETTING (ODR400 | ENABLE_ALL_AXES)
#define ACC_DEV_MAJOR 241

#define CALIBRATION_FILE_PATH	"/efs/calibration_data"
#define CAL_DATA_AMOUNT	20
#define Z_CAL_SIZE	1024

/* ABS axes parameter range [um/s^2] (for input event) */
#define GRAVITY_EARTH		9806550
#define ABSMAX_2G		(GRAVITY_EARTH * 2)
#define ABSMIN_2G		(-GRAVITY_EARTH * 2)
#define MIN_DELAY	5
#define MAX_DELAY	200

static const struct odr_delay {
	u8 odr; /* odr reg setting */
	s64 delay_ns; /* odr in ns */
} odr_delay_table[] = {
	{ ODR1344,     744047LL }, /* 1344Hz */
	{  ODR400,    2500000LL }, /*  400Hz */
	{  ODR200,    5000000LL }, /*  200Hz */
	{  ODR100,   10000000LL }, /*  100Hz */
	{   ODR50,   20000000LL }, /*   50Hz */
	{   ODR25,   40000000LL }, /*   25Hz */
	{   ODR10,  100000000LL }, /*   10Hz */
	{    ODR1, 1000000000LL }, /*    1Hz */
};

/* K2DH acceleration data */
struct k2dh_acc {
	s16 x;
	s16 y;
	s16 z;
};

struct k2dh_data {
	struct i2c_client *client;
	struct mutex read_lock;
	struct mutex write_lock;
	struct device *dev;
	struct k2dh_acc cal_data;
	struct k2dh_acc acc_xyz;
	u8 ctrl_reg1_shadow;
	atomic_t opened; /* opened implies enabled */
	struct input_dev *input;
	struct delayed_work work;
	atomic_t delay;
	atomic_t enable;
	u8 position;
	axes_func_s16 convert_axes;
	axes_func_s16 (*select_func) (u8);
};

/* Read X,Y and Z-axis acceleration raw data */
static int k2dh_read_accel_raw_xyz(struct k2dh_data *data,
				struct k2dh_acc *acc)
{
	int err;
	s8 reg = OUT_X_L | AC; /* read from OUT_X_L to OUT_Z_H by auto-inc */
	u8 acc_data[6];

	err = i2c_smbus_read_i2c_block_data(data->client, reg,
					    sizeof(acc_data), acc_data);
	if (err != sizeof(acc_data)) {
		pr_err("%s : failed to read 6 bytes for getting x/y/z(err=%d)\n",
		       __func__, err);
		return -EIO;
	}

	acc->x = (acc_data[1] << 8) | acc_data[0];
	acc->y = (acc_data[3] << 8) | acc_data[2];
	acc->z = (acc_data[5] << 8) | acc_data[4];

	acc->x = acc->x >> 4;
	acc->y = acc->y >> 4;
	acc->z = acc->z >> 4;

	if (data->convert_axes)
			data->convert_axes(&acc->x,
				&acc->y, &acc->z);

	return 0;
}

static int k2dh_read_accel_xyz(struct k2dh_data *data,
				struct k2dh_acc *acc)
{
	int err = 0;

	mutex_lock(&data->read_lock);
	err = k2dh_read_accel_raw_xyz(data, acc);
	mutex_unlock(&data->read_lock);
	if (err < 0) {
		pr_err("%s: k2dh_read_accel_raw_xyz() failed\n", __func__);
		return err;
	}

	acc->x -= data->cal_data.x;
	acc->y -= data->cal_data.y;
	acc->z -= data->cal_data.z;

	return err;
}

static int k2dh_open_calibration(struct k2dh_data *data)
{
	struct file *cal_filp = NULL;
	int err = 0;
	mm_segment_t old_fs;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(CALIBRATION_FILE_PATH, O_RDONLY, 0666);
	if (IS_ERR(cal_filp)) {
		err = PTR_ERR(cal_filp);
		if (err != -ENOENT)
			pr_err("%s: Can't open calibration file(%d)\n",
				__func__, err);
		set_fs(old_fs);
		return err;
	}

	err = cal_filp->f_op->read(cal_filp,
		(char *)&data->cal_data, 3 * sizeof(s16), &cal_filp->f_pos);
	if (err != 3 * sizeof(s16)) {
		pr_err("%s: Can't read the cal data from file\n", __func__);
		err = -EIO;
	}

	k2dh_infomsg("%s: (%d,%d,%d)\n", __func__,
		data->cal_data.x, data->cal_data.y, data->cal_data.z);

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	return err;
}

static int k2dh_do_calibrate(struct device *dev, bool do_calib)
{
	struct k2dh_data *acc_data = dev_get_drvdata(dev);
	struct k2dh_acc data = { 0, };
	struct file *cal_filp = NULL;
	int sum[3] = { 0, };
	int err = 0;
	int i;
	s16 z_cal;
	mm_segment_t old_fs;

	if (do_calib) {
		for (i = 0; i < CAL_DATA_AMOUNT; i++) {
			mutex_lock(&acc_data->read_lock);
			err = k2dh_read_accel_raw_xyz(acc_data, &data);
			mutex_unlock(&acc_data->read_lock);
			if (err < 0) {
				pr_err("%s: k2dh_read_accel_raw_xyz()"\
					"failed in the %dth loop\n",
					__func__, i);
				return err;
			}

			sum[0] += data.x;
			sum[1] += data.y;
			sum[2] += data.z;
		}

		acc_data->cal_data.x = sum[0] / CAL_DATA_AMOUNT;
		acc_data->cal_data.y = sum[1] / CAL_DATA_AMOUNT;
		z_cal = sum[2] / CAL_DATA_AMOUNT;

		if (z_cal > 0)
			acc_data->cal_data.z = z_cal - Z_CAL_SIZE;
		else
			acc_data->cal_data.z = z_cal + Z_CAL_SIZE;

	} else {
		acc_data->cal_data.x = 0;
		acc_data->cal_data.y = 0;
		acc_data->cal_data.z = 0;
	}

	k2dh_infomsg("%s: cal data (%d,%d,%d)\n", __func__,
	      acc_data->cal_data.x, acc_data->cal_data.y, acc_data->cal_data.z);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(CALIBRATION_FILE_PATH,
			O_CREAT | O_TRUNC | O_WRONLY, 0666);
	if (IS_ERR(cal_filp)) {
		pr_err("%s: Can't open calibration file\n", __func__);
		set_fs(old_fs);
		err = PTR_ERR(cal_filp);
		return err;
	}

	err = cal_filp->f_op->write(cal_filp,
		(char *)&acc_data->cal_data, 3 * sizeof(s16), &cal_filp->f_pos);
	if (err != 3 * sizeof(s16)) {
		pr_err("%s: Can't write the cal data to file\n", __func__);
		err = -EIO;
	}

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	return err;
}

static int k2dh_accel_enable(struct k2dh_data *data)
{
	int err = 0;
	static bool isFirst = true;

	if (atomic_read(&data->opened) == 0) {
		if (isFirst) {
			err = k2dh_open_calibration(data);
			if (err < 0 && err != -ENOENT)
				pr_err("%s: k2dh_open_calibration() failed(%d)\n",
				__func__, err);
			isFirst = false;
		}

		data->ctrl_reg1_shadow = DEFAULT_POWER_ON_SETTING;
		err = i2c_smbus_write_byte_data(data->client, CTRL_REG1,
						DEFAULT_POWER_ON_SETTING);
		if (err)
			pr_err("%s: i2c write ctrl_reg1 failed(err=%d)\n",
				__func__, err);

		err = i2c_smbus_write_byte_data(data->client, CTRL_REG4,
				CTRL_REG4_HR | CTRL_REG4_BDU);
		if (err)
			pr_err("%s: i2c write ctrl_reg4 failed(err=%d)\n",
				__func__, err);
	}

	atomic_add(1, &data->opened);

	return err;
}

static int k2dh_accel_disable(struct k2dh_data *data)
{
	int err = 0;

	atomic_sub(1, &data->opened);
	if (atomic_read(&data->opened) == 0) {
		err = i2c_smbus_write_byte_data(data->client, CTRL_REG1,
								PM_OFF);
		if (err)
			pr_err("%s: i2c write ctrl_reg1 failed(err=%d)\n",
				__func__, err);
		data->ctrl_reg1_shadow = PM_OFF;
	}

	return err;
}

static s64 k2dh_get_delay(struct k2dh_data *data)
{
	int i;
	u8 odr;
	s64 delay = -1;

	odr = data->ctrl_reg1_shadow & ODR_MASK;
	for (i = 0; i < ARRAY_SIZE(odr_delay_table); i++) {
		if (odr == odr_delay_table[i].odr) {
			delay = odr_delay_table[i].delay_ns;
			break;
		}
	}
	return delay;
}

static int k2dh_set_delay(struct k2dh_data *data, s64 delay_ns)
{
	int odr_value = ODR1;
	int res = 0;
	int i;

	/* round to the nearest delay that is less than
	 * the requested value (next highest freq)
	 */
	for (i = 0; i < ARRAY_SIZE(odr_delay_table); i++) {
		if (delay_ns < odr_delay_table[i].delay_ns)
			break;
	}
	if (i > 0)
		i--;
	odr_value = odr_delay_table[i].odr;
	delay_ns = odr_delay_table[i].delay_ns;

	k2dh_infomsg("old=%lldns, new=%lldns, odr=0x%x/0x%x\n",
		k2dh_get_delay(data), delay_ns, odr_value,
			data->ctrl_reg1_shadow);
	mutex_lock(&data->write_lock);
	if (odr_value != (data->ctrl_reg1_shadow & ODR_MASK)) {
		u8 ctrl = (data->ctrl_reg1_shadow & ~ODR_MASK);
		ctrl |= odr_value;
		data->ctrl_reg1_shadow = ctrl;
		res = i2c_smbus_write_byte_data(data->client, CTRL_REG1, ctrl);
		if (res)
			pr_err("%s: i2c write ctrl_reg1 failed(err=%d)\n",
				__func__, res);
	}
	mutex_unlock(&data->write_lock);
	return res;
}

static int k2dh_suspend(struct device *dev)
{
	int res = 0;
	struct k2dh_data *data = dev_get_drvdata(dev);

	if (atomic_read(&data->enable)) {
		cancel_delayed_work_sync(&data->work);
		res = i2c_smbus_write_byte_data(data->client,
						CTRL_REG1, PM_OFF);
		if (res < 0)
			pr_err("%s: i2c write ctrl_reg1 failed(err=%d)\n",
				__func__, res);
	}
	return res;
}

static int k2dh_resume(struct device *dev)
{
	int res = 0;
	struct k2dh_data *data = dev_get_drvdata(dev);

	if (atomic_read(&data->enable)) {
		res = i2c_smbus_write_byte_data(data->client, CTRL_REG1,
						data->ctrl_reg1_shadow);
		if (res < 0)
			pr_err("%s: i2c write ctrl_reg1 failed(err=%d)\n",
				__func__, res);

		schedule_delayed_work(&data->work,
			msecs_to_jiffies(5));
	}

	return res;
}

static const struct dev_pm_ops k2dh_pm_ops = {
	.suspend = k2dh_suspend,
	.resume = k2dh_resume,
};

static ssize_t k2dh_enable_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct k2dh_data *data = input_get_drvdata(input);

	return sprintf(buf, "%d\n", atomic_read(&data->enable));
}

static ssize_t k2dh_enable_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct k2dh_data *data = input_get_drvdata(input);
	u8 enable = 0;
	int err;

	if (kstrtou8(buf, 10, &enable))
		return -EINVAL;

	if (enable) {
		err = k2dh_accel_enable(data);
		if (err < 0)
			goto done;
		schedule_delayed_work(&data->work,
			msecs_to_jiffies(5));
	} else {
		cancel_delayed_work_sync(&data->work);
		err = k2dh_accel_disable(data);
		if (err < 0)
			goto done;
	}
	atomic_set(&data->enable, (int)enable);
	pr_info("%s, enable = %d\n", __func__, enable);
done:
	return count;
}
static DEVICE_ATTR(enable,
		   S_IRUGO | S_IWUSR | S_IWGRP,
		   k2dh_enable_show, k2dh_enable_store);

static ssize_t k2dh_delay_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct k2dh_data *data = input_get_drvdata(input);

	return sprintf(buf, "%d\n", atomic_read(&data->delay));
}

static ssize_t k2dh_delay_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct k2dh_data *data = input_get_drvdata(input);
	unsigned long delay = 0;
	if (_kstrtoul(buf, 10, &delay))
		return -EINVAL;

	if (delay > MAX_DELAY)
		delay = MAX_DELAY;
	if (delay < MIN_DELAY)
		delay = MIN_DELAY;
	atomic_set(&data->delay, delay);
	k2dh_set_delay(data, delay * 1000000);
	pr_info("%s, delay = %ld\n", __func__, delay);
	return count;
}
static DEVICE_ATTR(poll_delay,
		   S_IRUGO | S_IWUSR | S_IWGRP,
		   k2dh_delay_show, k2dh_delay_store);

static struct attribute *k2dh_attributes[] = {
	&dev_attr_enable.attr,
	&dev_attr_poll_delay.attr,
	NULL
};

static struct attribute_group k2dh_attribute_group = {
	.attrs = k2dh_attributes
};

static ssize_t k2dh_calibration_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int err;
	struct k2dh_data *data = dev_get_drvdata(dev);

	err = k2dh_open_calibration(data);
	if (err < 0)
		pr_err("%s: k2dh_open_calibration() failed\n", __func__);

	if (!data->cal_data.x && !data->cal_data.y && !data->cal_data.z)
		err = -1;

	return sprintf(buf, "%d %d %d %d\n",
		err, data->cal_data.x, data->cal_data.y, data->cal_data.z);
}

static ssize_t k2dh_calibration_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct k2dh_data *data = dev_get_drvdata(dev);
	bool do_calib;
	int err;

	if (sysfs_streq(buf, "1"))
		do_calib = true;
	else if (sysfs_streq(buf, "0"))
		do_calib = false;
	else {
		pr_debug("%s: invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	if (atomic_read(&data->opened) == 0) {
		/* if off, turn on the device.*/
		err = i2c_smbus_write_byte_data(data->client, CTRL_REG1,
			DEFAULT_POWER_ON_SETTING);
		if (err) {
			pr_err("%s: i2c write ctrl_reg1 failed(err=%d)\n",
				__func__, err);
		}
	}

	err = k2dh_do_calibrate(dev, do_calib);
	if (err < 0) {
		pr_err("%s: k2dh_do_calibrate() failed\n", __func__);
		return err;
	}

	if (atomic_read(&data->opened) == 0) {
		/* if off, turn on the device.*/
		err = i2c_smbus_write_byte_data(data->client, CTRL_REG1,
			PM_OFF);
		if (err) {
			pr_err("%s: i2c write ctrl_reg1 failed(err=%d)\n",
				__func__, err);
		}
	}

	return count;
}

static ssize_t k2dh_accel_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", VENDOR);
}

static ssize_t k2dh_accel_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", CHIP_ID);
}

static ssize_t k2dh_fs_read(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct k2dh_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%d,%d,%d\n",
		data->acc_xyz.x, data->acc_xyz.y, data->acc_xyz.z);
}

static ssize_t
k2dh_accel_position_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct k2dh_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%u\n", data->position);
}

static ssize_t
k2dh_accel_position_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	struct k2dh_data *data = dev_get_drvdata(dev);
	int err;

	err = kstrtou8(buf, 10, &data->position);
	if (err < 0)
		pr_err("%s, kstrtoint failed.", __func__);

	if (data->select_func)
		data->convert_axes = data->select_func(data->position);

	return count;
}

static DEVICE_ATTR(name, S_IRUGO, k2dh_accel_name_show, NULL);
static DEVICE_ATTR(vendor, S_IRUGO, k2dh_accel_vendor_show, NULL);
static DEVICE_ATTR(calibration, S_IRUGO | S_IWUSR | S_IWGRP,
		   k2dh_calibration_show, k2dh_calibration_store);
static DEVICE_ATTR(raw_data, S_IRUGO, k2dh_fs_read, NULL);
static DEVICE_ATTR(position,   S_IRUGO | S_IRWXU | S_IRWXG,
	k2dh_accel_position_show, k2dh_accel_position_store);

void k2dh_shutdown(struct i2c_client *client)
{
	int res = 0;
	struct k2dh_data *data = i2c_get_clientdata(client);

	k2dh_infomsg("is called.\n");

	if (atomic_read(&data->enable))
		cancel_delayed_work_sync(&data->work);
	res = i2c_smbus_write_byte_data(data->client,
					CTRL_REG1, PM_OFF);
	if (res < 0)
		pr_err("%s: pm_off failed %d\n", __func__, res);
}

static void k2dh_work_func(struct work_struct *work)
{
	struct k2dh_data *data
		= container_of(work, struct k2dh_data, work.work);
	static int count;

	k2dh_read_accel_xyz(data, &data->acc_xyz);

	input_report_abs(data->input, ABS_X, data->acc_xyz.x);
	input_report_abs(data->input, ABS_Y, data->acc_xyz.y);
	input_report_abs(data->input, ABS_Z, data->acc_xyz.z);
	input_sync(data->input);
	if (count++ == 100) {
		pr_info("%s: x:%d, y:%d, z:%d\n", __func__,
			data->acc_xyz.x, data->acc_xyz.y, data->acc_xyz.z);
		count = 0;
	}
	schedule_delayed_work(&data->work, msecs_to_jiffies(
		atomic_read(&data->delay)));
}

static int k2dh_probe(struct i2c_client *client,
		       const struct i2c_device_id *id)
{
	struct k2dh_data *data;
	struct acc_platform_data *pdata;
	int err;

	k2dh_infomsg("is started.\n");

	if (!i2c_check_functionality(client->adapter,
				     I2C_FUNC_SMBUS_WRITE_BYTE_DATA |
				     I2C_FUNC_SMBUS_READ_I2C_BLOCK)) {
		pr_err("%s: i2c functionality check failed!\n", __func__);
		err = -ENODEV;
		goto exit;
	}

	data = kzalloc(sizeof(struct k2dh_data), GFP_KERNEL);
	if (data == NULL) {
		dev_err(&client->dev,
				"failed to allocate memory for module data\n");
		err = -ENOMEM;
		goto exit;
	}

	/* Checking device */
	err = i2c_smbus_write_byte_data(client, CTRL_REG1,
		PM_OFF);
	if (err) {
		pr_err("%s: there is no such device, err = %d\n",
							__func__, err);
		goto err_read_reg;
	}

	data->client = client;
	i2c_set_clientdata(client, data);

	mutex_init(&data->read_lock);
	mutex_init(&data->write_lock);
	atomic_set(&data->opened, 0);
	atomic_set(&data->enable, 0);
	atomic_set(&data->delay, 200);

	pdata = client->dev.platform_data;
	if (pdata && pdata->select_func) {
		data->select_func = pdata->select_func;
		data->position = pdata->accel_get_position();
		data->convert_axes = pdata->select_func(data->position);
		k2dh_infomsg("%s, position = %d\n", __func__, data->position);
	} else {
		pr_err("%s, platform_data is NULL\n", __func__);
		goto err_check_platform_data;
	}

	/* Input device */
	data->input = input_allocate_device();
	if (!data->input) {
		pr_err("%s: count not allocate input device\n", __func__);
		err = -ENOMEM;
		goto err_check_platform_data;
	}
	data->input->name = "accelerometer";
	data->input->id.bustype = BUS_I2C;
	input_set_capability(data->input, EV_ABS, ABS_MISC);
	input_set_abs_params(data->input, ABS_X, ABSMIN_2G, ABSMAX_2G, 0, 0);
	input_set_abs_params(data->input, ABS_Y, ABSMIN_2G, ABSMAX_2G, 0, 0);
	input_set_abs_params(data->input, ABS_Z, ABSMIN_2G, ABSMAX_2G, 0, 0);
	input_set_drvdata(data->input, data);

	err = input_register_device(data->input);
	if (err < 0) {
		input_free_device(data->input);
		pr_err("%s: could not register input device\n", __func__);
		goto err_input_register;
	}

	/* Setup sysfs */
	err =
	    sysfs_create_group(&data->input->dev.kobj,
			       &k2dh_attribute_group);
	if (err < 0)
		goto err_sysfs_create_group;

	/* Setup driver interface */
	INIT_DELAYED_WORK(&data->work, k2dh_work_func);

	/* creating device for test & calibration */
	data->dev = sensors_classdev_register("accelerometer_sensor");
	if (IS_ERR(data->dev)) {
		pr_err("%s: class create failed(accelerometer_sensor)\n",
			__func__);
		err = PTR_ERR(data->dev);
		goto err_acc_device_create;
	}

	err = device_create_file(data->dev, &dev_attr_raw_data);
	if (err < 0) {
		pr_err("%s: Failed to create device file(%s)\n",
				__func__, dev_attr_raw_data.attr.name);
		goto err_raw_data_device_create_file;
	}

	err = device_create_file(data->dev, &dev_attr_calibration);
	if (err < 0) {
		pr_err("%s: Failed to create device file(%s)\n",
				__func__, dev_attr_calibration.attr.name);
		goto err_cal_device_create_file;
	}

	err = device_create_file(data->dev, &dev_attr_vendor);
	if (err < 0) {
		pr_err("%s: Failed to create device file(%s)\n",
				__func__, dev_attr_vendor.attr.name);
		goto err_vendor_device_create_file;
	}

	err = device_create_file(data->dev, &dev_attr_name);
	if (err < 0) {
		pr_err("%s: Failed to create device file(%s)\n",
				__func__, dev_attr_name.attr.name);
		goto err_name_device_create_file;
	}

	err = device_create_file(data->dev, &dev_attr_position);
	if (err < 0) {
		pr_err("%s: Failed to create device file(%s)\n",
			__func__, dev_attr_position.attr.name);
		goto err_position_device_create_file;
	}


	dev_set_drvdata(data->dev, data);
	k2dh_infomsg("is successful.\n");
	return 0;

err_position_device_create_file:
	device_remove_file(data->dev, &dev_attr_name);
err_name_device_create_file:
	device_remove_file(data->dev, &dev_attr_vendor);
err_vendor_device_create_file:
	device_remove_file(data->dev, &dev_attr_calibration);
err_cal_device_create_file:
	device_remove_file(data->dev, &dev_attr_raw_data);
err_raw_data_device_create_file:
	sensors_classdev_unregister(data->dev);
err_acc_device_create:
	sysfs_remove_group(&data->input->dev.kobj,
			   &k2dh_attribute_group);
err_sysfs_create_group:
err_input_register:
	input_unregister_device(data->input);
err_check_platform_data:
	mutex_destroy(&data->read_lock);
	mutex_destroy(&data->write_lock);
err_read_reg:
	kfree(data);
exit:
	return err;
}

static int k2dh_remove(struct i2c_client *client)
{
	struct k2dh_data *data = i2c_get_clientdata(client);
	int err = 0;

	if (atomic_read(&data->opened) > 0) {
		err = i2c_smbus_write_byte_data(data->client,
					CTRL_REG1, PM_OFF);
		if (err < 0)
			pr_err("%s: pm_off failed %d\n", __func__, err);
	}

	device_remove_file(data->dev, &dev_attr_position);
	device_remove_file(data->dev, &dev_attr_name);
	device_remove_file(data->dev, &dev_attr_vendor);
	device_remove_file(data->dev, &dev_attr_calibration);
	device_remove_file(data->dev, &dev_attr_raw_data);
	sensors_classdev_unregister(data->dev);
	input_unregister_device(data->input);
	mutex_destroy(&data->read_lock);
	mutex_destroy(&data->write_lock);
	kfree(data);

	return 0;
}

static const struct i2c_device_id k2dh_id[] = {
	{ "accelerometer", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, k2dh_id);

static struct i2c_driver k2dh_driver = {
	.probe = k2dh_probe,
	.shutdown = k2dh_shutdown,
	.remove = __devexit_p(k2dh_remove),
	.id_table = k2dh_id,
	.driver = {
		.pm = &k2dh_pm_ops,
		.owner = THIS_MODULE,
		.name = "accelerometer",
	},
};

static int __init k2dh_init(void)
{
	return i2c_add_driver(&k2dh_driver);
}

static void __exit k2dh_exit(void)
{
	i2c_del_driver(&k2dh_driver);
}

module_init(k2dh_init);
module_exit(k2dh_exit);

MODULE_DESCRIPTION("k2dh accelerometer driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
