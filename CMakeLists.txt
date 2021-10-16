cmake_minimum_required(VERSION 3.16)

project(Postform)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

add_subdirectory(libpostform)

include(cmake/ditto.cmake)

if (POSTFORM_BUILD_EXAMPLES MATCHES "true")
  if (POSTFORM_BUILD_TARGET_APP MATCHES "true")
    include(cmake/cortex-m_hal.cmake)
    include(cmake/cortex-m_startup.cmake)
    if (triple STREQUAL "thumbv6m-none-eabi")
      include(cmake/cortex-m_atomics.cmake)
    endif()
  endif()
  add_subdirectory(app)
endif()

if (POSTFORM_BUILD_UNITTESTS MATCHES "true")
  include(cmake/googletest.cmake)
endif()
