#include <algorithm>
#include <cstdint>

std::uint32_t* const CORE_DEBUG_DHCSR =
    reinterpret_cast<std::uint32_t*>(0xE000EDF0UL);

extern std::uint8_t __StackInit;

extern "C"
void Reset_Handler() {
    // Initialize data section
    extern std::uint8_t __data_start__;
    extern std::uint8_t __data_end__;
    extern std::uint8_t __etext;
    std::size_t size = static_cast<size_t>(&__data_end__ - &__data_start__);
    std::copy(&__etext, &__etext + size, &__data_start__);

    // Initialize bss section
    extern std::uint8_t __bss_start__;
    extern std::uint8_t __bss_end__;
    std::fill(&__bss_start__, &__bss_end__, UINT8_C(0x00));

    // Initialize static objects by calling their constructors
    typedef void (*function_t)();
    extern function_t __init_array_start[0];
    extern function_t __init_array_end[0];
    std::for_each(__init_array_start, __init_array_end, [](function_t ptr) {
        ptr();
    });

    // Jump to main
    asm volatile (
        "msr msp, %[stack_top]\n"
        // We want to make sure that any stacking 
        // operations after this use the correct
        // stack pointer value
        "dsb\n" 
        "bl main"
        : : [stack_top] "r" (&__StackInit)
    );
}

#define DEFINE_DEFAULT_ISR(name) \
    extern "C" \
    __attribute__((interrupt)) \
    __attribute__((weak)) \
    __attribute__((noreturn)) \
    void name() { \
        volatile int32_t exception_number; \
        asm("mrs %[ipsr_reg], ipsr   \n" \
             : [ipsr_reg] "=r" (exception_number)); \
        exception_number -= 16; \
        if (*CORE_DEBUG_DHCSR & (1 << 0)) { \
            asm("bkpt #0"); \
        } \
        while(true); \
    }

DEFINE_DEFAULT_ISR(defaultISR)
DEFINE_DEFAULT_ISR(HardFault_Handler)
DEFINE_DEFAULT_ISR(SysTick_Handler)

// TODO :: Replace by proper NVIC table
volatile const std::uintptr_t g_pfnVectors[]
__attribute__((section(".isr_vector"))) {
    // Stack Ptr initialization
    reinterpret_cast<std::uintptr_t>(&__StackInit),
    // Entry point
    reinterpret_cast<std::uintptr_t>(Reset_Handler),
    // Exceptions
    reinterpret_cast<std::uintptr_t>(defaultISR),               /* NMI_Handler */
    reinterpret_cast<std::uintptr_t>(HardFault_Handler),        /* HardFault_Handler */
    reinterpret_cast<std::uintptr_t>(defaultISR),               /* MemManage_Handler */
    reinterpret_cast<std::uintptr_t>(defaultISR),               /* BusFault_Handler */
    reinterpret_cast<std::uintptr_t>(defaultISR),               /* UsageFault_Handler */
    reinterpret_cast<std::uintptr_t>(nullptr),                  /* Reserved */
    reinterpret_cast<std::uintptr_t>(nullptr),                  /* Reserved */
    reinterpret_cast<std::uintptr_t>(nullptr),                  /* Reserved */
    reinterpret_cast<std::uintptr_t>(nullptr),                  /* Reserved */
    reinterpret_cast<std::uintptr_t>(defaultISR),               /* SVC_Handler */
    reinterpret_cast<std::uintptr_t>(defaultISR),               /* DebugMon_Handler */
    reinterpret_cast<std::uintptr_t>(nullptr),                  /* Reserved */
    reinterpret_cast<std::uintptr_t>(defaultISR),               /* PendSV_Handler */
    reinterpret_cast<std::uintptr_t>(SysTick_Handler),          /* SysTick_Handler */
};

