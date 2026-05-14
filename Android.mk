# Select gen4/gen5 build based on the target platform
ifneq ($(filter gen4,$(TARGET_BOARD_PLATFORM)),)
include $(call my-dir)/gen4/Android.mk
else ifneq ($(filter gen5,$(TARGET_BOARD_PLATFORM)),)
include $(call my-dir)/gen5/Android.mk
endif
