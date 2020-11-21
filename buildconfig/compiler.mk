
ARM_GCC_SYSROOT := $(shell arm-none-eabi-gcc -print-sysroot)
ARM_GCC_VERSION := $(shell arm-none-eabi-gcc --version | head -n1 | cut -d ' ' -f 7)

CC := clang
CXX := clang++

COMPILER_TRIPLE := \
    --target=arm-none-eabi

COMPILER_CFLAGS := \
	--sysroot=$(ARM_GCC_SYSROOT) \
    -I$(ARM_GCC_SYSROOT)/include/c++/$(ARM_GCC_VERSION) \
    -I$(ARM_GCC_SYSROOT)/include/c++/$(ARM_GCC_VERSION)/arm-none-eabi/thumb/v7-m/nofp/

COMPILER_LDFLAGS := \
    -nostdlib \
    -L $(ARM_GCC_SYSROOT)/lib/thumb/v7-m/nofp \
    -lc_nano \
    -lstdc++_nano \
    -lm

