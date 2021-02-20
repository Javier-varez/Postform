LOCAL_DIR := $(call current-dir)
POSTFORM_TOP_DIR := $(LOCAL_DIR)/..

include $(CLEAR_VARS)
LOCAL_NAME := format
TARGET_CFLAGS := \
    -mcpu=cortex-m3 \
    -mfloat-abi=soft \
    -mthumb
LOCAL_CFLAGS := \
    $(TARGET_CFLAGS) \
    -Os \
    -g3 \
    -I$(LOCAL_DIR)/inc \
    -DSTM32F1 \
    -Wall \
    -Werror \
    -Wextra \
    -Wno-gnu-string-literal-operator-template
LOCAL_CXXFLAGS := \
    $(LOCAL_CFLAGS) \
    -std=gnu++17 \
    -fno-exceptions \
    -fno-rtti \
    -ffunction-sections \
    -fdata-sections
LOCAL_LDFLAGS := \
    -Wl,--gc-sections \
    -lnosys
LOCAL_LINKER_FILE := \
    $(LOCAL_DIR)/memory.ld
LOCAL_SRC := \
    $(LOCAL_DIR)/src/postform_config.cpp \
    $(LOCAL_DIR)/src/hal/systick.cpp \
    $(LOCAL_DIR)/src/main.cpp
LOCAL_ARM_ARCHITECTURE := v7-m
LOCAL_ARM_FPU := nofp
LOCAL_COMPILER := arm_clang
LOCAL_STATIC_LIBS := \
    libopencm3_stm32f1 \
    libcortex_m_startup \
    libpostform
include $(BUILD_BINARY)
