
#include "rtt_logger.h"

void RttLogger::startMessage() {
  m_index = 0;
}

void RttLogger::appendData(const uint8_t* data, uint32_t length) {
  if ((BUFFER_LENGTH - m_index) > length) {
    memcpy(&m_buffer[m_index], data, length);
    m_index += length;
  }
}

void RttLogger::appendString(const char* str) {
  while ((*str != '\0') && (m_index < BUFFER_LENGTH)) {
    m_buffer[m_index] = *str;
    m_index++;
    str++;
  };

  if (m_index < BUFFER_LENGTH) {
    m_buffer[m_index++] = '\0';
  }
}

void RttLogger::finishMessage() {
  Rtt& rtt = Rtt::getInstance();
  Rtt::Writer writer = rtt.getWriter();

  writer.write(m_buffer, m_index);
  writer.commit();
}
