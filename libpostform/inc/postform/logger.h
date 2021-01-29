#ifndef POSTFORM_LOGGER_H_
#define POSTFORM_LOGGER_H_

#include <cstdint>
#include <cstring>

namespace Postform {

/**
 * @brief Internal representation of an interned string.
 *
 * This is serialized as a pointer, instead of copying
 * the whole string through the transport.
 */
struct InternedString {
  const char* str;
};

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
 * void startMessage(uint64_t timestamp);
 * void appendData(const uint8_t* data, uint32_t length);
 * void appendString(const char* string);
 * void finishMessage();
 * ```
 */
template<class Derived>
class Logger {
 public:

  /**
   * @brief writes a log to the transport
   * @param level level of the current log.
   * @param args arguments to serialize in the log
   */
  template<typename ... T>
  inline void log(LogLevel level, T ...args) {
    if (level < m_level) return;

    m_derived.startMessage(getGlobalTimestamp());
    sendRemainingArguments(args...);
    m_derived.finishMessage();
  }

  /**
   * @brief validator for a format string.
   *
   * Doesn't require an implementation, since it is only
   * used for the format string validation.
   */
  inline void printfFmtValidator([[maybe_unused]] const char* fmt, ...)
    __attribute__((format(printf, 2, 3))) { }

  /**
   * @brief Sets the log level for the logger.
   *
   * Logs with levels above or equal the selected one are printed.
   * Others are ignored.
   */
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
 * @brief variadic template user defined literal to declare a debug interned string.
 *
 * This is currently only supported by clang.
 */
template<typename T, T... C>
Postform::InternedString operator ""_intern_debug() {
  return Postform::InternedString { decltype(InternedDebugString<C..., T{}>{})::string };
}

/**
 * @brief variadic template user defined literal to declare a info interned string.
 *
 * This is currently only supported by clang.
 */
template<typename T, T... C>
Postform::InternedString operator ""_intern_info() {
  return Postform::InternedString { decltype(InternedInfoString<C..., T{}>{})::string };
}

/**
 * @brief variadic template user defined literal to declare a warning interned string.
 *
 * This is currently only supported by clang.
 */
template<typename T, T... C>
Postform::InternedString operator ""_intern_warning() {
  return Postform::InternedString { decltype(InternedWarningString<C..., T{}>{})::string };
}

/**
 * @brief variadic template user defined literal to declare a error interned string.
 *
 * This is currently only supported by clang.
 */
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

/**
 * @brief Macro for a debug log with a printf-like formatting
 */
#define LOG_DEBUG(logger, fmt, ...) __LOG(Postform::LogLevel::DEBUG, _intern_debug, logger, fmt, ## __VA_ARGS__)
/**
 * @brief Macro for an info log with a printf-like formatting
 */
#define LOG_INFO(logger, fmt, ...) __LOG(Postform::LogLevel::INFO, _intern_info, logger, fmt, ## __VA_ARGS__)
/**
 * @brief Macro for a warning log with a printf-like formatting
 */
#define LOG_WARNING(logger, fmt, ...) __LOG(Postform::LogLevel::WARNING, _intern_warning, logger, fmt, ## __VA_ARGS__)
/**
 * @brief Macro for an error log with a printf-like formatting
 */
#define LOG_ERROR(logger, fmt, ...) __LOG(Postform::LogLevel::ERROR, _intern_error, logger, fmt, ## __VA_ARGS__)

#endif  // POSTFORM_LOGGER_H_
