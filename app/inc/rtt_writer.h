
#ifndef RTT_WRITER_H_
#define RTT_WRITER_H_

#include <cstdint>

namespace Rtt {
class Manager;
struct Channel;

class Writer {
 public:
  void write(const uint8_t *data, uint32_t size);
  void commit();

  Writer();
  Writer(const Writer&) = delete;
  Writer& operator=(const Writer&) = delete;

  Writer(Writer&&);
  Writer& operator=(Writer&&);
  ~Writer();

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

  Writer(Manager* rtt, Channel* channel);
  uint32_t getMaxContiguous() const;

  friend class Manager;
};

}  // namespace Rtt

#endif  // RTT_WRITER_H_
