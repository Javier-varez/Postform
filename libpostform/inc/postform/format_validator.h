
#ifndef POSTFORM_FORMAT_VALIDATOR_H_
#define POSTFORM_FORMAT_VALIDATOR_H_

#include <array>
#include <cstdint>
#include <type_traits>

#include "postform/macros.h"
#include "postform/types.h"
#include "postform/utils.h"

namespace Postform {

struct SizeSpecHandler {
  const char* size_spec;
  bool (*matcher)(std::size_t size);
};

constexpr static SizeSpecHandler default_size_handler{
    "", [](std::size_t size [[maybe_unused]]) { return true; }};

constexpr static std::array<SizeSpecHandler, 5> integer_size_handlers = {
    SizeSpecHandler{"", [](std::size_t size) { return size == sizeof(int); }},
    SizeSpecHandler{"l",
                    [](std::size_t size) { return size == sizeof(long int); }},
    SizeSpecHandler{
        "ll", [](std::size_t size) { return size == sizeof(long long int); }},
    SizeSpecHandler{"hh",
                    [](std::size_t size) { return size == sizeof(char); }},
    SizeSpecHandler{"h",
                    [](std::size_t size) { return size == sizeof(short); }},
};

struct SizeSpecHandlers {
  const SizeSpecHandler* handlers = &default_size_handler;
  uint32_t num = 1;
};

struct FormatSpecHandler {
  const SizeSpecHandlers size_handlers;
  const char* format_spec;
  bool (*matcher)();
};

template <class T>
[[nodiscard]] constexpr static bool formatValidatorSingleArgument(
    const char* fmt, [[maybe_unused]] T arg, std::size_t* position) {
  // This array needs to be defined inside the template in order to have
  // visibility of T.
  constexpr std::array<FormatSpecHandler, 8> format_spec_handlers = {
      FormatSpecHandler{SizeSpecHandlers{}, "s",
                        []() { return std::is_convertible_v<T, const char*>; }},
      FormatSpecHandler{
          SizeSpecHandlers{integer_size_handlers.data(),
                           integer_size_handlers.size()},
          "d", []() { return std::is_integral_v<T> && std::is_signed_v<T>; }},
      FormatSpecHandler{
          SizeSpecHandlers{integer_size_handlers.data(),
                           integer_size_handlers.size()},
          "i", []() { return std::is_integral_v<T> && std::is_signed_v<T>; }},
      FormatSpecHandler{
          SizeSpecHandlers{integer_size_handlers.data(),
                           integer_size_handlers.size()},
          "u", []() { return std::is_integral_v<T> && std::is_unsigned_v<T>; }},
      FormatSpecHandler{SizeSpecHandlers{integer_size_handlers.data(),
                                         integer_size_handlers.size()},
                        "o", []() { return std::is_integral_v<T>; }},
      FormatSpecHandler{SizeSpecHandlers{integer_size_handlers.data(),
                                         integer_size_handlers.size()},
                        "x", []() { return std::is_integral_v<T>; }},
      FormatSpecHandler{SizeSpecHandlers{}, "p",
                        []() { return std::is_pointer_v<T>; }},
      FormatSpecHandler{
          SizeSpecHandlers{}, "k",
          []() { return std::is_same_v<T, Postform::InternedString>; }},
  };

  /// Common code for handling all supported format_specifiers
  std::size_t i = 0;
  while (fmt[i] != '\0') {
    if (fmt[i++] == '%') {
      if (fmt[i] == '%') {
        i++;
        continue;
      }

      for (const auto& format_spec_handler : format_spec_handlers) {
        for (uint32_t size_index = 0;
             size_index < format_spec_handler.size_handlers.num; size_index++) {
          const SizeSpecHandler* size_handler =
              &format_spec_handler.size_handlers.handlers[size_index];
          if (!strStartsWith(&fmt[i], size_handler->size_spec)) {
            continue;
          }

          auto size_handler_spec_size = stringLength(size_handler->size_spec);

          if (strStartsWith(&fmt[i + size_handler_spec_size],
                            format_spec_handler.format_spec)) {
            if (position != nullptr)
              *position = i + size_handler_spec_size +
                          stringLength(format_spec_handler.format_spec);
            return format_spec_handler.matcher() &&
                   size_handler->matcher(sizeof(T));
          }
        }
      }

      // some other unhandled format specifier was found!
      return false;
    }
  }
  if (position != nullptr) *position = i;
  return false;
}

[[nodiscard]] constexpr static bool formatValidator(const char* fmt) {
  std::size_t i = 0;
  while (fmt[i] != '\0') {
    if (fmt[i] == '%' && fmt[++i] != '%') {
      return false;
    }
    i++;
  }
  return true;
}

template <class T, class... U>
[[nodiscard]] constexpr static bool formatValidator(
    const char* fmt, [[maybe_unused]] T arg, [[maybe_unused]] U... args) {
  std::size_t position = 0;
  if (formatValidatorSingleArgument(fmt, arg, &position)) {
    return formatValidator(&fmt[position], args...);
  }
  return false;
}

template <class T>
struct ConstexprStaticInstance {
  constexpr static std::decay_t<T> value{};
};
}  // namespace Postform

