#ifndef FORMAT_H_
#define FORMAT_H_

#include <cstdint>

template<class Derived>
class Logger {
public:
  template<typename T>
  inline void sendRemainingArguments(const T& first_arg) {
    sendArgument(first_arg);
  }

  template<typename T, typename ... Types>
  inline void sendRemainingArguments(const T& first_arg, Types... args) {
    sendArgument(first_arg);
    sendRemainingArguments(args...);
  }

  inline void printfFmtValidator(const char* fmt, ...) __attribute__ ((format (printf, 2, 3))) {
    (void) fmt;
  }

 private:
  template<typename T>
  inline void sendArgument(const T& argument) {
    Derived& derived = static_cast<Derived&>(*this);
    derived.addData(reinterpret_cast<const uint8_t*>(argument), sizeof(T));
  }
};

#define LOG_DEBUG(logger, fmt) \
    { \
        if (false) (logger)->printfFmtValidator(fmt); \
        __attribute__((section(".intern_strings"))) static const char string[] = fmt; \
        std::uint32_t fmt_id = reinterpret_cast<std::uint32_t>(string); \
        (logger)->sendRemainingArguments(fmt_id); \
    }

#define LOG_DEBUG_ARGS(logger, fmt, ...) \
    { \
        if (false) (logger)->printfFmtValidator(fmt, __VA_ARGS__); \
        __attribute__((section(".intern_strings"))) static const char string[] = fmt; \
        std::uint32_t fmt_id = reinterpret_cast<std::uint32_t>(string); \
        (logger)->sendRemainingArguments(fmt_id, __VA_ARGS__); \
    }

#endif

