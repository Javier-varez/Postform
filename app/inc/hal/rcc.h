
#ifndef RCC_H_
#define RCC_H_

#include <cstdint>

enum SystemClockSwitch {
  HSI = 0x00,
  HSE = 0x01,
  PLL = 0x02,
};

enum class AhbPrescaler {
  DIV_1 = 0x00,
  DIV_2 = 0x08,
  DIV_4 = 0x09,
  DIV_8 = 0x0a,
  DIV_16 = 0x0b,
  DIV_64 = 0x0c,
  DIV_128 = 0x0d,
  DIV_256 = 0x0e,
  DIV_512 = 0x0f,
};

enum class ApbPrescaler {
  DIV_1 = 0x00,
  DIV_2 = 0x04,
  DIV_4 = 0x05,
  DIV_8 = 0x06,
  DIV_16 = 0x07,
};

enum class PllSource {
  HSI_DIV_2 = 0,
  HSE = 1,
};

enum class PllHsePrescaler { DIV_1 = 0, DIV_2 = 1 };

enum class PllMultiplier {
  FACTOR_2 = 0x00,
  FACTOR_3 = 0x01,
  FACTOR_4 = 0x02,
  FACTOR_5 = 0x03,
  FACTOR_6 = 0x04,
  FACTOR_7 = 0x05,
  FACTOR_8 = 0x06,
  FACTOR_9 = 0x07,
  FACTOR_10 = 0x08,
  FACTOR_11 = 0x09,
  FACTOR_12 = 0x0a,
  FACTOR_13 = 0x0b,
  FACTOR_14 = 0x0c,
  FACTOR_15 = 0x0d,
  FACTOR_16 = 0x0e,
};

struct RccRegisters {
  union {
    uint32_t reg;
    struct {
      bool hsi_on : 1;
      bool hsi_ready : 1;
      uint32_t : 1;
      uint32_t hsi_trim : 5;
      uint32_t hsi_calibration : 8;
      bool hse_on : 1;
      bool hse_ready : 1;
      bool hse_bypass_oscilator : 1;
      uint32_t css_on : 1;
      uint32_t : 4;
      bool pll_on : 1;
      bool pll_ready : 1;
    } bits;
  } control_reg;
  union {
    uint32_t reg;
    struct {
      SystemClockSwitch system_clk_switch : 2;
      SystemClockSwitch system_clk_switch_status : 2;
      AhbPrescaler ahb_prescaler : 4;
      ApbPrescaler apb1_prescaler : 3;
      ApbPrescaler apb2_prescaler : 3;
      uint32_t adc_prescaler : 2;
      PllSource pll_source : 1;
      PllHsePrescaler pll_hse_divider : 1;
      PllMultiplier pll_multiplier : 4;
      uint32_t usb_prescaler : 1;
      uint32_t : 1;
      uint32_t micro_clk_out : 3;
    } bits;
  } clock_config_reg;
  uint32_t clock_interrupt_reg;
  uint32_t apb2_reset_reg;
  uint32_t apb1_reset_reg;
  uint32_t ahb_enable_reg;
  union {
    uint32_t reg;
    struct {
      uint32_t alternate_func_io_clk_enable : 1;
      uint32_t : 1;
      uint32_t port_a_clk_enable : 1;
      uint32_t port_b_clk_enable : 1;
      uint32_t port_c_clk_enable : 1;
      uint32_t port_d_clk_enable : 1;
      uint32_t port_e_clk_enable : 1;
      uint32_t port_f_clk_enable : 1;
      uint32_t port_g_clk_enable : 1;
      uint32_t adc1_clk_enable : 1;
      uint32_t adc2_clk_enable : 1;
      uint32_t tim1_clk_enable : 1;
      uint32_t spi1_clk_enable : 1;
      uint32_t tim8_clk_enable : 1;
      uint32_t usart1_clk_enable : 1;
      uint32_t adc3_clk_enable : 1;
      uint32_t : 3;
      uint32_t tim9_clk_enable : 1;
      uint32_t tim10_clk_enable : 1;
      uint32_t tim11_clk_enable : 1;
    } bits;
  } apb2_enable_reg;
  union {
    uint32_t reg;
    struct {
      uint32_t tim2_clk_enable : 1;
      uint32_t tim3_clk_enable : 1;
      uint32_t tim4_clk_enable : 1;
      uint32_t tim5_clk_enable : 1;
      uint32_t tim6_clk_enable : 1;
      uint32_t tim7_clk_enable : 1;
      uint32_t tim12_clk_enable : 1;
      uint32_t tim13_clk_enable : 1;
      uint32_t tim14_clk_enable : 1;
      uint32_t : 2;
      uint32_t wwdgen_clk_enable : 1;
      uint32_t : 2;
      uint32_t spi2_clk_enable : 1;
      uint32_t spi3_clk_enable : 1;
      uint32_t : 1;
      uint32_t usart2_clk_enable : 1;
      uint32_t usart3_clk_enable : 1;
      uint32_t uart4_clk_enable : 1;
      uint32_t uart5_clk_enable : 1;
      uint32_t i2c1_clk_enable : 1;
      uint32_t i2c2_clk_enable : 1;
      uint32_t usb_clk_enable : 1;
      uint32_t : 1;
      uint32_t can_clk_enable : 1;
      uint32_t : 1;
      uint32_t backup_clk_enable : 1;
      uint32_t power_clk_enable : 1;
      uint32_t dac_clk_enable : 1;
    } bits;
  } apb1_enable_reg;
};

#endif  // RCC_H_
