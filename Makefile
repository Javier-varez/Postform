BUILD_SYSTEM_DIR := buildsystem
include $(BUILD_SYSTEM_DIR)/top.mk

include $(call all-makefiles-under, .)
