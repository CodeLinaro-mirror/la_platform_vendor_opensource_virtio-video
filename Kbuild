# SPDX-License-Identifier: GPL-2.0-only

#ifeq ($(CONFIG_MSM_VIRTIO_VIDEO), y)
LINUXINCLUDE    += -I${VIDEO_ROOT}/include/uapi \
                   -I${KERNEL_ROOT}/include \
                   -I${VIDEO_ROOT}/driver

USERINCLUDE     += -I${VIDEO_ROOT}/include/uapi

KBUILD_CPPFLAGS += -DVIRTIO_VIDEO_MSM
KBUILD_CPPFLAGS += -DMSM_HAB_NO_SUPPORT

ccflags-y := -I"$(src)/include/uapi"

msm_virtio_video-objs := \
	driver/virtio_video_driver.o \
	driver/virtio_video_vq.o \
	driver/virtio_video_device.o \
	driver/virtio_video_dec.o \
	driver/virtio_video_enc.o \
	driver/virtio_video_cam.o \
	driver/virtio_video_caps.o \
	driver/virtio_video_helpers.o \
	driver/virtio_video_msm_mem.o \
	driver/virtio_video_msm_debug.o \
	driver/virtio_video_msm_v4l2.o \
	driver/virtio_video_msm_vb2.o \
	driver/virtio_video_msm_vq.o \
	driver/virtio_video_msm_hab.o

obj-m += msm_virtio_video.o

ccflags-y += -DDRIVER_VERSION=\"2.3.0\"
#endif
