LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := emu-players
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
    LOCAL_CFLAGS := -DHAVE_NEON=1
    LOCAL_ARM_NEON := true
endif
LOCAL_ARM_MODE := arm
LOCAL_CFLAGS += -ffast-math -O3
FILE_LIST := $(wildcard $(LOCAL_PATH)/../../../emu-players/*.cpp)
LOCAL_SRC_FILES := $(FILE_LIST:$(LOCAL_PATH)/%=%)
LHA_FILE_LIST := $(wildcard $(LOCAL_PATH)/../../../tizen/serviceapp/lha/*.c)
LOCAL_SRC_FILES += $(LHA_FILE_LIST:$(LOCAL_PATH)/%=%)
LOCAL_SRC_FILES += emu-players.cpp
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../emu-players
LOCAL_LDLIBS += -llog -lz
LOCAL_LDLIBS += -lOpenSLES

include $(BUILD_SHARED_LIBRARY)
