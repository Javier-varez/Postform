
ifneq ($(USE_GCC),)
include buildconfig/gcc.mk
else
include buildconfig/clang.mk
endif
