cmake_minimum_required(VERSION 3.16)

if (POSTFORM_BUILD_TARGET_APP MATCHES "true")
  # Postform format application
  add_executable(postform_format
    src/main.cpp src/postform_config.cpp)

  target_include_directories(postform_format PRIVATE inc)

  target_compile_options(postform_format PRIVATE
    -Os -std=gnu++17)

  # Clang emits a warning for the string literal operator template GNU extension
  # We are opting in gnu extensions, so this warning is fine.
  if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(postform_format
      PRIVATE
        -Wno-gnu-string-literal-operator-template)
  endif()

  target_compile_definitions(postform_format PRIVATE "-DSTM32F1")

  if (triple STREQUAL "thumbv6m-none-eabi")
    # Use polyfilled atomics on thumbv6m
    target_link_libraries(postform_format
      PRIVATE cortex-m_atomics)
  endif()

  target_link_libraries(postform_format
    PRIVATE
      cortex-m_startup
      cortex-m_hal
      postform)

  target_link_options(postform_format
    PRIVATE
      "SHELL:-Wl,-T ${CMAKE_CURRENT_SOURCE_DIR}/memory.ld"
      "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/postform_format.map")

  set_target_properties(postform_format
    PROPERTIES
      LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/memory.ld)
else()
  # Postform host application
  add_executable(postform_test
    src/host_main.cpp)

  target_compile_options(postform_test PRIVATE
    -Os -std=gnu++17)

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
endif()
