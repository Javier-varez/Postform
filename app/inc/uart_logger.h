
#ifndef UART_LOGGER_H_
#define UART_LOGGER_H_

#include "format.h"

class UartLogger: public Logger<UartLogger> {
 private:
  void addData(const uint8_t* data, uint32_t length);

  friend Logger<UartLogger>;
};

#endif  // UART_LOGGER_H_