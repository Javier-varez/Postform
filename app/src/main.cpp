
#include <cstdint>

#include "uart_logger.h"

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
  UartLogger logger;
  LOG_DEBUG(&logger, "Ooopsie");
  LOG_DEBUG_ARGS(&logger, "Ooops%die2", 23);
  LOG_DEBUG_ARGS(&logger, "Ooops%die2", 123);
  RegAccess<RCC_APB2_ENR>::writeRegister(0x10);
  RegAccess<GPIO_PORTC + GPIO_CRH_OFFSET>::writeRegister(1 << 20);
  while (true) {
    RegAccess<GPIO_PORTC + GPIO_BSRR_OFFSET>::writeRegister(1 << 13);
    RegAccess<GPIO_PORTC + GPIO_BSRR_OFFSET>::writeRegister(1 << 29);
  }
  return 0;
}

