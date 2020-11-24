
#include "rtt.h"

#include "utils.h"

static constexpr std::uint32_t UP_BUFFER_SIZE = 1024;
static constexpr std::uint32_t DOWN_BUFFER_SIZE = 16;
static UNINIT std::uint8_t s_up_buffer[UP_BUFFER_SIZE];
static UNINIT std::uint8_t s_down_buffer[DOWN_BUFFER_SIZE];

CLINKAGE Rtt::ControlBlock _SEGGER_RTT;
Rtt::ControlBlock _SEGGER_RTT {
  s_up_buffer, UP_BUFFER_SIZE,
  s_down_buffer, DOWN_BUFFER_SIZE
};

Rtt::Writer Rtt::Manager::getWriter() {
  if (!m_taken.exchange(true)) {
    // Writer was taken successfully
    return Writer { this, &_SEGGER_RTT.up_channel };
  }

  return Writer {};
}
