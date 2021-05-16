
#ifndef UART_H_
#define UART_H_

#include <cstdint>

struct UartRegisters {
  union {
    uint32_t reg;
    struct {
      uint32_t parity_error : 1;
      uint32_t framing_error : 1;
      uint32_t noise_error : 1;
      uint32_t overrun_error : 1;
      uint32_t idle_line : 1;
      uint32_t rx_not_empty : 1;
      uint32_t tx_complete : 1;
      uint32_t tx_empty : 1;
      uint32_t lin_break : 1;
      uint32_t cts_flag : 1;
    } bits;
  } status_reg;
  union {
    uint32_t reg;
    struct {
      uint32_t value : 9;
    } bits;
  } data_reg;
  union {
    uint32_t reg;
    struct {
      uint32_t fraction : 4;
      uint32_t mantissa : 12;
    } bits;
  } baudrate_reg;
  union {
    uint32_t reg;
    struct {
      uint32_t send_break : 1;
      uint32_t receiver_wakeup : 1;
      uint32_t receiver_enable : 1;
      uint32_t transmitter_enable : 1;
      uint32_t idle_irq_enable : 1;
      uint32_t rx_not_empty_irq_enable : 1;
      uint32_t tx_complete_irq_enable : 1;
      uint32_t tx_empty_irq_enable : 1;
      uint32_t parity_error_irq_enable : 1;
      uint32_t parity_selection : 1;
      uint32_t parity_control_enable : 1;
      uint32_t wakeup : 1;
      uint32_t word_length : 1;
      uint32_t usart_enable : 1;
    } bits;
  } control_reg_1;
  union {
    uint32_t reg;
    struct {
      uint32_t address : 4;
      uint32_t : 1;
      uint32_t lin_break_detection : 1;
      uint32_t lin_break_detection_irq_enable : 1;
      uint32_t : 1;
      uint32_t last_bit_clk_pulse : 1;
      uint32_t clock_phase : 1;
      uint32_t clock_polarity : 1;
      uint32_t clock_enable : 1;
      uint32_t stop_bits : 2;
      uint32_t lin_enable : 1;
    } bits;
  } control_reg_2;
  union {
    uint32_t reg;
    struct {
      uint32_t error_irq_enable : 1;
      uint32_t irda_enable : 1;
      uint32_t irda_low_power : 1;
      uint32_t half_duplex_enable : 1;
      uint32_t smartcard_nack_enable : 1;
      uint32_t smartcard_mode_enable : 1;
      uint32_t dma_enable_rx : 1;
      uint32_t dma_enable_tx : 1;
      uint32_t rts_enable : 1;
      uint32_t cts_enable : 1;
      uint32_t cts_irq_enable : 1;
    } bits;
  } control_reg_3;
};

class Uart {
 public:
  Uart(volatile UartRegisters* regs) : m_regs(*regs) {}
  void Init(uint32_t clk_rate_mhz, uint32_t baudrate) {
    m_regs.control_reg_1.bits.usart_enable = true;
    m_regs.baudrate_reg.reg = clk_rate_mhz / baudrate;
    m_regs.control_reg_1.bits.transmitter_enable = true;
  }

  void write(uint8_t value) {
    while (!m_regs.status_reg.bits.tx_empty) {
    }
    m_regs.data_reg.reg = value;
  }
  void commit() {}

 private:
  volatile UartRegisters& m_regs;
};

#endif  // UART_H_
