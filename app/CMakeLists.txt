cmake_minimum_required(VERSION 3.16)

# Postform test application
add_executable(postform_test 
  src/host_main.cpp)
target_compile_options(postform_test
  PRIVATE
    -Os)

# Clang emits a warning for the string literal operator template GNU extension
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
    "SHELL:-Wl,-T ${CMAKE_CURRENT_SOURCE_DIR}/host.ld"
    "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/postform_test.map")
set_target_properties(postform_test
  PROPERTIES
    LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/host.ld)
