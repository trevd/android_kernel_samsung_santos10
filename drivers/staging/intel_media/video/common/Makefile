# Makefile for the drm device driver. This driver provides support for the
# Direct Rendering Infrastructure (DRI) in XFree86 4.1.0 and higher.

INCDIR=drivers/staging/mrfl
MEDIA_INCDIR=drivers/staging/intel_media
ccflags-y += \
	-I$(INCDIR)/ \
	-I$(INCDIR)/rgx/include \
	-I$(INCDIR)/interface \
	-I$(INCDIR)/drv \
	-I$(INCDIR)/rgx/services/server/env/linux \
	-I$(INCDIR)/rgx/services/server/include \
	-I$(INCDIR)/../intel_media/video/common \
	-I$(INCDIR)/../intel_media/video/decode \
	-I$(INCDIR)/../intel_media/video/encode \
	-I$(INCDIR)/../intel_media/video/vsp \
	-I$(INCDIR)/../../../include/linux \
	-I$(INCDIR)/../../../include/drm \
	-I$(INCDIR)/../../../include/drm/ttm

ccflags-y += -DANDROID -D_linux_ -DLINUX -D__KERNEL__  -DMERRIFIELD -DPSB_MSVDX_FW_LOADED_BY_HOST
ccflags-$(CONFIG_SUPPORT_VSP) += -DSUPPORT_VSP

#VIDEO_COMMON_DIR = $(TOP_DIR)/driver/staging/intel_media/video/common
#DECODE_DIR = $(TOP_DIR)/driver/staging/intel_media/video/decode

obj-y += \
	psb_ttm_glue.o \
	psb_cmdbuf.o \
	psb_buffer.o \
	psb_fence.o \
	psb_mmu.o \
	psb_ttm_fence.o \
	psb_ttm_fence_user.o \
	psb_ttm_placement_user.o

