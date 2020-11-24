
#ifndef RTT_RAW_WRITER_H_
#define RTT_RAW_WRITER_H_

#include <cstdint>

namespace Rtt {
class Manager;
struct Channel;

class RawWriter {
 public:
  void write(const uint8_t *data, uint32_t size);
  void commit();

  RawWriter();
  RawWriter(const RawWriter&) = delete;
  RawWriter& operator=(const RawWriter&) = delete;

  RawWriter(RawWriter&&);
  RawWriter& operator=(RawWriter&&);
  ~RawWriter();

  operator bool() { return State::Writable == m_state; }

 private:
  enum class State {
    Writable,
    Finished
  };

  Manager* m_manager = nullptr;
  Channel* m_channel = nullptr;
  uint32_t m_write_ptr = 0;
  State m_state = State::Writable;

  RawWriter(Manager* rtt, Channel* channel);
  uint32_t getMaxContiguous() const;

  friend class Manager;
};

}  // namespace Rtt

#endif  // RTT_RAW_WRITER_H_