#define POSTFORM_FORMAT_DEFAULT_ARGS_0()
#define POSTFORM_FORMAT_DEFAULT_ARGS_1(arg) \
  Postform::ConstexprStaticInstance<decltype(arg)>::value
#define POSTFORM_FORMAT_DEFAULT_ARGS_2(arg, ...) \
  POSTFORM_FORMAT_DEFAULT_ARGS_1(arg),           \
      POSTFORM_FORMAT_DEFAULT_ARGS_1(__VA_ARGS__)
#define POSTFORM_FORMAT_DEFAULT_ARGS_3(arg, ...) \
  POSTFORM_FORMAT_DEFAULT_ARGS_1(arg),           \
      POSTFORM_FORMAT_DEFAULT_ARGS_2(__VA_ARGS__)
#define POSTFORM_FORMAT_DEFAULT_ARGS_4(arg, ...) \
  POSTFORM_FORMAT_DEFAULT_ARGS_1(arg),           \
      POSTFORM_FORMAT_DEFAULT_ARGS_3(__VA_ARGS__)
#define POSTFORM_FORMAT_DEFAULT_ARGS_5(arg, ...) \
  POSTFORM_FORMAT_DEFAULT_ARGS_1(arg),           \
      POSTFORM_FORMAT_DEFAULT_ARGS_4(__VA_ARGS__)
#define POSTFORM_FORMAT_DEFAULT_ARGS_6(arg, ...) \
  POSTFORM_FORMAT_DEFAULT_ARGS_1(arg),           \
      POSTFORM_FORMAT_DEFAULT_ARGS_5(__VA_ARGS__)
#define POSTFORM_FORMAT_DEFAULT_ARGS_7(arg, ...) \
  POSTFORM_FORMAT_DEFAULT_ARGS_1(arg),           \
      POSTFORM_FORMAT_DEFAULT_ARGS_6(__VA_ARGS__)
#define POSTFORM_FORMAT_DEFAULT_ARGS_8(arg, ...) \
  POSTFORM_FORMAT_DEFAULT_ARGS_1(arg),           \
      POSTFORM_FORMAT_DEFAULT_ARGS_7(__VA_ARGS__)
#define POSTFORM_FORMAT_DEFAULT_ARGS_9(arg, ...) \
  POSTFORM_FORMAT_DEFAULT_ARGS_1(arg),           \
      POSTFORM_FORMAT_DEFAULT_ARGS_8(__VA_ARGS__)
#define POSTFORM_FORMAT_DEFAULT_ARGS_10(arg, ...) \
  POSTFORM_FORMAT_DEFAULT_ARGS_1(arg),            \
      POSTFORM_FORMAT_DEFAULT_ARGS_9(__VA_ARGS__)
#define POSTFORM_FORMAT_DEFAULT_ARGS_11(arg, ...) \
  POSTFORM_FORMAT_DEFAULT_ARGS_1(arg),            \
      POSTFORM_FORMAT_DEFAULT_ARGS_10(__VA_ARGS__)
#define POSTFORM_FORMAT_DEFAULT_ARGS_12(arg, ...) \
  POSTFORM_FORMAT_DEFAULT_ARGS_1(arg),            \
      POSTFORM_FORMAT_DEFAULT_ARGS_11(__VA_ARGS__)
#define POSTFORM_FORMAT_DEFAULT_ARGS_13(arg, ...) \
  POSTFORM_FORMAT_DEFAULT_ARGS_1(arg),            \
      POSTFORM_FORMAT_DEFAULT_ARGS_12(__VA_ARGS__)
#define POSTFORM_FORMAT_DEFAULT_ARGS_14(arg, ...) \
  POSTFORM_FORMAT_DEFAULT_ARGS_1(arg),            \
      POSTFORM_FORMAT_DEFAULT_ARGS_13(__VA_ARGS__)
#define POSTFORM_FORMAT_DEFAULT_ARGS_15(arg, ...) \
  POSTFORM_FORMAT_DEFAULT_ARGS_1(arg),            \
      POSTFORM_FORMAT_DEFAULT_ARGS_14(__VA_ARGS__)

#define POSTFORM_VALIDATE_FORMAT(fmt, ...)                     \
  Postform::formatValidator(fmt POSTFORM_EXPAND_COMMA(         \
      __VA_ARGS__) POSTFORM_CAT(POSTFORM_FORMAT_DEFAULT_ARGS_, \
                                POSTFORM_NARG(__VA_ARGS__)(__VA_ARGS__)))

#define POSTFORM_ASSERT_FORMAT(fmt, ...)                      \
  static_assert(POSTFORM_VALIDATE_FORMAT(fmt, ##__VA_ARGS__), \
                "Format string does not match arguments")

#endif
