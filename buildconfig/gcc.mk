
CC := arm-none-eabi-gcc
CXX := arm-none-eabi-g++

COMPILER_LDFLAGS := \
    -nostdlib \
    -lc_nano \
    -lstdc++_nano \
    -lm
