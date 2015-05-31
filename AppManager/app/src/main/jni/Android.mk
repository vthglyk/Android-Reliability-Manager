LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE    := send_Pids_to_kernel
LOCAL_SRC_FILES := send_Pids_to_kernel.c
include $(BUILD_SHARED_LIBRARY)