#ifndef POSTFORM_CONFIG_H_
#define POSTFORM_CONFIG_H_

#include <cstdint>

#include "postform/utils.h"

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

/**
 * @brief Declares a postform configuration in your application
 *
 * Use this to declare the Postform configuration in your code.
 * This is a required symbol.
 *
 * Multiple definitions of the postform configuration will cause a
 * linker error.
 */
#define DECLARE_POSTFORM_CONFIG(content) \
  CLINKAGE __attribute__((section(".postform_config"))) \
  const Postform::Config _postform_config { content }


#endif  // POSTFORM_CONFIG_H_
