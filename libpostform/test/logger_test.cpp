#include "postform/logger.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <type_traits>
#include <variant>

#include "mock_logger.h"

using ::testing::_;
using ::testing::ElementsAreArray;
using ::testing::StrictMock;
using ::testing::TestWithParam;
using ::testing::Values;

namespace Postform {

struct Leb128Params {
  std::variant<int64_t, uint64_t> input;
  std::vector<uint8_t> expectation;
};

class LoggerTest : public TestWithParam<Leb128Params> {
 public:
  template <class T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
  void writeLeb128(T value) {
    logger.writeLeb128(&writer, value);
  }

  MockLogger logger;
  StrictMock<MockWriter> writer;
};

TEST_P(LoggerTest, Leb128) {
  auto param = GetParam();

  EXPECT_CALL(writer, write(_, _)).With(ElementsAreArray(param.expectation));
  if (std::holds_alternative<uint64_t>(param.input)) {
    writeLeb128(std::get<uint64_t>(param.input));
  } else if (std::holds_alternative<int64_t>(param.input)) {
    writeLeb128(std::get<int64_t>(param.input));
  }
}

INSTANTIATE_TEST_SUITE_P(
    TestLeb128, LoggerTest,
    Values(Leb128Params{std::variant<int64_t, uint64_t>(uint64_t{0}),
                        std::vector<uint8_t>{0U}},
           Leb128Params{std::variant<int64_t, uint64_t>(uint64_t{0x7FU}),
                        std::vector<uint8_t>{0x7F}},
           Leb128Params{std::variant<int64_t, uint64_t>(uint64_t{0xFFU}),
                        std::vector<uint8_t>{0xFF, 0x01}},
           Leb128Params{std::variant<int64_t, uint64_t>(uint64_t{0xA55AU}),
                        std::vector<uint8_t>{0xDA, 0xCA, 0x02}},
           Leb128Params{
               std::variant<int64_t, uint64_t>(uint64_t{0xFFFFFFFFFFFFFFFF}),
               std::vector<uint8_t>{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                    0xFF, 0xFF, 0x01}},
           Leb128Params{std::variant<int64_t, uint64_t>(int64_t{0}),
                        std::vector<uint8_t>{0U}},
           Leb128Params{std::variant<int64_t, uint64_t>(int64_t{-1}),
                        std::vector<uint8_t>{0x7F}},
           Leb128Params{std::variant<int64_t, uint64_t>(int64_t{-256}),
                        std::vector<uint8_t>{0x80, 0x7E}},
           Leb128Params{std::variant<int64_t, uint64_t>(int64_t{-257}),
                        std::vector<uint8_t>{0xFF, 0x7D}},
           Leb128Params{std::variant<int64_t, uint64_t>(int64_t{-255}),
                        std::vector<uint8_t>{0x81, 0x7E}}));

}  // namespace Postform
