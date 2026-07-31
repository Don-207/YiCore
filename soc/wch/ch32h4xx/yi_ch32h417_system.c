/**
 * @file yi_ch32h417_system.c
 * @brief Implement the YiCore system contract for the CH32H417 V3F core.
 * @author Don
 * @date 2026-07-31
 * @version 1.0.0
 */

#include "ch32h417.h"
#include "system_ch32h417.h"
#include "yi_riscv_irq.h"
#include "yi_system.h"

/** Saved boot-relative millisecond count; no periodic timer is active yet. */
static uint32_t yi_ch32h417_uptime_ms;

/**
 * @brief Configure the vendor clock tree for the V3F bring-up image.
 * @return Zero after clock initialization.
 * @note Uses the vendor 25 MHz HSE profile and changes global clock registers.
 */
int yi_system_init(void)
{
    SystemInit();
    SystemAndCoreClockUpdate();
    yi_ch32h417_uptime_ms = 0U;
    return 0;
}

/** @brief Return elapsed milliseconds accumulated by YiCore blocking delays. */
uint32_t yi_system_uptime_ms(void)
{
    return yi_ch32h417_uptime_ms;
}

/** @brief Return approximate elapsed microseconds from blocking-delay accounting. */
uint32_t yi_system_uptime_us(void)
{
    return yi_ch32h417_uptime_ms * 1000U;
}

/**
 * @brief Busy-wait for an approximate number of microseconds.
 * @param delay_us Requested delay in microseconds.
 * @note Thread context only; timing depends on compiler output and V3F clock.
 */
void yi_system_delay_us(uint32_t delay_us)
{
    /** Approximate inner-loop count calibrated conservatively for RV32 V3F. */
    volatile uint32_t cycles = (SystemCoreClock / 5000000U) * delay_us;

    while (cycles != 0U) {
        __asm volatile("nop");
        cycles--;
    }
}

/**
 * @brief Busy-wait for milliseconds and update the bring-up uptime counter.
 * @param delay_ms Requested delay in milliseconds.
 * @note Thread context only; no hardware timer is configured in this stage.
 */
void yi_system_delay_ms(uint32_t delay_ms)
{
    /** Remaining whole milliseconds in the requested delay. */
    uint32_t remaining_ms = delay_ms;

    while (remaining_ms != 0U) {
        yi_system_delay_us(1000U);
        yi_ch32h417_uptime_ms++;
        remaining_ms--;
    }
}

/** @brief Disable V3F machine interrupts without preserving prior state. */
void yi_system_irq_lock(void)
{
    (void)yi_riscv_irq_lock();
}

/** @brief Disable V3F machine interrupts and return the previous MIE state. */
uint32_t yi_system_irq_save(void)
{
    return yi_riscv_irq_lock();
}

/** @brief Restore the V3F machine interrupt state saved by yi_system_irq_save(). */
void yi_system_irq_restore(uint32_t key)
{
    yi_riscv_irq_unlock(key);
}
