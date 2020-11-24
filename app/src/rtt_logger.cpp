
#include "rtt_logger.h"

void RttLogger::startMessage() {
  Rtt::Manager& rtt = Rtt::Manager::getInstance();
  m_writer = rtt.getCobsWriter();
}

void RttLogger::appendData(const uint8_t* data, uint32_t length) {
  m_writer.write(data, length);
}

void RttLogger::appendString(const char* str) {
  const uint32_t len = strlen(str) + 1;
  m_writer.write(reinterpret_cast<const uint8_t*>(str), len);
}

void RttLogger::finishMessage() {
  m_writer.commit();
}
