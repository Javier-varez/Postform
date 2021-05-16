MEMORY
{
  FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 64K
  RAM (rwx) : ORIGIN = 0x20000000, LENGTH = 20K
  UART2: ORIGIN = 0x40004400, LENGTH = 1K
  GPIOA: ORIGIN = 0x40010800, LENGTH = 1K
  RCC: ORIGIN = 0x40021000, LENGTH = 1K
  FLASH_REGS: ORIGIN = 0x40022000, LENGTH = 1K
}

SECTIONS
{
	.uart2_regs (NOLOAD) : {
		*(.uart2_regs)
	} > UART2

	.gpioa_regs (NOLOAD) : {
		*(.gpioa_regs)
	} > GPIOA

	.rcc_regs (NOLOAD) : {
		*(.rcc_regs)
	} > RCC

	.flash_regs (NOLOAD) : {
		*(.flash_regs)
	} > FLASH_REGS
}
