
#ifndef SERIAL_TRANSPORT_MOCK_H_
#define SERIAL_TRANSPORT_MOCK_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace Postform {
class SerialTransportMock {
 public:
  MOCK_METHOD(bool, write, (const std::uint8_t*, std::uint32_t), ());
  MOCK_METHOD(void, commit, (), ());
};
}  // namespace Postform

#endif  // SERIAL_TRANSPORT_MOCK_H_
