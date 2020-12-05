
#ifndef RTT_RTT_H_
#define RTT_RTT_H_

#include <atomic>
#include <cstdint>
#include <cstring>

namespace Rtt {

enum class Flags: uint32_t {
  BLOCK_IF_FULL = 2,
  NO_BLOCK_TRIM = 1
};

struct Channel {
  const char* const name { nullptr };
  std::uint8_t* const buffer = nullptr;
  const std::uint32_t size { 0 };
  std::uint32_t write { 0 };
  volatile std::uint32_t read { 0 };
  Flags flags { Flags::NO_BLOCK_TRIM };

  Channel(const char* name, std::uint8_t* buffer, std::uint32_t size) :
    name(name), buffer(buffer), size(size) { }
};

struct Header {
  constexpr static std::uint32_t ID_LENTGH = 16;

  char id[ID_LENTGH];
  std::uint32_t max_up_channels { 1 };
  std::uint32_t max_down_channels { 1 };

  Header() {
    // We need to split this string or otherwise it will be found in memory in .rodata
    // which is not correct
    const char* first_part = "SEGGER";
    const char* second_part = " RTT\0\0\0\0\0";
    constexpr uint32_t FIRST_PART_LENGTH = 6;
    constexpr uint32_t SECOND_PART_LENGTH = 10;
    std::memcpy(id, first_part, FIRST_PART_LENGTH);
    std::memcpy(id + FIRST_PART_LENGTH, second_part, SECOND_PART_LENGTH);
  }
};

struct ControlBlock {
  Header header;
  Rtt::Channel up_channel;
  Rtt::Channel down_channel;

  ControlBlock(std::uint8_t* up_buffer, std::uint32_t up_buffer_size,
                std::uint8_t* down_buffer, std::uint32_t down_buffer_size) :
    header(),
    up_channel("up", up_buffer, up_buffer_size),
    down_channel("down", down_buffer, down_buffer_size) { }
};

}  // namespace Rtt

#endif  // RTT_RTT_H_
