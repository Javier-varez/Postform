

#ifndef RTT_LOGGER_H_
#define RTT_LOGGER_H_

#include "postform/logger.h"
#include "postform/rtt/rtt_manager.h"

namespace Postform {

class RttLogger: public Logger<RttLogger> {
 public:
  RttLogger();

 private:
  Rtt::Manager& m_manager;
  Rtt::CobsWriter m_writer;

  void startMessage(uint64_t timestamp);
  void appendData(const uint8_t* data, uint32_t length);
  void appendString(const char* str);
  void finishMessage();

  friend Logger<RttLogger>;
};

}  // namespace Postform

#endif
