LOCAL_DIR := $(call current-dir)

include $(CLEAR_VARS)
LOCAL_NAME := format
TARGET_CFLAGS := \
    $(COMPILER_TRIPLE) \
    -mcpu=cortex-m3 \
    -mfloat-abi=soft \
    -mthumb
LOCAL_CFLAGS := \
    $(COMPILER_CFLAGS) \
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
    -fno-exceptions \
    -fno-rtti \
    -ffunction-sections \
    -fdata-sections
LOCAL_LDFLAGS := \
    $(COMPILER_LDFLAGS) \
    -Wl,--gc-sections \
    -Llibopencm3/lib \
    -lopencm3_stm32f1
LOCAL_LINKER_FILE := \
    $(LOCAL_DIR)/gcc.ld
LOCAL_SRC := \
    $(LOCAL_DIR)/src/startup.cpp \
    $(LOCAL_DIR)/src/rtt_logger.cpp \
    $(LOCAL_DIR)/src/rtt/rtt.cpp \
    $(LOCAL_DIR)/src/rtt/raw_writer.cpp \
    $(LOCAL_DIR)/src/rtt/cobs_writer.cpp \
    $(LOCAL_DIR)/src/hal/systick.cpp \
    $(LOCAL_DIR)/src/main.cpp
include $(BUILD_BINARY)

$(LOCAL_TARGET): libopencm3/lib/libopencm3_stm32f1.a

libopencm3/lib/libopencm3_stm32f1.a:
	$(call print-build-header, libopencm3, MAKE $(notdir $@))
	$(SILENT)$(MAKE) -C libopencm3 TARGETS=stm32/f1 > /dev/null

clean_libopencm3:
	$(SILENT)$(MAKE) -C libopencm3 clean > /dev/null
.PHONY: clean_libopencm3

clean: clean_libopencm3