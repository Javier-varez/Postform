#ifndef FORMAT_H_
#define FORMAT_H_

#include <cstdint>

struct InternedString {
  const char* str;
};

template<class Derived>
class Logger {
 public:
  template<typename ... T>
  inline void log(T ...args) {
    Derived& derived = static_cast<Derived&>(*this);
    derived.startMessage();
    sendRemainingArguments(args...);
    derived.finishMessage();
  }

  inline void printfFmtValidator([[maybe_unused]] const char* fmt, ...)
    __attribute__((format(printf, 2, 3))) { }

 private:
  template<typename T>
  inline void sendArgument(const T argument) {
    Derived& derived = static_cast<Derived&>(*this);
    derived.appendData(reinterpret_cast<const uint8_t*>(&argument), sizeof(T));
  }

  template<>
  inline void sendArgument(const char* argument) {
    Derived& derived = static_cast<Derived&>(*this);
    derived.appendString(argument);
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

template<typename T, T... C>
InternedString operator ""_intern_debug() {
  return InternedString { decltype(InternedDebugString<C..., T{}>{})::string };
}

template<typename T, T... C>
InternedString operator ""_intern_info() {
  return InternedString { decltype(InternedInfoString<C..., T{}>{})::string };
}

#define LOG_DEBUG(logger, fmt, ...) \
  { \
    (logger)->printfFmtValidator(fmt, ## __VA_ARGS__); \
    (logger)->log(fmt ## _intern_debug, ## __VA_ARGS__); \
  }

#define LOG_INFO(logger, fmt, ...) \
  { \
    (logger)->printfFmtValidator(fmt, ## __VA_ARGS__); \
    (logger)->log(fmt ## _intern_info, ## __VA_ARGS__); \
  }

#endif

