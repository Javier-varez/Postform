
#ifndef RTT_RTT_MANAGER_H_
#define RTT_RTT_MANAGER_H_

#include <atomic>
#include <cstdint>

#include "rtt/rtt.h"
#include "rtt/raw_writer.h"
#include "rtt/cobs_writer.h"

namespace Rtt {
class Manager {
 public:
  static Manager& getInstance() {
    static Manager manager;
    return manager;
  }

  RawWriter getRawWriter();
  CobsWriter getCobsWriter();

 private:
  std::atomic<bool> m_taken { false };

  Manager() = default;
  void releaseWriter() {
    m_taken.store(false);
  }

  bool takeWriter() {
    return !m_taken.exchange(true);
  }

  friend class RawWriter;
  friend class CobsWriter;
};
}  // namespace Rtt

#endif  // RTT_RTT_MANAGER_H_
