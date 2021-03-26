cmake_minimum_required(VERSION 3.16)

project(Postform)

# Get version number
execute_process(
  COMMAND git describe
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_STRIP_TRAILING_WHITESPACE
  OUTPUT_VARIABLE POSTFORM_VERSION)
message(VERBOSE "postform version is ${POSTFORM_VERSION}")

# Postform library
add_library(postform STATIC
  libpostform/src/file_logger.cpp
  libpostform/src/format_validator.cpp
  libpostform/src/macros.cpp
  libpostform/src/platform.cpp
  libpostform/src/rtt/cobs_writer.cpp
  libpostform/src/rtt/raw_writer.cpp
  libpostform/src/rtt/rtt_manager.cpp)

target_compile_features(postform
  PUBLIC
    cxx_std_17)
target_compile_options(postform
  PRIVATE
    -Wall
    -Wextra
    -Werror
    -DPOSTFORM_COMMIT_ID=${POSTFORM_VERSION})

# Clang emits a warning for tthe string literal operator template GNU extension
# We are opting in gnu extensions, so this warning is fine.
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
target_compile_options(postform
  PRIVATE
    -Wno-gnu-string-literal-operator-template)
endif()

target_include_directories(postform
  PUBLIC
    libpostform/inc)
target_compile_options(postform
  PRIVATE
    -Os)
target_link_options(postform 
  INTERFACE
    "SHELL:-Wl,-T ${CMAKE_SOURCE_DIR}/libpostform/postform.ld")
set_target_properties(postform
  PROPERTIES
  INTERFACE_LINK_DEPENDS
  ${CMAKE_SOURCE_DIR}/libpostform/postform.ld)

# Postform test application
add_executable(postform_test 
  app/src/host_main.cpp)
target_compile_options(postform_test
  PRIVATE
    -Os)

# Clang emits a warning for tthe string literal operator template GNU extension
# We are opting in gnu extensions, so this warning is fine.
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
target_compile_options(postform_test
  PRIVATE
    -Wno-gnu-string-literal-operator-template)
endif()

target_link_libraries(postform_test
  PRIVATE
    postform)
target_link_options(postform_test
  PRIVATE
    "SHELL:-Wl,-T ${CMAKE_SOURCE_DIR}/app/host_ld.x"
    "-Wl,-Map=${CMAKE_BINARY_DIR}/postform_test.map")
set_target_properties(postform_test
  PROPERTIES
    LINK_DEPENDS ${CMAKE_SOURCE_DIR}/app/host_ld.x)
