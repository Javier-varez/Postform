
#include "postform/rtt/cobs_writer.h"
#include "postform/rtt/rtt_manager.h"

Rtt::CobsWriter::CobsWriter(Rtt::Manager* manager, Rtt::Channel* channel) :
  m_manager(manager),
  m_channel(channel),
  m_write_ptr(channel->write),
  m_marker_ptr(channel->write) {
    blockUntilNotFull();
    m_channel->buffer[m_write_ptr] = 0;
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

  other.m_manager = nullptr;
  other.m_channel = nullptr;
  other.m_write_ptr = 0;
  other.m_marker_ptr = 0;
}

Rtt::CobsWriter& Rtt::CobsWriter::operator=(CobsWriter&& other) {
  if (this != &other) {
    commit();

    m_manager = other.m_manager;
    m_channel = other.m_channel;
    m_write_ptr = other.m_write_ptr;
    m_marker_ptr = other.m_marker_ptr;

    other.m_manager = nullptr;
    other.m_channel = nullptr;
    other.m_write_ptr = 0;
    other.m_marker_ptr = 0;
  }
  return *this;
}

void Rtt::CobsWriter::write(const uint8_t* data, uint32_t size) {
  if (!*this) {
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
  if (*this) {
    // Update the write pointer and mark the writer as done
    blockUntilNotFull();
    updateMarker();

    m_channel->write = m_write_ptr;
    m_manager->releaseWriter();
    m_manager = nullptr;
  }
}
