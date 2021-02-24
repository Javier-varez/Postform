
#include "postform/utils.h"
#include "postform/macros.h"
#include "postform/shared_types.hpp"

CLINKAGE __attribute__((section(".postform_platform_descriptors")))
volatile const Postform::PlatformDescription _postform_platform_description = Postform::PlatformDescription {};

CLINKAGE __attribute__((section(".postform_version")))
volatile const char _postform_version[] = __POSTFORM_EXPAND_AND_STRINGIFY(POSTFORM_COMMIT_ID);

namespace Postform {
  volatile uint32_t dummy;
}  // namespace Postform

