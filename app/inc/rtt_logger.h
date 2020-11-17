

#ifndef RTT_LOGGER_H_
#define RTT_LOGGER_H_

#include "format.h"
#include "segger_rtt.h"

class RttLogger: public Logger<RttLogger> {
 public:
  void startMessage();
  void appendData(const uint8_t* data, uint32_t length);
  void appendString(const char* str);
  void finishMessage();

 private:
  constexpr static uint32_t BUFFER_LENGTH = 128;
  uint8_t m_buffer[BUFFER_LENGTH];
  uint32_t m_index;

  friend Logger<RttLogger>;
};

#endif
