
#ifndef POSTFORM_RTT_RTT_H_
#define POSTFORM_RTT_RTT_H_

#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <span>

#include "ditto/assert.h"

namespace Postform::Rtt {

enum class Flags : uint32_t { BLOCK_IF_FULL = 2, NO_BLOCK_TRIM = 1 };

struct Channel {
  const char* name{nullptr};
  std::uint8_t* buffer = nullptr;
  std::uint32_t size{0};
  std::atomic<std::uint32_t> write{0};
  std::atomic<std::uint32_t> read{0};
  std::atomic<Flags> flags{Flags::NO_BLOCK_TRIM};
};

struct ChannelDescriptor {
  const char* name;
  std::span<uint8_t> buffer;
};

struct Header {
  constexpr static std::uint32_t ID_LENTGH = 16;

  char id[ID_LENTGH];
  std::uint32_t max_up_channels;
  std::uint32_t max_down_channels;

  Header(uint32_t max_up_channels, uint32_t max_down_channels)
      : max_up_channels(max_up_channels), max_down_channels(max_down_channels) {
    // We need to split this string or otherwise it will be found in memory in
    // .rodata which is not correct
    const char* first_part = "SEGGER";
    const char* second_part = " RTT\0\0\0\0\0";
    constexpr uint32_t FIRST_PART_LENGTH = 6;
    constexpr uint32_t SECOND_PART_LENGTH = 10;
    std::memcpy(id, first_part, FIRST_PART_LENGTH);
    std::memcpy(id + FIRST_PART_LENGTH, second_part, SECOND_PART_LENGTH);
  }
};

template <std::size_t UP_CHANNELS, std::size_t DOWN_CHANNELS>
struct ControlBlock {
  Header header;
  std::array<Rtt::Channel, UP_CHANNELS> up_channels;
  std::array<Rtt::Channel, DOWN_CHANNELS> down_channels;

  ControlBlock(std::span<ChannelDescriptor> up_channel_descriptors,
               std::span<ChannelDescriptor> down_channel_descriptors)
      : header(UP_CHANNELS, DOWN_CHANNELS) {
    DITTO_VERIFY(up_channel_descriptors.size() == UP_CHANNELS);
    DITTO_VERIFY(down_channel_descriptors.size() == DOWN_CHANNELS);

    for (uint32_t i = 0; i < up_channel_descriptors.size(); i++) {
      up_channels[i].name = up_channel_descriptors[i].name;
      up_channels[i].buffer = up_channel_descriptors[i].buffer.data();
      up_channels[i].size = up_channel_descriptors[i].buffer.size();
    }
    for (uint32_t i = 0; i < down_channel_descriptors.size(); i++) {
      down_channels[i].name = down_channel_descriptors[i].name;
      down_channels[i].buffer = down_channel_descriptors[i].buffer.data();
      down_channels[i].size = down_channel_descriptors[i].buffer.size();
    }
  }
};

}  // namespace Postform::Rtt

#endif  // RTT_RTT_H_
