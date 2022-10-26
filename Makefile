# SPDX-License-Identifier: GPL-2.0-only

ifeq ($(CONFIG_MSM_VIRTIO_VIDEO), y)
LINUXINCLUDE    += -I$(srctree)/techpack/virtio-video/include/uapi

USERINCLUDE     += -I$(srctree)/techpack/virtio-video/include/uapi

obj-y += driver/
endif
