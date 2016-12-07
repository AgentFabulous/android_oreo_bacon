LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

#===============================================================================
#            Deploy the headers that can be exposed
#===============================================================================


LOCAL_CFLAGS := \
    -D_ANDROID_

LOCAL_SRC_FILES:=       \
    src/DivXDrmDecrypt.cpp

LOCAL_C_INCLUDES:= \
    $(LOCAL_PATH)/inc/mm-video-v4l2/DivxDrmDecrypt \


LOCAL_MODULE:= libdivxdrmdecrypt
LOCAL_MODULE_TAGS := optional

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/inc

LOCAL_SHARED_LIBRARIES := liblog libdl libOmxCore

LOCAL_LDLIBS +=
include $(BUILD_SHARED_LIBRARY)
