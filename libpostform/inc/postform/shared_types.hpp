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

}  // namespace Postform

#endif  // POSTFORM_SHARED_TYPES_H_
