BUILD_SYSTEM_DIR := buildsystem
include $(BUILD_SYSTEM_DIR)/top.mk

TARGET_CFLAGS := \
    -mcpu=cortex-m3 \
    -mfloat-abi=soft \
    -mthumb

include $(call all-makefiles-under, .)
