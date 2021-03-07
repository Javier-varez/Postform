
#include "postform/macros.h"
#include "postform/shared_types.hpp"
#include "postform/utils.h"

CLINKAGE __attribute__((
    section(".postform_version"))) volatile const char _postform_version[] =
    __POSTFORM_EXPAND_AND_STRINGIFY(POSTFORM_COMMIT_ID);

namespace Postform {
volatile uint32_t dummy;
}  // namespace Postform
