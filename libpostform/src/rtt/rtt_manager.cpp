
#include "postform/rtt/rtt_manager.h"

#include "postform/utils.h"

static constexpr std::uint32_t UP_BUFFER_SIZE = 1024;
static constexpr std::uint32_t DOWN_BUFFER_SIZE = 16;
static UNINIT std::uint8_t s_up_buffer[UP_BUFFER_SIZE];
static UNINIT std::uint8_t s_down_buffer[DOWN_BUFFER_SIZE];

extern "C" Rtt::ControlBlock _SEGGER_RTT;
Rtt::ControlBlock _SEGGER_RTT {
  s_up_buffer, UP_BUFFER_SIZE,
  s_down_buffer, DOWN_BUFFER_SIZE
};

Rtt::RawWriter Rtt::Manager::getRawWriter() {
  if (takeWriter()) {
    return RawWriter { this, &_SEGGER_RTT.up_channel };
  }
  return RawWriter {};
}

Rtt::CobsWriter Rtt::Manager::getCobsWriter() {
  if (takeWriter()) {
    return CobsWriter { this, &_SEGGER_RTT.up_channel };
  }
  return CobsWriter {};
}
