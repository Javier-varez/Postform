
#include "postform/rtt/transport.h"

Postform::Rtt::Transport::Transport(Channel* channel)
    : m_channel(channel),
      m_write_ptr(m_channel->write.load(std::memory_order_relaxed)) {}

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
