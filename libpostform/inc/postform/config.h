#ifndef POSTFORM_CONFIG_H_
#define POSTFORM_CONFIG_H_

#include <cstdint>

#include "postform/utils.h"

namespace Postform {
struct Config {
  const uint32_t timestamp_frequency;
};
}  // namespace Postform

// Use this to declare the Postform configuration in your code.
// This is a required symbol.
#define DECLARE_POSTFORM_CONFIG(content) \
  CLINKAGE __attribute__((section(".postform_config"))) \
  const Postform::Config _postform_config { content }


#endif  // POSTFORM_CONFIG_H_
