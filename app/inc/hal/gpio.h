
#ifndef GPIO_H_
#define GPIO_H_

#include <cstdint>

enum class GpioConfig : uint32_t {
  Analog = 0x00,
  FloatingInput = 0x01,
  InputWithPullUpDown = 0x02,
  PushPullOutput = 0x00,
  OpenDrainOutput = 0x01,
  PushPullAlternateFunc = 0x02,
  OpenDrainAlternateFunc = 0x03,
};

enum class GpioMode : uint32_t {
  Input = 0x00,
  OutputMedium = 0x01,
  OutputSlow = 0x02,
  OutputFast = 0x03,
};

struct GpioBankRegisters {
  union {
    uint32_t reg;
    struct {
      GpioMode mode0 : 2;
      GpioConfig config0 : 2;
      GpioMode mode1 : 2;
      GpioConfig config1 : 2;
      GpioMode mode2 : 2;
      GpioConfig config2 : 2;
      GpioMode mode3 : 2;
      GpioConfig config3 : 2;
      GpioMode mode4 : 2;
      GpioConfig config4 : 2;
      GpioMode mode5 : 2;
      GpioConfig config5 : 2;
      GpioMode mode6 : 2;
      GpioConfig config6 : 2;
      GpioMode mode7 : 2;
      GpioConfig config7 : 2;
      GpioMode mode8 : 2;
      GpioConfig config8 : 2;
      GpioMode mode9 : 2;
      GpioConfig config9 : 2;
      GpioMode mode10 : 2;
      GpioConfig config10 : 2;
      GpioMode mode11 : 2;
      GpioConfig config11 : 2;
      GpioMode mode12 : 2;
      GpioConfig config12 : 2;
      GpioMode mode13 : 2;
      GpioConfig config13 : 2;
      GpioMode mode14 : 2;
      GpioConfig config14 : 2;
      GpioMode mode15 : 2;
      GpioConfig config15 : 2;
    } bits;
  } control_reg_low;
  union {
    uint32_t reg;
    struct {
      GpioMode mode16 : 2;
      GpioConfig config16 : 2;
      GpioMode mode17 : 2;
      GpioConfig config17 : 2;
      GpioMode mode18 : 2;
      GpioConfig config18 : 2;
      GpioMode mode19 : 2;
      GpioConfig config19 : 2;
      GpioMode mode20 : 2;
      GpioConfig config20 : 2;
      GpioMode mode21 : 2;
      GpioConfig config21 : 2;
      GpioMode mode22 : 2;
      GpioConfig config22 : 2;
      GpioMode mode23 : 2;
      GpioConfig config23 : 2;
      GpioMode mode24 : 2;
      GpioConfig config24 : 2;
      GpioMode mode25 : 2;
      GpioConfig config25 : 2;
      GpioMode mode26 : 2;
      GpioConfig config26 : 2;
      GpioMode mode27 : 2;
      GpioConfig config27 : 2;
      GpioMode mode28 : 2;
      GpioConfig config28 : 2;
      GpioMode mode29 : 2;
      GpioConfig config29 : 2;
      GpioMode mode30 : 2;
      GpioConfig config30 : 2;
      GpioMode mode31 : 2;
      GpioConfig config31 : 2;
    } bits;
  } control_reg_high;
  union {
    uint32_t reg;
    struct {
      uint32_t pin0 : 1;
      uint32_t pin1 : 1;
      uint32_t pin2 : 1;
      uint32_t pin3 : 1;
      uint32_t pin4 : 1;
      uint32_t pin5 : 1;
      uint32_t pin6 : 1;
      uint32_t pin7 : 1;
      uint32_t pin8 : 1;
      uint32_t pin9 : 1;
      uint32_t pin10 : 1;
      uint32_t pin11 : 1;
      uint32_t pin12 : 1;
      uint32_t pin13 : 1;
      uint32_t pin14 : 1;
      uint32_t pin15 : 1;
    } bits;
  } input_reg;
  union {
    uint32_t reg;
    struct {
      uint32_t pin0 : 1;
      uint32_t pin1 : 1;
      uint32_t pin2 : 1;
      uint32_t pin3 : 1;
      uint32_t pin4 : 1;
      uint32_t pin5 : 1;
      uint32_t pin6 : 1;
      uint32_t pin7 : 1;
      uint32_t pin8 : 1;
      uint32_t pin9 : 1;
      uint32_t pin10 : 1;
      uint32_t pin11 : 1;
      uint32_t pin12 : 1;
      uint32_t pin13 : 1;
      uint32_t pin14 : 1;
      uint32_t pin15 : 1;
    } bits;
  } output_reg;
  union {
    uint32_t reg;
    struct {
      uint32_t set_pin0 : 1;
      uint32_t set_pin1 : 1;
      uint32_t set_pin2 : 1;
      uint32_t set_pin3 : 1;
      uint32_t set_pin4 : 1;
      uint32_t set_pin5 : 1;
      uint32_t set_pin6 : 1;
      uint32_t set_pin7 : 1;
      uint32_t set_pin8 : 1;
      uint32_t set_pin9 : 1;
      uint32_t set_pin10 : 1;
      uint32_t set_pin11 : 1;
      uint32_t set_pin12 : 1;
      uint32_t set_pin13 : 1;
      uint32_t set_pin14 : 1;
      uint32_t set_pin15 : 1;
      uint32_t reset_pin0 : 1;
      uint32_t reset_pin1 : 1;
      uint32_t reset_pin2 : 1;
      uint32_t reset_pin3 : 1;
      uint32_t reset_pin4 : 1;
      uint32_t reset_pin5 : 1;
      uint32_t reset_pin6 : 1;
      uint32_t reset_pin7 : 1;
      uint32_t reset_pin8 : 1;
      uint32_t reset_pin9 : 1;
      uint32_t reset_pin10 : 1;
      uint32_t reset_pin11 : 1;
      uint32_t reset_pin12 : 1;
      uint32_t reset_pin13 : 1;
      uint32_t reset_pin14 : 1;
      uint32_t reset_pin15 : 1;
    } bits;
  } set_reset_reg;
  union {
    uint32_t reg;
    struct {
      uint32_t reset_pin0 : 1;
      uint32_t reset_pin1 : 1;
      uint32_t reset_pin2 : 1;
      uint32_t reset_pin3 : 1;
      uint32_t reset_pin4 : 1;
      uint32_t reset_pin5 : 1;
      uint32_t reset_pin6 : 1;
      uint32_t reset_pin7 : 1;
      uint32_t reset_pin8 : 1;
      uint32_t reset_pin9 : 1;
      uint32_t reset_pin10 : 1;
      uint32_t reset_pin11 : 1;
      uint32_t reset_pin12 : 1;
      uint32_t reset_pin13 : 1;
      uint32_t reset_pin14 : 1;
      uint32_t reset_pin15 : 1;
    } bits;
  } reset_reg;
};

#endif  // GPIO_H_
