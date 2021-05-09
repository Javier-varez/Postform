
#ifndef TIMESTAMP_MOCK_H_
#define TIMESTAMP_MOCK_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>

class TimestampMock {
 public:
  MOCK_METHOD(uint64_t, getGlobalTimestamp, (), ());
};

extern TimestampMock* g_timestamp;

#endif  // TIMESTAMP_MOCK_H_
