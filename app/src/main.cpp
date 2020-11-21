
#include <cstdint>

#include "rtt_logger.h"

template<std::uint32_t PTR>
class RegAccess {
 public:
  static void writeRegister(std::uint32_t value) {
    *reinterpret_cast<volatile uint32_t*>(PTR) = value;
  }
  static std::uint32_t readRegister() {
    return *reinterpret_cast<volatile uint32_t*>(PTR);
  }
};

constexpr uint32_t RCC_APB2_ENR = 0x40021018;
constexpr uint32_t GPIO_PORTC = 0x40011000;
constexpr uint32_t GPIO_CRH_OFFSET = 0x04;
constexpr uint32_t GPIO_BSRR_OFFSET = 0x10;

int main() {
  RttLogger logger;

  RegAccess<RCC_APB2_ENR>::writeRegister(0x10);
  RegAccess<GPIO_PORTC + GPIO_CRH_OFFSET>::writeRegister(1 << 20);
  while (true) {
    LOG_DEBUG(&logger, "Is this %s or what?!", "nice");
    for (volatile int i = 0; i < 500000; i++) { }
    RegAccess<GPIO_PORTC + GPIO_BSRR_OFFSET>::writeRegister(1 << 13);
    LOG_INFO(&logger, "I am %d years old...", 28);
    for (volatile int i = 0; i < 500000; i++) { }
    RegAccess<GPIO_PORTC + GPIO_BSRR_OFFSET>::writeRegister(1 << 29);
    LOG_DEBUG(&logger, "Third string! With multiple %s and more numbers: %d", "args", -1124);
    for (volatile int i = 0; i < 500000; i++) { }
  }
  return 0;
}
