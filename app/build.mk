LOCAL_DIR := $(call current-dir)

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
    -Ilibopencm3/include \
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
    -lnosys \
    -Llibopencm3/lib \
    -lopencm3_stm32f1
LOCAL_LINKER_FILE := \
    $(LOCAL_DIR)/gcc.ld
LOCAL_SRC := \
    $(LOCAL_DIR)/src/startup.cpp \
    $(LOCAL_DIR)/src/postform_config.cpp \
    $(LOCAL_DIR)/src/hal/systick.cpp \
    $(LOCAL_DIR)/src/main.cpp
LOCAL_ARM_ARCHITECTURE := v7-m
LOCAL_ARM_FPU := nofp
LOCAL_COMPILER := arm_clang
LOCAL_STATIC_LIBS := libpostform
include $(BUILD_BINARY)

$(LOCAL_TARGET): libopencm3/lib/libopencm3_stm32f1.a

libopencm3/lib/libopencm3_stm32f1.a:
	$(call print-build-header, libopencm3, MAKE $(notdir $@))
	$(SILENT)$(MAKE) -C libopencm3 TARGETS=stm32/f1 > /dev/null

clean_libopencm3:
	$(SILENT)$(MAKE) -C libopencm3 clean > /dev/null
.PHONY: clean_libopencm3

clean: clean_libopencm3
