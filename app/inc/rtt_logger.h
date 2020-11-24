

#ifndef RTT_LOGGER_H_
#define RTT_LOGGER_H_

#include "logger.h"
#include "rtt/rtt.h"

class RttLogger: public Logger<RttLogger> {
 private:
  Rtt::CobsWriter m_writer;

  void startMessage();
  void appendData(const uint8_t* data, uint32_t length);
  void appendString(const char* str);
  void finishMessage();

  friend Logger<RttLogger>;
};

#endif
