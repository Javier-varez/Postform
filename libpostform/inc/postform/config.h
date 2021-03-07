#ifndef POSTFORM_CONFIG_H_
#define POSTFORM_CONFIG_H_

#include <cstdint>

#include "postform/shared_types.hpp"
#include "postform/utils.h"

/**
 * @brief Declares a postform configuration in your application
 *
 * Use this to declare the Postform configuration in your code.
 * This is a required symbol.
 *
 * Multiple definitions of the postform configuration will cause a
 * linker error.
 */
#define DECLARE_POSTFORM_CONFIG(content)                      \
  CLINKAGE __attribute__((section(".postform_config"), used)) \
      const Postform::Config _postform_config {               \
    content                                                   \
  }

#endif  // POSTFORM_CONFIG_H_
