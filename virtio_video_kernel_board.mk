# SPDX-License-Identifier: GPL-2.0-only

TARGET_VIRTIO_VIDEO_ENABLE := false
ifeq ($(ENABLE_HYP),true)
  ifneq ($(filter gen5 auto_gen,$(TARGET_BOARD_PLATFORM)),)
    TARGET_VIRTIO_VIDEO_ENABLE := true
  else ifeq ($(TARGET_USES_GY), true)
    TARGET_VIRTIO_VIDEO_ENABLE := true
  endif
endif

#check DLKM and OVERRIDE flags
ifeq ($(TARGET_VIRTIO_VIDEO_ENABLE),true)
  ifeq ($(TARGET_KERNEL_DLKM_DISABLE),true)
    ifeq ($(TARGET_KERNEL_DLKM_VIDEO_OVERRIDE),false)
      TARGET_VIRTIO_VIDEO_ENABLE := false
      $(warning "Virtio-video disabled due to DLKM overide")
    endif
  endif
endif

# Check QMAA flags
ifeq ($(TARGET_USES_QMAA), true)
  ifneq ($(TARGET_USES_QMAA_OVERRIDE_VIDEO), true)
    TARGET_VIRTIO_VIDEO_ENABLE := false
    $(warning "Virtio-video disabled due to QMAA")
  endif
endif

# Build video kernel driver
ifeq ($(TARGET_VIRTIO_VIDEO_ENABLE),true)
  $(warning "Virtio-video board vendor enabled")
  BOARD_VENDOR_KERNEL_MODULES += $(KERNEL_MODULES_OUT)/msm_virtio_video.ko
  BUILD_VIDEO_TECHPACK_SOURCE := true
endif
