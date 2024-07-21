LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libui
LOCAL_CFLAGS := -D ANDROID -Wall -Wshadow -Wcast-qual -Wpedantic -Werror=incompatible-pointer-types -Werror=deprecated-declarations -Os
LOCAL_SRC_FILES := libui.c lib.c view.c callback.c
LOCAL_C_INCLUDES += $(LOCAL_PATH)
LOCAL_LDLIBS += -llog -landroid

include $(BUILD_STATIC_LIBRARY)