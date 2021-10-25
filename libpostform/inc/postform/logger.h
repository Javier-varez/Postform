#ifndef POSTFORM_LOGGER_H_
#define POSTFORM_LOGGER_H_

#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <span>
#include <type_traits>

#include "postform/args.h"
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
template <class Derived, class Writer>
class Logger {
 public:
  Logger() {
    extern volatile uint32_t dummy;
    static_cast<void>(dummy);
  }

  /**
   * @brief writes a log to the transport
   * @param level level of the current log.
   * @param args arguments to serialize in the log
   */
  template <typename... T>
  inline void log(LogLevel level, T... args) {
    if (level < m_level.load(std::memory_order_relaxed)) return;
    const auto arg_array = build_args(args...);
    vlog(arg_array);
  }

  /**
   * @brief Sets the log level for the logger.
   *
   * Logs with levels above or equal the selected one are printed.
   * Others are ignored.
   */
  void setLevel(LogLevel level) {
    m_level.store(level, std::memory_order_relaxed);
  }

 private:
  std::atomic<LogLevel> m_level = LogLevel::DEBUG;

  /**
   * @brief Creates a log with the supplied arguments.
   *        The arguments must include the format string.
   * @param arguments pointer to the argument array.
   * @param nargs number of arguments to log
   */
  void vlog(std::span<const Argument> arguments) {
    uint64_t timestamp = getGlobalTimestamp();

    Writer writer = static_cast<Derived&>(*this).getWriter({});
    writeLeb128(&writer, timestamp);
    for (const auto& argument : arguments) {
      switch (argument.type) {
        case Argument::Type::STRING_POINTER:
          writer.write(reinterpret_cast<const uint8_t*>(argument.str_ptr),
                       strlen(argument.str_ptr) + 1);
          break;
        case Argument::Type::UNSIGNED_INTEGER: {
          writeLeb128(&writer, argument.unsigned_long_long);
          break;
        }
        case Argument::Type::SIGNED_INTEGER: {
          writeLeb128(&writer, argument.signed_long_long);
          break;
        }
        case Argument::Type::INTERNED_STRING: {
          auto ptr = reinterpret_cast<uintptr_t>(argument.interned_string.str);
          writeLeb128(&writer, ptr);
          break;
        }
        case Argument::Type::VOID_PTR: {
          auto ptr = reinterpret_cast<uintptr_t>(argument.void_ptr);
          writeLeb128(&writer, ptr);
          break;
        }
      }
    }
  }

  template <class T,
            std::enable_if_t<std::is_integral_v<T> && std::is_unsigned_v<T>,
                             bool> = true>
  void writeLeb128(Writer* writer, T value) {
    constexpr std::size_t MAX_BUF_SIZE = (sizeof(T) * 8 + 6) / 7;
    uint8_t buffer[MAX_BUF_SIZE];
    uint32_t number_of_bytes = 0;

    do {
      buffer[number_of_bytes] = value & 0x7F;
      value >>= 7;
      if (value) {
        buffer[number_of_bytes] |= 0x80;
      }
      number_of_bytes++;
    } while (value);

    writer->write(buffer, number_of_bytes);
  }

  template <class T,
            std::enable_if_t<std::is_integral_v<T> && std::is_signed_v<T>,
                             bool> = true>
  void writeLeb128(Writer* writer, T value) {
    constexpr std::size_t MAX_BUF_SIZE = (sizeof(T) * 8 + 6) / 7;
    const bool negative = value < 0;
    uint8_t buffer[MAX_BUF_SIZE];
    uint32_t number_of_bytes = 0;

    bool done = false;
    do {
      buffer[number_of_bytes] = value & 0x7f;
      value >>= 7;
      if (negative) {
        value |= T{0x7f} << (sizeof(T) * 8 - 7);
      }
      if ((value == -1) && (buffer[number_of_bytes] & 0x40)) {
        done = true;
      } else if ((value == 0) && !(buffer[number_of_bytes] & 0x40)) {
        done = true;
      } else {
        buffer[number_of_bytes] |= 0x80;
      }
      number_of_bytes++;
    } while (!done);

    writer->write(buffer, number_of_bytes);
  }

  friend class LoggerTest;
};

/**
 * @brief Struct template representing an interned user string.
 *
 * Instantiates a string constant in the ".interned_strings.user" section.
 * This section is not used in runtime, only used as debug information for
 * Postform
 */
template <char... N>
struct InternedUserString {
  __attribute__((
      section(".interned_strings.user"))) static constexpr char string[]{N...};
};

template <char... N>
constexpr char InternedUserString<N...>::string[];

}  // namespace Postform

/**
 * @brief variadic template user defined literal to declare a error interned
 * string.
 *
 * This is currently only supported by clang.
 */
template <typename T, T... chars>
constexpr Postform::InternedString operator""_intern() {
#if !defined(__clang__) && defined(__GNUC__)
#pragma message(                                          \
    "Operator _intern is not supported on GCC.\n"         \
    "Section attributes for static member variables are " \
    "ignored in templated classes\n"                      \
    "For more information visit the tracking issue: "     \
    "https://github.com/Javier-varez/Postform/issues/79")
#endif
  return Postform::InternedString{
      Postform::InternedUserString<chars..., '\0'>::string};
}

#define __POSTFORM_LOG(level, intern_mode, logger, fmt, ...)                \
  {                                                                         \
    POSTFORM_ASSERT_FORMAT(fmt, ##__VA_ARGS__);                             \
    constexpr static char __attribute__((section(intern_mode)))             \
        pfmt_interned_string[] =                                            \
            __FILE__ "@" __POSTFORM_EXPAND_AND_STRINGIFY(__LINE__) "@" fmt; \
    (logger)->log(level, Postform::InternedString{pfmt_interned_string},    \
                  ##__VA_ARGS__);                                           \
  }

/**
 * @brief Macro for a debug log with a printf-like formatting
 */
#define LOG_DEBUG(logger, fmt, ...)                                            \
  __POSTFORM_LOG(Postform::LogLevel::DEBUG, ".interned_strings.debug", logger, \
                 fmt, ##__VA_ARGS__)

/**
 * @brief Macro for an info log with a printf-like formatting
 */
#define LOG_INFO(logger, fmt, ...)                                           \
  __POSTFORM_LOG(Postform::LogLevel::INFO, ".interned_strings.info", logger, \
                 fmt, ##__VA_ARGS__)

/**
 * @brief Macro for a warning log with a printf-like formatting
 */
#define LOG_WARNING(logger, fmt, ...)                                      \
  __POSTFORM_LOG(Postform::LogLevel::WARNING, ".interned_strings.warning", \
                 logger, fmt, ##__VA_ARGS__)
/**
 * @brief Macro for an error log with a printf-like formatting
 */
#define LOG_ERROR(logger, fmt, ...)                                            \
  __POSTFORM_LOG(Postform::LogLevel::ERROR, ".interned_strings.error", logger, \
                 fmt, ##__VA_ARGS__)

#endif  // POSTFORM_LOGGER_H_
