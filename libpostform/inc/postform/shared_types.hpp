#ifndef POSTFORM_SHARED_TYPES_H_
#define POSTFORM_SHARED_TYPES_H_

#include <cstdint>

namespace Postform {
/**
 * @brief Postform configuration structure.
 *
 * An instance of the configuration must be present in the
 * ".postform_config" with the symbol name _postform_config.
 */
struct Config {
  const uint32_t timestamp_frequency;
};

/**
 * @brief Platform description that allows to get information about
 *        multiple data types for the platform
 */
struct PlatformDescription {
    uint32_t char_size = sizeof(char);
    uint32_t short_size = sizeof(short);
    uint32_t int_size = sizeof(int);
    uint32_t long_int_size = sizeof(long int);
    uint32_t long_long_int_size = sizeof(long long int);
    uint32_t ptr_size = sizeof(void*);
};
}  // namespace Postform

#endif  // POSTFORM_SHARED_TYPES_H_

