
#include "rtt/cobs_writer.h"
#include "rtt/rtt.h"

Rtt::CobsWriter::CobsWriter() : m_state(State::Finished) { }

Rtt::CobsWriter::CobsWriter(Rtt::Manager* manager, Rtt::Channel* channel) :
  m_manager(manager),
  m_channel(channel),
  m_marker_ptr(channel->write.load()) {
    m_write_ptr = m_marker_ptr;
    // Set the marker
    blockUntilNotFull();
    m_channel->buffer[m_marker_ptr] = 0;
    m_write_ptr = nextWritePtr();
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

uint32_t Rtt::CobsWriter::nextWritePtr() {
  uint32_t write_ptr = m_write_ptr + 1;
  if (write_ptr >= m_channel->size) {
    write_ptr -= m_channel->size;
  }
  return write_ptr;
}

void Rtt::CobsWriter::updateMarker() {
  // Encode chunk length and move marker
  m_channel->buffer[m_marker_ptr] = markerDistance();

  // Update marker position
  m_marker_ptr = m_write_ptr;
  m_channel->buffer[m_marker_ptr] = 0;

  m_write_ptr = nextWritePtr();
}

void Rtt::CobsWriter::write(const uint8_t* data, uint32_t size) {
  if (m_state == State::Finished) {
    return;
  }

  for (uint32_t i = 0; i < size; i++) {
    blockUntilNotFull();
    if (data[i] == 0) {
      updateMarker();
    } else {
      m_channel->buffer[m_write_ptr] = data[i];
      m_write_ptr = nextWritePtr();

      // Check if we need to insert a virtual zero.
      if (markerDistance() == 0xFF) {
        blockUntilNotFull();
        updateMarker();
      }
    }
  }
}

void Rtt::CobsWriter::commit() {
  if (m_state == State::Writable) {
    // Update the write pointer and mark the writer as done
    blockUntilNotFull();
    updateMarker();

    m_channel->write.store(m_write_ptr);
    m_state = State::Finished;
    if (m_manager) m_manager->releaseWriter();
  }
}

void Rtt::CobsWriter::blockUntilNotFull() {
  uint32_t next_write_ptr = nextWritePtr();
  if (m_channel->read.load() == next_write_ptr) {
    m_channel->write.store(m_marker_ptr);
    while (m_channel->read.load() == next_write_ptr) { }
  }
}
