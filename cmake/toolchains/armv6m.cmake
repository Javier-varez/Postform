set(CMAKE_SYSTEM_NAME, Generic)
set(CMAKE_SYSTEM_PROCESSOR, arm)

set(triple thumbv6m-none-eabi)
set(ARM_TARGET_CONFIG "-mcpu=cortex-m0plus -mfloat-abi=soft -mthumb")
set(CMAKE_C_COMPILER clang)
set(CMAKE_C_COMPILER_TARGET ${triple})
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_COMPILER_TARGET ${triple})

# Sysroot
execute_process(COMMAND arm-none-eabi-gcc -print-sysroot
  OUTPUT_VARIABLE gcc_arm_sysroot
  OUTPUT_STRIP_TRAILING_WHITESPACE)

# Gcc version
execute_process(COMMAND arm-none-eabi-gcc -dumpversion
  OUTPUT_VARIABLE gcc_arm_version
  OUTPUT_STRIP_TRAILING_WHITESPACE)

# Appropriate multilib directory
execute_process(COMMAND arm-none-eabi-gcc -mcpu=cortex-m0plus -mfloat-abi=soft -mthumb -print-multi-directory
  OUTPUT_VARIABLE gcc_arm_multi_dir
  OUTPUT_STRIP_TRAILING_WHITESPACE)

# libgcc location
execute_process(COMMAND arm-none-eabi-gcc -mcpu=cortex-m0plus -mfloat-abi=soft -mthumb -print-libgcc-file-name
  OUTPUT_VARIABLE lib_gcc_file
  OUTPUT_STRIP_TRAILING_WHITESPACE)
get_filename_component(lib_gcc_dir ${lib_gcc_file} DIRECTORY)

include_directories(
  ${gcc_arm_sysroot}/include/c++/${gcc_arm_version}
  ${gcc_arm_sysroot}/include/c++/${gcc_arm_version}/arm-none-eabi/${gcc_arm_multi_dir}
  ${gcc_arm_sysroot}/include)

set(arm_flags "${ARM_TARGET_CONFIG} -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections")
set(CMAKE_C_FLAGS "${arm_flags} -DARMV6_ARCH")
set(CMAKE_CXX_FLAGS "${arm_flags} -DARMV6_ARCH")

link_directories(
  ${gcc_arm_sysroot}/lib/${gcc_arm_multi_dir}
  ${lib_gcc_dir})
link_libraries(-nostdlib -lnosys -lc_nano -lstdc++_nano -lgcc)

set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
