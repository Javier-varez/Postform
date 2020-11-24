
#include "rtt/cobs_writer.h"
#include "rtt/rtt.h"

Rtt::CobsWriter::CobsWriter() : m_state(State::Finished) { }

Rtt::CobsWriter::CobsWriter(Rtt::Manager* manager, Rtt::Channel* channel) :
  m_manager(manager),
  m_channel(channel),
  m_marker_ptr(channel->write.load()) {
    m_write_ptr = m_marker_ptr;
    // Set the marker
    m_channel->buffer[m_marker_ptr] = 0;
    incrementWritePtr();
  }

Rtt::CobsWriter::~CobsWriter() {
  commit();
}

Rtt::CobsWriter::CobsWriter(CobsWriter&& other) {
  m_manager = other.m_manager;
  m_channel = other.m_channel;
  m_write_ptr = other.m_write_ptr;
  m_marker_ptr = other.m_marker_ptr;
  m_state = other.m_state;

  other.m_manager = nullptr;
  other.m_channel = nullptr;
  other.m_write_ptr = 0;
  other.m_marker_ptr = 0;
  other.m_state = State::Finished;
}

Rtt::CobsWriter& Rtt::CobsWriter::operator=(CobsWriter&& other) {
  if (this != &other) {
    commit();

    m_manager = other.m_manager;
    m_channel = other.m_channel;
    m_write_ptr = other.m_write_ptr;
    m_marker_ptr = other.m_marker_ptr;
    m_state = other.m_state;

    other.m_manager = nullptr;
    other.m_channel = nullptr;
    other.m_write_ptr = 0;
    other.m_marker_ptr = 0;
    other.m_state = State::Finished;
  }
  return *this;
}

uint8_t Rtt::CobsWriter::markerDistance() {
  const uint32_t channel_size = m_channel->size;
  if (m_marker_ptr > m_write_ptr) {
    return channel_size - m_marker_ptr + m_write_ptr;
  }
  return m_write_ptr - m_marker_ptr;
}

void Rtt::CobsWriter::incrementWritePtr() {
  m_write_ptr++;
  if (m_write_ptr >= m_channel->size) {
    m_write_ptr -= m_channel->size;
  }
}

void Rtt::CobsWriter::encodeInPlace(const uint8_t* data, uint32_t size) {
  for (uint32_t i = 0; i < size; i++) {
    if (data[i] == 0) {
      // Encode chunk length and move marker
      m_channel->buffer[m_marker_ptr] = markerDistance();

      // Update marker position
      m_marker_ptr = m_write_ptr;
      m_channel->buffer[m_marker_ptr] = 0;

      incrementWritePtr();
    } else {
      m_channel->buffer[m_write_ptr] = data[i];
      incrementWritePtr();

      // Check if we need to insert a virtual zero.
      if (markerDistance() == 0xFF) {
        m_channel->buffer[m_marker_ptr] = 0xFF;

        m_marker_ptr = m_write_ptr;
        m_channel->buffer[m_marker_ptr] = 0;

        incrementWritePtr();
      }
    }
  }
}

void Rtt::CobsWriter::write(const uint8_t* data, uint32_t size) {
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
      m_channel->write.store(m_marker_ptr);
      continue;
    }

    encodeInPlace(data, count);

    // Increment data to point to the remaining buffer
    data += count;
    // Reduce size by the count
    size -= count;
  }
}

void Rtt::CobsWriter::commit() {
  if (m_state == State::Writable) {
    // Update the write pointer and mark the writer as done
    m_channel->buffer[m_marker_ptr] = markerDistance();
    m_channel->buffer[m_write_ptr] = 0;
    incrementWritePtr();

    m_channel->write.store(m_write_ptr);
    m_state = State::Finished;
    if (m_manager) m_manager->releaseWriter();
  }
}

uint32_t Rtt::CobsWriter::getMaxContiguous() const {
  const uint32_t read = m_channel->read.load();
  const uint32_t channel_size = m_channel->size;

  uint32_t max = 0;
  if (read == 0) {
    max =  channel_size - m_write_ptr - 1;
  } else if (read > m_write_ptr) {
    max = read - m_write_ptr - 1;
  } else {
    max = channel_size - m_write_ptr;
  }

  // TODO(javier-varez): subtract here the byte stuffing overhead
  return max;
}
