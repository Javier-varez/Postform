
#include <cstdint>
#include <cstdio>

#include "cortex_m_hal/systick.h"
#include "hal/flash.h"
#include "hal/gpio.h"
#include "hal/rcc.h"
#include "hal/uart.h"
#include "postform/rtt/transport.h"
#include "postform/serial_logger.h"
#include "postform/utils.h"

static volatile __attribute__((section(".uart2_regs")))
UartRegisters uart2_regs;
static volatile __attribute__((section(".gpioa_regs")))
GpioBankRegisters bank_a_regs;
static volatile __attribute__((section(".rcc_regs"))) RccRegisters rcc_regs;
static volatile __attribute__((section(".flash_regs")))
FlashRegisters flash_regs;

static UNINIT std::array<std::uint8_t, 1024> s_up_buffer;
static std::array<Postform::Rtt::ChannelDescriptor, 1> s_up_descriptors{
    {{"postform_channel", s_up_buffer}}};

extern "C" Postform::Rtt::ControlBlock<1, 0> _SEGGER_RTT{s_up_descriptors, {}};

Uart uart{&uart2_regs};

Postform::SerialLogger<Uart> uart_logger{&uart};

Postform::Rtt::Transport transport{&_SEGGER_RTT.up_channels[0]};
Postform::SerialLogger<Postform::Rtt::Transport> logger{&transport};

namespace Ditto {
void assert_failed(const char* condition, int line, const char* file) {
  LOG_ERROR(&logger,
            "Oh boy, something really bad happened! "
            "Condition `%s` failed in file `%s`, line %d",
            condition, file, line);
  while (true) {
  }
}
}  // namespace Ditto

void configureClocks() {
  // Enable hse @ 8MHz
  rcc_regs.control_reg.bits.hse_on = 1;
  while (!rcc_regs.control_reg.bits.hse_ready) {
  }

  rcc_regs.clock_config_reg.bits.pll_source = PllSource::HSE;
  rcc_regs.clock_config_reg.bits.pll_hse_divider = PllHsePrescaler::DIV_1;
  rcc_regs.clock_config_reg.bits.pll_multiplier = PllMultiplier::FACTOR_9;
  rcc_regs.control_reg.bits.pll_on = true;
  while (!rcc_regs.control_reg.bits.pll_on) {
  }

  // Set proper flash latency and enable prefetch. This needs to be done before
  // switching to the PLL
  flash_regs.access_control_reg.reg = 0x32;

  // AHB 72 MHz, APB1 36 MHz, APB2 72 MHz
  rcc_regs.clock_config_reg.bits.ahb_prescaler = AhbPrescaler::DIV_1;
  rcc_regs.clock_config_reg.bits.apb1_prescaler = ApbPrescaler::DIV_2;
  rcc_regs.clock_config_reg.bits.apb2_prescaler = ApbPrescaler::DIV_1;

  rcc_regs.clock_config_reg.bits.system_clk_switch = SystemClockSwitch::PLL;
  while (rcc_regs.clock_config_reg.bits.system_clk_switch_status !=
         SystemClockSwitch::PLL) {
  }
}

void configureUart() {
  rcc_regs.apb2_enable_reg.bits.port_a_clk_enable = true;
  rcc_regs.apb1_enable_reg.bits.usart2_clk_enable = true;

  bank_a_regs.control_reg_low.bits.config2 = GpioConfig::PushPullAlternateFunc;
  bank_a_regs.control_reg_low.bits.mode2 = GpioMode::OutputSlow;
  // APB1 works at 1/2 HCLK
  uart.Init(36'000'000, 115'200);
}

void initializeHardware() {
  configureClocks();
  configureUart();
}

int main() {
  initializeHardware();

  SysTick& systick = SysTick::getInstance();

  const uint32_t systick_clk_hz = 72'000'000;
  systick.init(systick_clk_hz);

  logger.setLevel(Postform::LogLevel::DEBUG);

  uint32_t iteration = 0;
  while (true) {
    // All the logs here together act as the worst case for the decoder. Without
    // proper framing they would be confused from the host. If they all show
    // that means that the COBS framing works
    LOG_DEBUG(&logger, "Iteration number: %u", iteration);
    LOG_DEBUG(&uart_logger, "The UART works too!");
    LOG_DEBUG(&logger, "Is this %s or what?!", "nice");
    LOG_INFO(&logger, "I am %d years old...", 28);
    LOG_WARNING(&logger, "Third string! With multiple %s and more numbers: %d",
                "args", -1124);
    LOG_ERROR(&logger, "Oh boy, error %d just happened", 234556);
    char char_array[] = "123";
    LOG_ERROR(&logger, "This is my char array: %s", char_array);
    LOG_ERROR(&logger, "different unsigned sizes: %hhu, %hu, %u, %lu, %llu",
              static_cast<unsigned char>(123),
              static_cast<unsigned short>(43212),
              static_cast<unsigned int>(123123123),
              static_cast<unsigned long>(123123123),
              static_cast<unsigned long long>(123123123));
    LOG_ERROR(&logger, "different signed sizes: %hhd, %hd, %d, %ld, %lld",
              static_cast<signed char>(-123), static_cast<short>(-13212),
              static_cast<int>(-123123123), static_cast<long>(-123123123),
              static_cast<long long>(-123123123));
    LOG_ERROR(&logger, "different octal sizes: %hho, %ho, %o, %lo, %llo",
              static_cast<unsigned char>(0123),
              static_cast<unsigned short>(0123),
              static_cast<unsigned int>(0123123),
              static_cast<unsigned long>(0123123123),
              static_cast<unsigned long long>(0123123123));
    LOG_ERROR(&logger, "different hex sizes: %hhx, %hx, %x, %lx, %llx",
              static_cast<unsigned char>(0xf3),
              static_cast<unsigned short>(0x1321),
              static_cast<unsigned int>(0x12341235),
              static_cast<unsigned long>(0x12341234),
              static_cast<unsigned long long>(0x1234567812345678));
    LOG_ERROR(&logger, "Pointer %p", reinterpret_cast<void*>(0x12341234));

    constexpr auto interned_string =
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
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
        "amet metus aliquet pharetra ac in ante. Phasellus sit amet dui "
        "vehicula, "
        "tristique neque et, ullamcorper est. Integer ullamcorper risus in "
        "mattis "
        "laoreet. Nullam dignissim vel ex vel molestie. Vestibulum id eleifend "
        "metus. Curabitur malesuada condimentum augue ut molestie. Vivamus "
        "pellentesque purus sed velit placerat ultricies. In ut erat diam. "
        "Suspendisse potenti."_intern;

    LOG_DEBUG(&logger,
              "Now if I wanted to print a really long text I can use %%k: %k",
              interned_string);
    DITTO_VERIFY(iteration < 5);
    systick.delay(SysTick::TICKS_PER_SECOND);
    iteration++;
  }
  return 0;
}
