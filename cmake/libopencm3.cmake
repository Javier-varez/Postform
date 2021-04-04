
include(ExternalProject)

find_program(MAKE_EXE NAMES gmake nmake make)
ExternalProject_Add(libopencm3_connect
  GIT_REPOSITORY    https://github.com/libopencm3/libopencm3
  GIT_TAG           8435287300e5ca9af9f889c529e7b1fa019c42fb
  CONFIGURE_COMMAND ""
  BUILD_COMMAND     ${MAKE_EXE} TARGETS=stm32/f1 -j
  BUILD_IN_SOURCE   true
  INSTALL_COMMAND   ""
)

ExternalProject_Get_Property(libopencm3_connect SOURCE_DIR)

add_library(libopencm3 STATIC IMPORTED GLOBAL)
add_dependencies(libopencm3 libopencm3_connect)

set_target_properties(libopencm3 
  PROPERTIES IMPORTED_LOCATION 
    ${SOURCE_DIR}/lib/libopencm3_stm32f1.a
)
include_directories(${SOURCE_DIR}/include)

message("Source dir of myExtProj = ${SOURCE_DIR}")

