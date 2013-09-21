# arch/x86/platform/intel-mid/sec_libs/Makefile
#
# Copyright (C) 2013 Samsung Electronics Co, Ltd.
#
# This software is licensed under the terms of the GNU General Public
# License version 2, as published by the Free Software Foundation, and
# may be copied, distributed, and modified under those terms.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#

obj-$(CONFIG_BOARD_CTP_SAMSUNG)		+= sec_common.o \
					   customize-board.o \
					   intel_muxtbl.o \
					   sec_muxtbl.o \
					   common_initcall.o

# Samsung debugging features
obj-$(CONFIG_SEC_DEBUG)			+= sec_debug.o
obj-$(CONFIG_INTEL_SCU_IPC_UTIL_EMERG_OEMNIB)	+= sec_debug_scratchpad.o
obj-$(CONFIG_SAMSUNG_USE_SEC_LOG_BUF)	+= sec_log_buf.o
obj-$(CONFIG_SAMSUNG_USE_SEC_XEN_CONSOLE)	+= sec_xen_console.o
obj-$(CONFIG_SAMSUNG_ADD_GAFORENSICINFO)+= sec_gaf.o
obj-$(CONFIG_SAMSUNG_USE_GETLOG)	+= sec_getlog.o
obj-$(CONFIG_SAMSUNG_USE_LOGGER_ADDON)	+= sec_logger.o

# platform-data
obj-$(subst m,y,$(CONFIG_MFD_MAX77693))			+= platform_max77693.o
obj-$(subst m,y,$(CONFIG_SND_SOC_WM8994))		+= platform_wm8994.o
obj-$(subst m,y,$(CONFIG_SND_SOC_WM8994))		+= platform_wm8994_regulator.o
obj-$(subst m,y,$(CONFIG_SND_SOC_WM8994))		+= platform_ctp_audio_wm1811.o
obj-$(subst m,y,$(CONFIG_BT))				+= platform_bcm_bluetooth.o
obj-$(subst m,y,$(CONFIG_BLUETOOTH_BCM4330))		+= platform_bcm4330.o
obj-$(subst m,y,$(CONFIG_BLUETOOTH_BCM4334))		+= platform_bcm4334.o
obj-$(subst m,y,$(CONFIG_REGULATOR_MAX8893))		+= platform_max8893.o
obj-$(subst m,y,$(CONFIG_VIDEO_S5K5CCGX))		+= platform_s5k5ccgx.o
obj-$(subst m,y,$(CONFIG_VIDEO_DB8131M))		+= platform_db8131m.o
obj-$(subst m,y,$(CONFIG_BOARD_CTP_SAMSUNG))		+= platform_sec_gps.o
obj-$(subst m,y,$(CONFIG_HSI_ARASAN))			+= platform_svnet_modem.o
obj-$(subst m,y,$(CONFIG_SAMSUNG_MHL))			+= platform_mhl.o
#Sensor
obj-$(subst m,y,$(CONFIG_SENSORS_K2DH))			+= platform_k2dh.o
obj-$(subst m,y,$(CONFIG_SENSORS_YAS532))		+= platform_yas532.o
obj-$(subst m,y,$(CONFIG_SENSORS_BH1730))		+= platform_bh1730.o
obj-$(subst m,y,$(CONFIG_SENSORS_ASP01))	+= platform_asp01.o