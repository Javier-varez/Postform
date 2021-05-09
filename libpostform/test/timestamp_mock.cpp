
#include "timestamp_mock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

TimestampMock* g_timestamp = nullptr;

namespace Postform {
uint64_t getGlobalTimestamp() { return g_timestamp->getGlobalTimestamp(); }
}  // namespace Postform
