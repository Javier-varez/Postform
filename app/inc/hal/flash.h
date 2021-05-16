
#ifndef FLASH_H_
#define FLASH_H_

#include <cstdint>

enum FlashLatency {
  WAIT_0 = 0,
  WAIT_1 = 1,
  WAIT_2 = 2,
};

struct FlashRegisters {
  union {
    uint32_t reg;
    struct {
      FlashLatency latency : 3;
      bool half_cycle_access_enable : 1;
      bool prefetch_buffer_enable : 1;
      bool prefetch_buffer_status : 1;
    } bits;
  } access_control_reg;
};

#endif  // FLASH_H_
