#ifndef LOGGER_H_
#define LOGGER_H_

#include <cstdint>
#include <cstring>

namespace Postform {

struct InternedString {
  const char* str;
};

enum class LogLevel {
  DEBUG,
  INFO,
  WARNING,
  ERROR,
  OFF
};

uint64_t getGlobalTimestamp();

template<class Derived>
class Logger {
 public:
  template<typename ... T>
  inline void log(LogLevel level, T ...args) {
    if (level < m_level) return;

    m_derived.startMessage(getGlobalTimestamp());
    sendRemainingArguments(args...);
    m_derived.finishMessage();
  }

  inline void printfFmtValidator([[maybe_unused]] const char* fmt, ...)
    __attribute__((format(printf, 2, 3))) { }

  void setLevel(LogLevel level) { m_level = level; }

 private:
  LogLevel m_level = LogLevel::DEBUG;
  Derived& m_derived = static_cast<Derived&>(*this);

  template<typename T>
  inline void sendArgument(const T argument) {
    m_derived.appendData(reinterpret_cast<const uint8_t*>(&argument), sizeof(T));
  }

  inline void sendArgument(const char* argument) {
    m_derived.appendString(argument);
  }

  template<typename T>
  inline void sendRemainingArguments(const T& first_arg) {
    sendArgument(first_arg);
  }

  template<typename T, typename ... Types>
  inline void sendRemainingArguments(const T& first_arg, Types... args) {
    sendArgument(first_arg);
    sendRemainingArguments(args...);
  }
};

}  // namespace Postform

template<char... N>
struct InternedDebugString {
  __attribute__((section(".interned_strings.debug"))) static constexpr char string[] { N... };
};

template<char... N>
constexpr char InternedDebugString<N...>::string[];

template<char... N>
struct InternedInfoString {
  __attribute__((section(".interned_strings.info"))) static constexpr char string[] { N... };
};

template<char... N>
constexpr char InternedInfoString<N...>::string[];

template<char... N>
struct InternedWarningString {
  __attribute__((section(".interned_strings.warning"))) static constexpr char string[] { N... };
};

template<char... N>
constexpr char InternedWarningString<N...>::string[];

template<char... N>
struct InternedErrorString {
  __attribute__((section(".interned_strings.error"))) static constexpr char string[] { N... };
};

template<char... N>
constexpr char InternedErrorString<N...>::string[];

template<typename T, T... C>
Postform::InternedString operator ""_intern_debug() {
  return Postform::InternedString { decltype(InternedDebugString<C..., T{}>{})::string };
}

template<typename T, T... C>
Postform::InternedString operator ""_intern_info() {
  return Postform::InternedString { decltype(InternedInfoString<C..., T{}>{})::string };
}

template<typename T, T... C>
Postform::InternedString operator ""_intern_warning() {
  return Postform::InternedString { decltype(InternedWarningString<C..., T{}>{})::string };
}

template<typename T, T... C>
Postform::InternedString operator ""_intern_error() {
  return Postform::InternedString { decltype(InternedErrorString<C..., T{}>{})::string };
}

#define __STRINGIFY(X) #X
#define __EXPAND_AND_STRINGIFY(X) __STRINGIFY(X)

#define __LOG(level, intern_mode, logger, fmt, ...) \
  { \
    (logger)->printfFmtValidator(fmt, ## __VA_ARGS__); \
    (logger)->log(level, __FILE__ "@" __EXPAND_AND_STRINGIFY(__LINE__) "@" fmt ## intern_mode, ## __VA_ARGS__); \
  }

#define LOG_DEBUG(logger, fmt, ...) __LOG(Postform::LogLevel::DEBUG, _intern_debug, logger, fmt, ## __VA_ARGS__)
#define LOG_INFO(logger, fmt, ...) __LOG(Postform::LogLevel::INFO, _intern_info, logger, fmt, ## __VA_ARGS__)
#define LOG_WARNING(logger, fmt, ...) __LOG(Postform::LogLevel::WARNING, _intern_warning, logger, fmt, ## __VA_ARGS__)
#define LOG_ERROR(logger, fmt, ...) __LOG(Postform::LogLevel::ERROR, _intern_error, logger, fmt, ## __VA_ARGS__)

#endif  // LOGGER_H_
