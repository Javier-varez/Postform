#ifndef POSTFORM_LOGGER_H_
#define POSTFORM_LOGGER_H_

#include <atomic>
#include <cstdint>
#include <cstring>

#include "postform/format_validator.h"
#include "postform/types.h"

namespace Postform {

/**
 * @brief Describes supported log levels by Postform
 */
enum class LogLevel {
  //! All logs are shown
  DEBUG,
  //! Error + Warning + Info logs are shown
  INFO,
  //! Error + Warning logs are shown
  WARNING,
  //! Only Error logs are shown
  ERROR,
  //! No logs are shown
  OFF
};

/**
 * @brief Postform calls this function to obtain the global timestamp.
 * @return the value of the timestamp
 *
 * SAFETY: Make sure this function is reentrant and does not lock.
 *         This can be called from both interrupt and thread contexts.
 *
 * A simple implementation might use an atomic counter.
 * ```
 * #include <atomic>
 *
 * uint64_t getGlobalTimestamp() {
 *   static std::atomic_uint32_t counter;
 *   return counter.fetch_add(1U);
 * }
 * ```
 */
extern uint64_t getGlobalTimestamp();

/**
 * @brief Base logger class used by Postform.
 *
 * Derived classes using CRTP can implement multiple
 * transports. These classes must implement the
 * following methods:
 *
 * ```
 * Writer getWriter();
 * ```
 */
template<class Derived, class Writer>
class Logger {
 public:

  /**
   * @brief writes a log to the transport
   * @param level level of the current log.
   * @param args arguments to serialize in the log
   */
  template<typename ... T>
  void log(LogLevel level, T ...args) {
    if (level < m_level.load()) return;

    uint64_t timestamp = getGlobalTimestamp();

    Writer writer = static_cast<Derived&>(*this).getWriter();
    writer.write(reinterpret_cast<const uint8_t*>(&timestamp), sizeof(timestamp));
    sendRemainingArguments(writer, args...);
    writer.commit();
  }

  /**
   * @brief Sets the log level for the logger.
   *
   * Logs with levels above or equal the selected one are printed.
   * Others are ignored.
   */
  void setLevel(LogLevel level) { m_level.store(level); }

 private:
  std::atomic<LogLevel> m_level = LogLevel::DEBUG;

  template<typename T>
  void sendArgument(Writer& writer, const T argument) {
    writer.write(reinterpret_cast<const uint8_t*>(&argument), sizeof(T));
  }

  template<>
  void sendArgument<const char*>(Writer& writer, const char* argument) {
    const auto length = strlen(argument) + 1;
    writer.write(reinterpret_cast<const uint8_t*>(argument), length);
  }

  template<>
  void sendArgument<char*>(Writer& writer, char* argument) {
    const auto length = strlen(argument) + 1;
    writer.write(reinterpret_cast<const uint8_t*>(argument), length);
  }

  template<typename T>
  void sendRemainingArguments(Writer& writer, const T first_arg) {
    sendArgument(writer, first_arg);
  }

  template<typename T, typename ... Types>
  void sendRemainingArguments(Writer& writer, const T first_arg, Types... args) {
    sendArgument(writer, first_arg);
    sendRemainingArguments(writer, args...);
  }
};

/**
 * @brief Struct template representing an interned debug string.
 *
 * Instantiates a string constant in the ".interned_strings.debug" section.
 * This section is not used in runtime, only used as debug information for Postform
 */
template<char... N>
struct InternedDebugString {
  __attribute__((section(".interned_strings.debug"))) static constexpr char string[] { N... };
};

template<char... N>
constexpr char InternedDebugString<N...>::string[];

/**
 * @brief Struct template representing an interned info string.
 *
 * Instantiates a string constant in the ".interned_strings.info" section.
 * This section is not used in runtime, only used as debug information for Postform
 */
template<char... N>
struct InternedInfoString {
  __attribute__((section(".interned_strings.info"))) static constexpr char string[] { N... };
};

template<char... N>
constexpr char InternedInfoString<N...>::string[];

/**
 * @brief Struct template representing an interned warning string.
 *
 * Instantiates a string constant in the ".interned_strings.warning" section.
 * This section is not used in runtime, only used as debug information for Postform
 */
template<char... N>
struct InternedWarningString {
  __attribute__((section(".interned_strings.warning"))) static constexpr char string[] { N... };
};

template<char... N>
constexpr char InternedWarningString<N...>::string[];

/**
 * @brief Struct template representing an interned error string.
 *
 * Instantiates a string constant in the ".interned_strings.error" section.
 * This section is not used in runtime, only used as debug information for Postform
 */
template<char... N>
struct InternedErrorString {
  __attribute__((section(".interned_strings.error"))) static constexpr char string[] { N... };
};

template<char... N>
constexpr char InternedErrorString<N...>::string[];

/**
 * @brief Struct template representing an interned user string.
 *
 * Instantiates a string constant in the ".interned_strings.user" section.
 * This section is not used in runtime, only used as debug information for Postform
 */
template<char... N>
struct InternedUserString {
  __attribute__((section(".interned_strings.user"))) static constexpr char string[] { N... };
};

template<char... N>
constexpr char InternedUserString<N...>::string[];

}  // namespace Postform

