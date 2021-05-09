
#include "postform/serial_logger.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "serial_transport_mock.h"
#include "timestamp_mock.h"

namespace Postform {
class SerialLoggerMock : public SerialLogger<SerialTransportMock> {
 public:
  SerialLoggerMock(SerialTransportMock* transport) : SerialLogger(transport) {}
};

using testing::_;
using testing::AnyNumber;
using testing::InSequence;
using testing::Mock;
using testing::Return;
using testing::StrictMock;

class SerialLoggerTest : public testing::Test {
 public:
  void SetUp() { g_timestamp = &timestamp; }
  void TearDown() { g_timestamp = nullptr; }

  auto getWriter() { return serial_logger.getWriter(); }

  StrictMock<TimestampMock> timestamp;
  StrictMock<Postform::SerialTransportMock> transport;
  Postform::SerialLoggerMock serial_logger{&transport};
};

TEST_F(SerialLoggerTest, CanObtainValidWriter) {
  auto writer = getWriter();
  EXPECT_TRUE(writer);
  EXPECT_CALL(transport, write(1));
  EXPECT_CALL(transport, write(0));
  EXPECT_CALL(transport, commit());
}

TEST_F(SerialLoggerTest, CannotObtainTwoValidWriters) {
  InSequence s;
  auto writer = getWriter();
  EXPECT_TRUE(writer);
  auto second_writer = getWriter();
  EXPECT_FALSE(second_writer);
  EXPECT_CALL(transport, write(1));
  EXPECT_CALL(transport, write(0));
  EXPECT_CALL(transport, commit());
}

TEST_F(SerialLoggerTest, WriterRunsCommitOnDestruction) {
  InSequence s;
  auto writer = getWriter();
  EXPECT_TRUE(writer);
  EXPECT_CALL(transport, write(1));
  EXPECT_CALL(transport, write(0));
  EXPECT_CALL(transport, commit());
}

TEST_F(SerialLoggerTest, WriterReleasesItself) {
  InSequence s;
  {
    // First get the writer and release it
    auto writer = getWriter();
    EXPECT_TRUE(writer);
    EXPECT_CALL(transport, write(1));
    EXPECT_CALL(transport, write(0));
    EXPECT_CALL(transport, commit());
  }
  {
    // Now we should be able to get the writer again
    auto writer = getWriter();
    EXPECT_TRUE(writer);
    EXPECT_CALL(transport, write(1));
    EXPECT_CALL(transport, write(0));
    EXPECT_CALL(transport, commit());
  }
}

TEST_F(SerialLoggerTest, DefaultConstructedWriterIsNotValid) {
  SerialWriter<SerialTransportMock> writer;
  EXPECT_FALSE(writer);
}

TEST_F(SerialLoggerTest, CallingCommitOnInvalidWriterDoesNotDoAnything) {
  // TODO(javier-varez): Test rcobs here when implemented
  SerialWriter<SerialTransportMock> writer;
  writer.commit();
}

TEST_F(SerialLoggerTest, CallingWriteOnInvalidWriterDoesNotDoAnything) {
  // TODO(javier-varez): Test rcobs here when implemented
  SerialWriter<SerialTransportMock> writer;
  uint8_t data[] = {
      123,
      213,
      231,
  };
  writer.write(data, 3);
}

TEST_F(SerialLoggerTest, CallingCommitOnWriterReleasesIt) {
  InSequence s;
  auto writer = getWriter();
  EXPECT_TRUE(writer);
  EXPECT_CALL(transport, write(1));
  EXPECT_CALL(transport, write(0));
  EXPECT_CALL(transport, commit());
  writer.commit();
  // Make sure commit gets called up to this point
  Mock::VerifyAndClearExpectations(&transport);

  EXPECT_FALSE(writer);
  // Now we can acquire it again
  auto second_writer = getWriter();
  EXPECT_TRUE(second_writer);
  EXPECT_CALL(transport, write(1));
  EXPECT_CALL(transport, write(0));
  EXPECT_CALL(transport, commit());
}

TEST_F(SerialLoggerTest, CanMoveWriter) {
  // First get the writer and release it
  auto writer = getWriter();
  EXPECT_TRUE(writer);

  // Test the move constructor
  auto second_writer = std::move(writer);
  EXPECT_TRUE(second_writer);
  EXPECT_FALSE(writer);

  // Test the move assignment operator
  writer = std::move(second_writer);
  EXPECT_TRUE(writer);
  EXPECT_FALSE(second_writer);

  // As a result, only a single valid writer should remain, which will call
  // commit
  InSequence s;
  EXPECT_CALL(transport, write(1));
  EXPECT_CALL(transport, write(0));
  EXPECT_CALL(transport, commit());
}

TEST_F(SerialLoggerTest, CanWriteToTransport) {
  SerialWriter<SerialTransportMock> writer = getWriter();

  const uint8_t data[] = {
      123,
      213,
      231,
  };
  InSequence s;
  EXPECT_CALL(transport, write(123));
  EXPECT_CALL(transport, write(213));
  EXPECT_CALL(transport, write(231));
  writer.write(data, 3);

  EXPECT_CALL(transport, write(4));
  EXPECT_CALL(transport, write(0));
  EXPECT_CALL(transport, commit());
}

TEST_F(SerialLoggerTest, WritesWithZeroes) {
  SerialWriter<SerialTransportMock> writer = getWriter();

  const uint8_t data[] = {
      123,
      213,
      0,
      231,
  };
  InSequence s;
  EXPECT_CALL(transport, write(123));
  EXPECT_CALL(transport, write(213));
  EXPECT_CALL(transport, write(3));
  EXPECT_CALL(transport, write(231));
  writer.write(data, 4);

  EXPECT_CALL(transport, write(2));
  EXPECT_CALL(transport, write(0));
  EXPECT_CALL(transport, commit());
}

TEST_F(SerialLoggerTest, InsertsDummyZeroAfter254NonZeroElements) {
  SerialWriter<SerialTransportMock> writer = getWriter();

  const uint8_t data[] = {
      123,
      213,
      0,
      231,
  };
  InSequence s;
  EXPECT_CALL(transport, write(123));
  EXPECT_CALL(transport, write(213));
  EXPECT_CALL(transport, write(3));
  EXPECT_CALL(transport, write(231));
  writer.write(data, 4);

  const uint8_t dummy_zero[] = {0};
  EXPECT_CALL(transport, write(2));
  writer.write(dummy_zero, 1);

  const uint8_t dummy_one[] = {1};
  for (uint32_t i = 0; i < 254; i++) {
    EXPECT_CALL(transport, write(1));
    writer.write(dummy_one, 1);
  }
  EXPECT_CALL(transport, write(255));  // This is a dummy zero
  EXPECT_CALL(transport, write(1));
  writer.write(dummy_one, 1);

  EXPECT_CALL(transport, write(2));
  EXPECT_CALL(transport, write(0));
  EXPECT_CALL(transport, commit());
}

}  // namespace Postform
