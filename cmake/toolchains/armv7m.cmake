
set(CMAKE_SYSTEM_NAME, none)
set(CMAKE_SYSTEM_PROCESSOR, arm)

set(triple arm-none-eabi)
set(CMAKE_C_COMPILER clang)
set(CMAKE_C_COMPILER_TARGET ${triple})
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_COMPILER_TARGET ${triple})

execute_process(COMMAND arm-none-eabi-gcc -print-sysroot
  OUTPUT_VARIABLE gcc_arm_sysroot
  OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(COMMAND arm-none-eabi-gcc -dumpversion
  OUTPUT_VARIABLE gcc_arm_version
  OUTPUT_STRIP_TRAILING_WHITESPACE)

set(CMAKE_SYSROOT ${gcc_arm_sysroot})

include_directories(
  ${gcc_arm_sysroot}/include/c++/${gcc_arm_version}
  ${gcc_arm_sysroot}/include/c++/${gcc_arm_version}/arm-none-eabi/thumb/v7-m/nofp)

set(arm_flags "-mcpu=cortex-m3 -mfloat-abi=soft -mthumb -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections")
set(CMAKE_C_FLAGS ${arm_flags})
set(CMAKE_CXX_FLAGS ${arm_flags})

link_directories(${gcc_arm_sysroot}/lib/thumb/v7-m/nofp)
link_libraries(-nostdlib -lnosys -lc_nano -lstdc++_nano)
set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