/**
 * @brief variadic template user defined literal to declare a debug interned string.
 *
 * This is currently only supported by clang.
 */
template<typename T, T... chars>
constexpr Postform::InternedString operator ""_intern_debug() {
  return Postform::InternedString { Postform::InternedDebugString<chars..., '\0'>::string };
}

/**
 * @brief variadic template user defined literal to declare a info interned string.
 *
 * This is currently only supported by clang.
 */
template<typename T, T... chars>
constexpr Postform::InternedString operator ""_intern_info() {
  return Postform::InternedString { Postform::InternedInfoString<chars..., '\0'>::string };
}

/**
 * @brief variadic template user defined literal to declare a warning interned string.
 *
 * This is currently only supported by clang.
 */
template<typename T, T... chars>
constexpr Postform::InternedString operator ""_intern_warning() {
  return Postform::InternedString { Postform::InternedWarningString<chars..., '\0'>::string };
}

/**
 * @brief variadic template user defined literal to declare a error interned string.
 *
 * This is currently only supported by clang.
 */
template<typename T, T... chars>
constexpr Postform::InternedString operator ""_intern_error() {
  return Postform::InternedString { Postform::InternedErrorString<chars..., '\0'>::string };
}

/**
 * @brief variadic template user defined literal to declare a error interned string.
 *
 * This is currently only supported by clang.
 */
template<typename T, T... chars>
constexpr Postform::InternedString operator ""_intern() {
  return Postform::InternedString { Postform::InternedUserString<chars..., '\0'>::string };
}

#define __POSTFORM_LOG(level, intern_mode, logger, fmt, ...) \
  { \
    POSTFORM_ASSERT_FORMAT(fmt, ## __VA_ARGS__); \
    (logger)->log(level, __FILE__ "@" __POSTFORM_EXPAND_AND_STRINGIFY(__LINE__) "@" fmt ## intern_mode, ## __VA_ARGS__); \
  }

/**
 * @brief Macro for a debug log with a printf-like formatting
 */
#define LOG_DEBUG(logger, fmt, ...) \
  __POSTFORM_LOG(Postform::LogLevel::DEBUG, _intern_debug, logger, fmt, ## __VA_ARGS__)
/**
 * @brief Macro for an info log with a printf-like formatting
 */
#define LOG_INFO(logger, fmt, ...) \
  __POSTFORM_LOG(Postform::LogLevel::INFO, _intern_info, logger, fmt, ## __VA_ARGS__)
/**
 * @brief Macro for a warning log with a printf-like formatting
 */
#define LOG_WARNING(logger, fmt, ...) \
  __POSTFORM_LOG(Postform::LogLevel::WARNING, _intern_warning, logger, fmt, ## __VA_ARGS__)
/**
 * @brief Macro for an error log with a printf-like formatting
 */
#define LOG_ERROR(logger, fmt, ...) \
  __POSTFORM_LOG(Postform::LogLevel::ERROR, _intern_error, logger, fmt, ## __VA_ARGS__)

#endif  // POSTFORM_LOGGER_H_
