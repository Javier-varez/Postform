LOCAL_DIR := $(call current-dir)

include $(CLEAR_VARS)
LOCAL_NAME := format
TARGET_CFLAGS := \
    -mcpu=cortex-m3 \
    -mfloat-abi=soft \
    -mthumb
POSTFORM_CFLAGS := \
    -Os \
    -g3 \
    -I$(LOCAL_DIR)/inc \
    -DSTM32F1 \
    -Wall \
    -Werror \
    -Wextra \
    -Wno-gnu-string-literal-operator-template
POSTFORM_CXXFLAGS := \
    -std=gnu++20 \
    -fno-exceptions \
    -fno-rtti \
    -ffunction-sections \
    -fdata-sections
LOCAL_CFLAGS := \
    $(TARGET_CFLAGS) \
    $(POSTFORM_CFLAGS)
LOCAL_CXXFLAGS := \
    $(LOCAL_CFLAGS) \
    $(POSTFORM_CXXFLAGS)
LOCAL_LDFLAGS := \
    -Wl,--gc-sections \
    -lnosys
LOCAL_LINKER_FILE := \
    $(LOCAL_DIR)/memory.ld
LOCAL_SRC := \
    $(LOCAL_DIR)/src/postform_config.cpp \
    $(LOCAL_DIR)/src/main.cpp
LOCAL_ARM_ARCHITECTURE := v7-m
LOCAL_ARM_FPU := nofp
LOCAL_COMPILER := arm_clang
LOCAL_STATIC_LIBS := \
    libditto \
    libcortex_m_startup \
    libcortex_m_hal \
    libpostform
include $(BUILD_BINARY)

include $(CLEAR_VARS)
CC := clang
CXX := clang++
LOCAL_NAME := format_host
LOCAL_CFLAGS := $(POSTFORM_CFLAGS)
LOCAL_CXXFLAGS := \
    $(LOCAL_CFLAGS) \
    $(POSTFORM_CXXFLAGS)
LOCAL_SRC := $(LOCAL_DIR)/src/host_main.cpp
LOCAL_STATIC_LIBS := \
    libpostform_host \
    libditto_host
LOCAL_LINKER_FILE := $(LOCAL_DIR)/host.ld
include $(BUILD_BINARY)
