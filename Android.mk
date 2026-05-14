ifneq ($(filter gen5,$(TARGET_BOARD_PLATFORM)),)
include $(call my-dir)/gen5/Android.mk
endif
