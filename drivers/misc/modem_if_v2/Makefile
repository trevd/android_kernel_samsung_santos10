# Makefile of modem_if_v2

EXTRA_CFLAGS += -Idrivers/misc/modem_if_v2

obj-y += sipc4_modem.o sipc4_io_device.o
obj-y += modem_utils.o

obj-$(CONFIG_UMTS_MODEM_XMM626X) += modem_modemctl_device_xmm6262.o

obj-$(CONFIG_LINK_DEVICE_MIPI) += modem_link_device_mipi.o
