
#include "postform/rtt/transport.h"

#include "postform/utils.h"

static constexpr std::uint32_t UP_BUFFER_SIZE = 1024;
static UNINIT std::uint8_t s_up_buffer[UP_BUFFER_SIZE];

extern "C" Postform::Rtt::ControlBlock _SEGGER_RTT{s_up_buffer, UP_BUFFER_SIZE,
                                                   nullptr, 0};

Postform::Rtt::Transport::Transport()
    : m_channel(&_SEGGER_RTT.up_channel),
      m_write_ptr(
          _SEGGER_RTT.up_channel.write.load(std::memory_order_relaxed)) {}

uint32_t Postform::Rtt::Transport::getNextWritePtr() const {
  uint32_t write_ptr = m_write_ptr;
  write_ptr++;
  if (write_ptr >= m_channel->size) {
    write_ptr -= m_channel->size;
  }
  return write_ptr;
}

void Postform::Rtt::Transport::write(uint8_t value) {
  auto& channel = *m_channel;
  const uint32_t next_write_ptr = getNextWritePtr();

  // Block if needed
  if (channel.flags.load(std::memory_order_relaxed) ==
      Rtt::Flags::BLOCK_IF_FULL) {
    while (channel.read.load(std::memory_order_relaxed) == next_write_ptr) {
      // Comit what we have until now, since it may be blocking the reader.
      commit();
    }
  }

  channel.buffer[m_write_ptr] = value;
  m_write_ptr = next_write_ptr;
}

void Postform::Rtt::Transport::commit() {
  m_channel->write.store(m_write_ptr, std::memory_order_relaxed);
}
