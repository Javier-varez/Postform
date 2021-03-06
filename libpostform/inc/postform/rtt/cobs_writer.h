
#ifndef POSTFORM_RTT_COBS_WRITER_H_
#define POSTFORM_RTT_COBS_WRITER_H_

#include <atomic>
#include <cstdint>

#include "postform/rtt/rtt.h"

namespace Postform {
namespace Rtt {
class Manager;

class CobsWriter {
 public:
  void write(const uint8_t* data, uint32_t size);
  void commit();

  CobsWriter() = default;
  CobsWriter(const CobsWriter&) = delete;
  CobsWriter& operator=(const CobsWriter&) = delete;

  CobsWriter(CobsWriter&&);
  CobsWriter& operator=(CobsWriter&&);
  ~CobsWriter();

  operator bool() { return m_manager != nullptr; }

 private:
  Manager* m_manager = nullptr;
  Channel* m_channel = nullptr;
  uint32_t m_write_ptr = 0;
  uint32_t m_marker_ptr = 0;

  CobsWriter(Manager* rtt, Channel* channel);

  inline void blockUntilNotFull() {
    if (m_channel->flags.load(std::memory_order_relaxed) !=
        Rtt::Flags::BLOCK_IF_FULL) {
      return;
    }
    const uint32_t next_write_ptr = nextWritePtr();
    if (m_channel->read.load(std::memory_order_acquire) == next_write_ptr) {
      m_channel->write.store(m_marker_ptr, std::memory_order_release);
      while (m_channel->read.load(std::memory_order_relaxed) ==
             next_write_ptr) {
      }
    }
  }

  inline uint8_t markerDistance() {
    if (m_marker_ptr > m_write_ptr) {
      return m_channel->size - m_marker_ptr + m_write_ptr;
    }
    return m_write_ptr - m_marker_ptr;
  }

  inline uint32_t nextWritePtr() {
    uint32_t write_ptr = m_write_ptr + 1;
    if (write_ptr >= m_channel->size) {
      write_ptr -= m_channel->size;
    }
    return write_ptr;
  }

  inline void updateMarker() {
    // Encode chunk length and move marker
    m_channel->buffer[m_marker_ptr] = markerDistance();

    // Update marker position
    m_marker_ptr = m_write_ptr;
    m_channel->buffer[m_write_ptr] = 0;
    m_write_ptr = nextWritePtr();
  }

  friend class Manager;
};

}  // namespace Rtt
}  // namespace Postform

#endif  // POSTFORM_RTT_COBS_WRITER_H_
