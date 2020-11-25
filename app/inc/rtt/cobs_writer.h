
#ifndef RTT_COBS_WRITER_H_
#define RTT_COBS_WRITER_H_

#include <cstdint>

namespace Rtt {
class Manager;
struct Channel;

class CobsWriter {
 public:
  void write(const uint8_t *data, uint32_t size);
  void commit();

  CobsWriter();
  CobsWriter(const CobsWriter&) = delete;
  CobsWriter& operator=(const CobsWriter&) = delete;

  CobsWriter(CobsWriter&&);
  CobsWriter& operator=(CobsWriter&&);
  ~CobsWriter();

  operator bool() { return State::Writable == m_state; }

 private:
  enum class State {
    Writable,
    Finished
  };

  Manager* m_manager = nullptr;
  Channel* m_channel = nullptr;
  uint32_t m_write_ptr = 0;
  uint32_t m_marker_ptr = 0;
  State m_state = State::Writable;

  CobsWriter(Manager* rtt, Channel* channel);
  uint32_t getMaxContiguous() const;
  void encodeInPlace(const uint8_t* data, uint32_t size);
  uint8_t markerDistance();
  void incrementWritePtr();
  void updateMarker();

  friend class Manager;
};

}  // namespace Rtt

#endif  // RTT_COBS_WRITER_H_
