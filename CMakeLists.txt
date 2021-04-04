cmake_minimum_required(VERSION 3.16)

project(Postform)

add_subdirectory(libpostform)
if (POSTFORM_BUILD_EXAMPLES MATCHES "true")
  if (POSTFORM_BUILD_TARGET_APP MATCHES "true")
    include(cmake/cortex-m_hal.cmake)
    include(cmake/cortex-m_startup.cmake)
    include(cmake/libopencm3.cmake)
  endif()
  add_subdirectory(app)
endif()
