LOCAL_DIR := $(call current-dir)

include $(CLEAR_VARS)
LOCAL_NAME := postform
LOCAL_CFLAGS := \
    $(TARGET_CFLAGS) \
    -I$(LOCAL_DIR)/inc \
    -Os \
    -g3 \
    -Wall \
    -Werror \
    -Wextra \
    -Wno-gnu-string-literal-operator-template \
	-ffunction-sections \
    -fdata-sections
LOCAL_CXXFLAGS := \
    $(LOCAL_CFLAGS) \
    -std=gnu++17 \
    -fno-exceptions \
    -fno-rtti
LOCAL_SRC := \
    $(LOCAL_DIR)/src/rtt_logger.cpp \
    $(LOCAL_DIR)/src/rtt/rtt_manager.cpp \
    $(LOCAL_DIR)/src/rtt/raw_writer.cpp \
    $(LOCAL_DIR)/src/rtt/cobs_writer.cpp \
    $(LOCAL_DIR)/src/format_validator.cpp \
    $(LOCAL_DIR)/src/macros.cpp \
    $(LOCAL_DIR)/src/platform.cpp
LOCAL_ARM_ARCHITECTURE := v7-m
LOCAL_ARM_FPU := nofp
LOCAL_COMPILER := arm_clang
LOCAL_ARFLAGS := -rcs
LOCAL_EXPORTED_DIRS := \
	$(LOCAL_DIR)/inc
LOCAL_LINKER_FILE := \
    $(LOCAL_DIR)/postform.ld
include $(BUILD_STATIC_LIB)
