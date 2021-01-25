
#include "postform/rtt_logger.h"

RttLogger::RttLogger() :
  Logger<RttLogger>(),
  m_manager(Rtt::Manager::getInstance()) {}

void RttLogger::startMessage(uint64_t timestamp) {
  m_writer = m_manager.getCobsWriter();
  m_writer.write(reinterpret_cast<const uint8_t*>(&timestamp), sizeof(timestamp));
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
