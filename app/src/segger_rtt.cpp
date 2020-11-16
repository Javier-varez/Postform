
#include "segger_rtt.h"

#include "utils.h"

static constexpr std::uint32_t UP_BUFFER_SIZE = 1024;
static constexpr std::uint32_t DOWN_BUFFER_SIZE = 16;
static UNINIT std::uint8_t s_up_buffer[UP_BUFFER_SIZE];
static UNINIT std::uint8_t s_down_buffer[DOWN_BUFFER_SIZE];

CLINKAGE Rtt::ControlBlock _SEGGER_RTT {
  s_up_buffer, UP_BUFFER_SIZE,
  s_down_buffer, DOWN_BUFFER_SIZE
};

void Rtt::write(const uint8_t *data, uint32_t size) {
  Writer writer(&_SEGGER_RTT.up_channel);
  writer.write(data, size);
}

Rtt::Writer::Writer(Channel* channel) :
  m_channel(channel),
  m_write_ptr(channel->write.load()) { }

Rtt::Writer::~Writer() {
  commit();
}

void Rtt::Writer::write(const uint8_t* data, uint32_t size) {
  if (m_state == State::Finished) {
    return;
  }

  while(size != 0) {
    uint32_t max_contiguous = getMaxContiguous();
    uint32_t count = size > max_contiguous ? max_contiguous : size;

    if (count == 0) {
      // Currently this only implements the blocking TX mode. We may need to add support
      // for other modes in the future.
      // We write the current buffer as is and then wait for more memory to be available
      m_channel->write.store(m_write_ptr);
      continue;
    }

    std::memcpy(&m_channel->buffer[m_write_ptr], data, count);

    // Increment data to point to the remaining buffer
    data += count;
    // Reduce size by the count
    size -= count;

    // Move the write pointer by the count and overflow if needed
    m_write_ptr += count;
    if (m_write_ptr >= m_channel->size) {
      m_write_ptr = 0;
    }
  }
}

void Rtt::Writer::commit() {
  if (m_state == State::Writable) {
    // Update the write pointer and mark the writer as done
    m_channel->write.store(m_write_ptr);
    m_state = State::Finished;
  }
}

uint32_t Rtt::Writer::getMaxContiguous() const {
  uint32_t read = m_channel->read.load();
  uint32_t channel_size = m_channel->size;

  if (read == 0) {
    return channel_size - m_write_ptr - 1;
  } else if (read > m_write_ptr) {
    return read - m_write_ptr - 1;
  } else {
    return channel_size - m_write_ptr;
  }
}


