
#include "postform/macros.h"

// Compile-time tests for the POSTFORM_NARG macro
// This can fail if the gnu extensions are not enabled.
// Make sure to build with -std=gnu++17
static_assert(POSTFORM_NARG() == 0, "Make sure to enable gnu extensions");
static_assert(POSTFORM_NARG(1) == 1);
static_assert(POSTFORM_NARG(1, 2) == 2);
static_assert(POSTFORM_NARG(1, 2, 3) == 3);
static_assert(POSTFORM_NARG(1, 2, 3, 4) == 4);
static_assert(POSTFORM_NARG(1, 2, 3, 4, 5) == 5);
static_assert(POSTFORM_NARG(1, 2, 3, 4, 5, 6) == 6);
static_assert(POSTFORM_NARG(1, 2, 3, 4, 5, 6, 7) == 7);
static_assert(POSTFORM_NARG(1, 2, 3, 4, 5, 6, 7, 8) == 8);
static_assert(POSTFORM_NARG(1, 2, 3, 4, 5, 6, 7, 8, 9) == 9);
static_assert(POSTFORM_NARG(1, 2, 3, 4, 5, 6, 7, 8, 9, 10) == 10);
static_assert(POSTFORM_NARG(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11) == 11);
static_assert(POSTFORM_NARG(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12) == 12);
static_assert(POSTFORM_NARG(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13) == 13);
static_assert(POSTFORM_NARG(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14) ==
              14);
