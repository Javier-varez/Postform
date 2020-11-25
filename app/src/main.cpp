
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

  uint32_t iteration = 0;
  while (true) {
    // All the logs here together act as the worst case for the decoder. Without proper framing
    // they would be confused from the host. If they all show that means that the COBS framing works
    LOG_DEBUG(&logger, "Iteration number: %d", iteration);
    LOG_DEBUG(&logger, "Is this %s or what?!", "nice");
    LOG_INFO(&logger, "I am %d years old...", 28);
    LOG_WARNING(&logger, "Third string! With multiple %s and more numbers: %d", "args", -1124);
    LOG_ERROR(&logger, "Oh boy, error %d just happened", 234556);

    // This log check what happens when we send something larger than 1024 bytes (buffer size)
    // It's also great to test the dummy zeroes, as it doesn't have any zeroes.
    LOG_DEBUG(&logger, "%s", "Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
                             "Proin congue, libero vitae condimentum egestas, tortor metus "
                             "condimentum augue, in pretium dolor purus quis lectus. Aenean "
                             "nunc sapien, eleifend quis convallis ut, venenatis quis mauris. "
                             "Morbi tempor, ex a lobortis luctus, sem nunc laoreet dolor, "
                             "pellentesque gravida mauris risus nec est. Aliquam ante sapien, "
                             "vehicula vel elementum at, feugiat quis libero. Nulla in lorem eu "
                             "erat vulputate efficitur. Etiam dapibus purus sed sagittis lobortis. "
                             "Sed quis porttitor nulla. Nulla in ante ac arcu semper efficitur ut "
                             "at erat. Fusce porttitor suscipit augue. Donec vel lorem justo. "
                             "Aenean id dolor quis erat blandit cursus. Aenean varius fringilla "
                             "eros vitae vestibulum.\n"
                             "Morbi tristique tristique nulla, at posuere ex sagittis at. Aliquam "
                             "est quam, porta nec erat ac, convallis tempus augue. Nam eu quam "
                             "vulputate, luctus sapien vel, tristique arcu. Suspendisse et ultrices "
                             "odio. Pellentesque consectetur lacus sapien, ut ornare odio sagittis "
                             "vel. Cras molestie eros odio, vitae ullamcorper ante vestibulum non. "
                             "Vestibulum facilisis diam vel condimentum gravida. Donec in odio sit "
                             "amet metus aliquet pharetra ac in ante. Phasellus sit amet dui vehicula, "
                             "tristique neque et, ullamcorper est. Integer ullamcorper risus in mattis "
                             "laoreet. Nullam dignissim vel ex vel molestie. Vestibulum id eleifend "
                             "metus. Curabitur malesuada condimentum augue ut molestie. Vivamus "
                             "pellentesque purus sed velit placerat ultricies. In ut erat diam. "
                             "Suspendisse potenti.");
    for (volatile int i = 0; i < 500000; i++) { }
    RegAccess<GPIO_PORTC + GPIO_BSRR_OFFSET>::writeRegister(1 << 13);
    for (volatile int i = 0; i < 500000; i++) { }
    RegAccess<GPIO_PORTC + GPIO_BSRR_OFFSET>::writeRegister(1 << 29);
    iteration++;
  }
  return 0;
}
