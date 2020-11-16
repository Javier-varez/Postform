

#ifndef RTT_LOGGER_H_
#define RTT_LOGGER_H_

#include "format.h"
#include "segger_rtt.h"

class RttLogger: public Logger<RttLogger> {
 public:
  void addData(const uint8_t* data, uint32_t length);
  void addString(const char* str);

  friend Logger<RttLogger>;
};

#endif
