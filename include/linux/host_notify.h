/*
 *  Host notify class driver
 *
 * Copyright (C) 2011 Samsung, Inc.
 * Author: Dongrak Shin <dongrak.shin@samsung.com>
 *
*/

#ifndef __LINUX_HOST_NOTIFY_H__
#define __LINUX_HOST_NOTIFY_H__

enum host_uevent_state {
	NOTIFY_HOST_NONE,
	NOTIFY_HOST_ADD,
	NOTIFY_HOST_REMOVE,
	NOTIFY_HOST_OVERCURRENT,
	NOTIFY_HOST_LOWBATT,
	NOTIFY_HOST_UNKNOWN,
};

enum otg_mode {
	NOTIFY_NONE_MODE,
	NOTIFY_HOST_MODE,
	NOTIFY_PERIPHERAL_MODE,
	NOTIFY_TEST_MODE,
};

enum booster_power {
	NOTIFY_POWER_OFF,
	NOTIFY_POWER_ON,
};

enum set_command {
	NOTIFY_SET_OFF,
	NOTIFY_SET_ON,
};

enum accessory_type {
	NOTIFY_DEFAULT,
	NOTIFY_GENERALHOST,
	NOTIFY_MPOS,
	NOTIFY_AUDIODOCK,
	NOTIFY_SMARTDOCK,
};

struct host_notify_type {
	struct list_head type_list;
	char		*name;
	int		type;
	int		enable_vbus_drive;
	int		enable_noti;
};

struct host_notify_dev {
	const char	*name;
	struct device	*dev;
	struct list_head usbtype_lists;
	char		*type_name;
	int		enable_vbus_drive;
	int		enable_noti;
	int		index;
	int		state;
	int		ovc_en;
	int		mode;
	int		booster;
	void		(*set_mode)(int);
	void		(*set_booster)(int);
};

struct host_notifier_platform_data {
	struct	host_notify_dev ndev;
	int		gpio;
	void	(*booster)(int);
	int		(*usbhostd_start)(void);
	int		(*usbhostd_stop)(void);
	int		(*usbhostd_wakeup)(void);
	int		thread_enable;
	int		inverse_vbus_trigger;
};

extern void host_notify_set_ovc_en(struct host_notify_dev *ndev, int enable);
extern int host_notify_get_ovc_en(struct host_notify_dev *ndev);
extern int host_notify_get_vbusdrive_en(struct host_notify_dev *ndev);
extern int host_notify_get_noti_en(struct host_notify_dev *ndev);
extern int host_notify_set_type(struct host_notify_dev *ndev, int type);
extern void host_notify_type_register(struct host_notify_dev *ndev,
			struct host_notify_type *notify_type);
extern void host_state_notify(struct host_notify_dev *ndev, int state);
extern int host_notify_dev_register(struct host_notify_dev *ndev);
extern void host_notify_dev_unregister(struct host_notify_dev *ndev);

#endif /* __LINUX_HOST_NOTIFY_H__ */
