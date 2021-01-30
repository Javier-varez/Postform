
#include "postform/format_validator.h"

// Compile-time tests for the POSTFORM_VALIDATE_FORMAT
static_assert(POSTFORM_VALIDATE_FORMAT("%u %d", 2u, 1));
static_assert(POSTFORM_VALIDATE_FORMAT("%s", ""));
static_assert(POSTFORM_VALIDATE_FORMAT("%d", 2));
static_assert(!POSTFORM_VALIDATE_FORMAT("%d", (const char*)123ull));
static_assert(!POSTFORM_VALIDATE_FORMAT("%s", 123ull));
static_assert(POSTFORM_VALIDATE_FORMAT("%s %u %u, %s", (const char*)123ull, 1ull, 1ull, ""));
static_assert(POSTFORM_VALIDATE_FORMAT("%s %s %d, %u", "", (const char*)123ull, 2ll, 12ull));
static_assert(POSTFORM_VALIDATE_FORMAT("fsdgfds%%"));
static_assert(!POSTFORM_VALIDATE_FORMAT("fsdgfds%s"));
static_assert(!POSTFORM_VALIDATE_FORMAT("fsdgfds%a"));
static_assert(POSTFORM_VALIDATE_FORMAT("%x", 12));

// Compile-time tests for the POSTFORM_ASSERT_FORMAT
POSTFORM_ASSERT_FORMAT("%u %u", 2u, 1u);
POSTFORM_ASSERT_FORMAT("%s", "random_str");
POSTFORM_ASSERT_FORMAT("%d", 1);
POSTFORM_ASSERT_FORMAT("%s %d %u, %s", "", 1, 1u, "");
POSTFORM_ASSERT_FORMAT("%s %s %u, %d", "", "", 1u, 1);
POSTFORM_ASSERT_FORMAT("fsdgfds%%%%");

// Compile-time tests for the POSTFORM_EXPAND_COMMA macro
static_assert(true POSTFORM_EXPAND_COMMA(1) "Comma wasn't expanded!");
static_assert(true POSTFORM_EXPAND_COMMA());
