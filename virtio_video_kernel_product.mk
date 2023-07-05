# SPDX-License-Identifier: GPL-2.0-only
TARGET_VIRTIO_VIDEO_ENABLE := false
ifdef TARGET_USES_GY
    ifeq ($(TARGET_KERNEL_DLKM_DISABLE), true)
        ifeq ($(TARGET_KERNEL_DLKM_VIDEO_OVERRIDE), true)
        TARGET_VIRTIO_VIDEO_ENABLE := true
        endif
    else
    TARGET_VIRTIO_VIDEO_ENABLE := true
    endif
endif

ifeq ($(TARGET_VIRTIO_VIDEO_ENABLE),true)
PRODUCT_PACKAGES  += $(KERNEL_MODULES_OUT)/msm_virtio_video.ko
endif
