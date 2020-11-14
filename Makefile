BUILD_SYSTEM_DIR := buildsystem
include $(BUILD_SYSTEM_DIR)/top.mk
include buildconfig/compiler.mk

include $(call all-makefiles-under, .)
