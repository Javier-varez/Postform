
#ifndef POSTFORM_RTT_LOGGER_H_
#define POSTFORM_RTT_LOGGER_H_

#include <atomic>
#include <string>
#include <vector>

#include "ditto/badge.h"
#include "postform/logger.h"

namespace Postform {

class FileLogger;
class FileWriter {
 public:
  FileWriter() = default;

  void write(const uint8_t* data, uint32_t size);
  void commit();

  FileWriter(const FileWriter&) = delete;
  FileWriter& operator=(const FileWriter&) = delete;

  FileWriter(FileWriter&&);
  FileWriter& operator=(FileWriter&&);
  ~FileWriter() { commit(); }

  operator bool() { return m_logger != nullptr; }

 private:
  int m_fd = -1;
  FileLogger* m_logger = nullptr;
  std::vector<uint8_t> m_data;

  FileWriter(FileLogger* logger, int fd) : m_fd(fd), m_logger(logger) {}
  friend class FileLogger;
};

class FileLogger : public Logger<FileLogger, FileWriter> {
 public:
  explicit FileLogger(std::string file_path);
  ~FileLogger();

  FileWriter getWriter(Ditto::Badge<Logger<FileLogger, FileWriter>>) {
    if (!m_taken.exchange(true)) {
      return FileWriter{this, m_fd};
    }
    return FileWriter{};
  }

  void release(Ditto::Badge<FileWriter>) { m_taken.store(false); }

 private:
  std::atomic_bool m_taken{false};
  int m_fd = -1;
};

}  // namespace Postform

#endif  // POSTFORM_RTT_LOGGER_H_
