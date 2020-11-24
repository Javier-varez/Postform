
#include "rtt_writer.h"
#include "rtt.h"

Rtt::Writer::Writer() : m_state(State::Finished) { }

Rtt::Writer::Writer(Rtt::Manager* manager, Rtt::Channel* channel) :
  m_manager(manager),
  m_channel(channel),
  m_write_ptr(channel->write.load()) { }

Rtt::Writer::~Writer() {
  commit();
}

Rtt::Writer::Writer(Writer&& other) {
  m_manager = other.m_manager;
  m_channel = other.m_channel;
  m_write_ptr = other.m_write_ptr;
  m_state = other.m_state;

  other.m_manager = nullptr;
  other.m_channel = nullptr;
  other.m_write_ptr = 0;
  other.m_state = State::Finished;
}

Rtt::Writer& Rtt::Writer::operator=(Writer&& other) {
  if (this != &other) {
    commit();

    m_manager = other.m_manager;
    m_channel = other.m_channel;
    m_write_ptr = other.m_write_ptr;
    m_state = other.m_state;

    other.m_manager = nullptr;
    other.m_channel = nullptr;
    other.m_write_ptr = 0;
    other.m_state = State::Finished;
  }
  return *this;
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
    if (m_manager) m_manager->releaseWriter();
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
