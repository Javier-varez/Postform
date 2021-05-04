#ifndef POSTFORM_ARGS_H_
#define POSTFORM_ARGS_H_

#include <postform/types.h>

#include <array>
#include <type_traits>

namespace Postform {

class Argument {
 public:
  const union {
    unsigned long long unsigned_long_long;
    signed long long signed_long_long;
    const char* str_ptr;
    const void* void_ptr;
    InternedString interned_string;
  };

  const enum class Type {
    UNSIGNED_INTEGER,
    SIGNED_INTEGER,
    STRING_POINTER,
    VOID_PTR,
    INTERNED_STRING
  } type;
};

template <
    class T,
    std::enable_if_t<std::is_integral_v<T> && std::is_signed_v<T>, bool> = true>
constexpr Argument make_arg(T value) {
  return Argument{.signed_long_long = value,
                  .type = Argument::Type::SIGNED_INTEGER};
}

template <class T,
          std::enable_if_t<std::is_integral_v<T> && std::is_unsigned_v<T>,
                           bool> = true>
constexpr Argument make_arg(T value) {
  return Argument{
      .unsigned_long_long = value,
      .type = Argument::Type::UNSIGNED_INTEGER,
  };
}

template <class T,
          std::enable_if_t<std::is_convertible_v<T, const char*>, bool> = true>
constexpr Argument make_arg(T value) {
  return Argument{.str_ptr = value, .type = Argument::Type::STRING_POINTER};
}

template <class T, std::enable_if_t<!std::is_convertible_v<T, const char*> &&
                                        std::is_convertible_v<T, const void*>,
                                    bool> = true>
constexpr Argument make_arg(T value) {
  return Argument{.void_ptr = value, .type = Argument::Type::VOID_PTR};
}

constexpr Argument make_arg(InternedString value) {
  return Argument{.interned_string = value,
                  .type = Argument::Type::INTERNED_STRING};
}

template <class... T>
constexpr std::array<Argument, sizeof...(T)> build_args(T... args) {
  return {make_arg(args)...};
}

}  // namespace Postform

#endif  // POSTFORM_ARGS_H_
