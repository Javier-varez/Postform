
#include "postform/file_logger.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace Postform {

void FileWriter::write(const uint8_t* data, uint32_t size) {
  if (*this) {
    m_data.insert(m_data.end(), data, data + size);
  }
}

void FileWriter::commit() {
  if (m_logger) {
    uint32_t size = m_data.size();
    [[maybe_unused]] uint32_t written = ::write(m_fd, &size, sizeof(size));
    written = ::write(m_fd, m_data.data(), m_data.size());
    m_logger->release({});
    m_logger = nullptr;
    m_fd = -1;
  }
}

FileWriter::FileWriter(FileWriter&& other) {
  m_fd = other.m_fd;
  m_logger = other.m_logger;
  other.m_fd = -1;
  other.m_logger = nullptr;
}

FileWriter& FileWriter::operator=(FileWriter&& other) {
  if (this != &other) {
    commit();
    m_fd = other.m_fd;
    m_logger = other.m_logger;
    other.m_fd = -1;
    other.m_logger = nullptr;
  }
  return *this;
}

FileLogger::FileLogger(std::string file_path) {
  m_fd = open(file_path.c_str(), O_CREAT | O_RDWR, 0664);
}

FileLogger::~FileLogger() { close(m_fd); }

}  // namespace Postform
