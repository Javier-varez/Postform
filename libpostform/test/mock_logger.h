#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "postform/logger.h"

class MockWriter {
 public:
  MOCK_METHOD(void, write, (const uint8_t*, size_t), ());
};

class MockLogger : public Postform::Logger<MockLogger, MockWriter> {
  MockWriter getWriter() { return MockWriter{}; }
};
