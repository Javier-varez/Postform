cmake_minimum_required(VERSION 3.16)

# Get version number
execute_process(
  COMMAND git describe
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  OUTPUT_STRIP_TRAILING_WHITESPACE
  OUTPUT_VARIABLE POSTFORM_VERSION)
message(VERBOSE "postform version is ${POSTFORM_VERSION}")

# Postform library
add_library(postform STATIC
  src/file_logger.cpp
  src/format_validator.cpp
  src/macros.cpp
  src/platform.cpp
  src/rtt/transport.cpp)

target_link_libraries(postform
  PUBLIC
  Ditto)

target_compile_options(postform
  PRIVATE
    -Wall
    -Wextra
    -Werror
    -std=gnu++17
    -DPOSTFORM_COMMIT_ID=${POSTFORM_VERSION})

# Clang emits a warning for the string literal operator template GNU extension
# We are opting in gnu extensions, so this warning is fine.
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
target_compile_options(postform
  PRIVATE
    -Wno-gnu-string-literal-operator-template)
endif()

target_include_directories(postform
  PUBLIC
    inc)
target_compile_options(postform
  PRIVATE
    -Os)
target_link_options(postform
  INTERFACE
    "SHELL:-Wl,-T ${CMAKE_CURRENT_SOURCE_DIR}/postform.ld")
set_target_properties(postform
  PROPERTIES
  INTERFACE_LINK_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/postform.ld)

if (POSTFORM_BUILD_UNITTESTS MATCHES "true")
  # Postform unit tests
  add_executable(postform_tests
    src/file_logger.cpp
    src/format_validator.cpp
    src/macros.cpp
    src/platform.cpp
    src/rtt/transport.cpp
    test/logger_test.cpp
    test/serial_logger_test.cpp
    test/timestamp_mock.cpp)

  target_link_libraries(postform_tests
    Ditto
    gtest_main
    gmock)

  target_compile_options(postform_tests
    PRIVATE
      -Wall
      -Wextra
      -Werror
      -std=gnu++17
      -DPOSTFORM_COMMIT_ID=${POSTFORM_VERSION})

  # Clang emits a warning for the string literal operator template GNU extension
  # We are opting in gnu extensions, so this warning is fine.
  if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  target_compile_options(postform_tests
    PRIVATE
      -Wno-gnu-string-literal-operator-template)
  endif()

  target_include_directories(postform_tests
    PUBLIC
      inc)
  target_compile_options(postform_tests
    PRIVATE
      -Os)
endif()
