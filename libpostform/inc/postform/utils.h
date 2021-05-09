
#ifndef POSTFORM_UTILS_H_
#define POSTFORM_UTILS_H_

#include <cstddef>
#include <cstdint>

#define UNINIT __attribute((section(".uninit")))
#define CLINKAGE extern "C"

// Constexpr utilities

namespace Postform {
[[nodiscard]] constexpr static bool strStartsWith(const char* str,
                                                  const char* pattern) {
  std::size_t i = 0;
  while (pattern[i] != '\0') {
    if (str[i] != pattern[i]) {
      return false;
    }
    i++;
  }
  return true;
}

[[nodiscard]] constexpr std::size_t stringLength(const char* str) {
  std::size_t length = 0;
  while (str[length] != '\0') {
    length++;
  }
  return length;
}
}  // namespace Postform

#endif  // POSTFORM_UTILS_H_
