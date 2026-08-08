/**
 * @file yi_ch32h417_system.c
 * @brief Implement the YiCore system contract for CH32H417 V3F and V5F.
 * @author Don
 * @date 2026-07-31
 * @version 1.0.0
 */

#include "ch32h417.h"
#include "system_ch32h417.h"
#include "yi_riscv_irq.h"
#include "yi_system.h"

/** Saved boot-relative milliseconds accumulated by blocking YiCore delays. */
static uint32_t yi_ch32h417_uptime_ms;

/**
 * @brief Wait for a core-local SysTick compare event.
 * @param ticks Number of HCLK ticks to wait; zero returns immediately.
 * @note Thread context only; consumes the calling core's SysTick peripheral.
 */
static void yi_ch32h417_delay_ticks(uint32_t ticks)
{
    if (ticks == 0U) {
        return;
    }
#if defined(Core_V3F)
    SysTick0->ISR &= ~(1UL << 0);
    SysTick0->CNT = 0U;
    SysTick0->CMP = ticks;
    SysTick0->CTLR = (1UL << 2) | (1UL << 0);
    while ((SysTick0->ISR & (1UL << 0)) == 0U) { }
    SysTick0->CTLR &= ~(1UL << 0);
#elif defined(Core_V5F)
    SysTick0->ISR &= ~(1UL << 1);
    SysTick1->CNT = 0U;
    SysTick1->CMP = ticks;
    SysTick1->CTLR = (1UL << 2) | (1UL << 0);
    while ((SysTick0->ISR & (1UL << 1)) == 0U) { }
    SysTick1->CTLR &= ~(1UL << 0);
#else
#error "CH32H417 system backend requires Core_V3F or Core_V5F"
#endif
}

/**
 * @brief Initialize the calling core's YiCore system and clock view.
 * @return Zero after the calling core is ready.
 * @note V3F owns shared clock-tree setup; V5F only refreshes clock variables.
 */
int yi_system_init(void)
{
#if defined(Core_V3F)
    SystemInit();
#endif
    SystemAndCoreClockUpdate();
    yi_ch32h417_uptime_ms = 0U;
    return 0;
}

/** @brief Return elapsed milliseconds accumulated by YiCore blocking delays. */
uint32_t yi_system_uptime_ms(void)
{
    return yi_ch32h417_uptime_ms;
}

/** @brief Return elapsed microseconds at millisecond accounting resolution. */
uint32_t yi_system_uptime_us(void)
{
    return yi_ch32h417_uptime_ms * 1000U;
}

/**
 * @brief Block for a requested number of microseconds using core-local SysTick.
 * @param delay_us Requested delay in microseconds.
 * @note Thread context only; requires successful yi_system_init().
 */
void yi_system_delay_us(uint32_t delay_us)
{
    /** HCLK ticks corresponding to the requested microsecond interval. */
    uint32_t delay_ticks = (HCLKClock / 1000000U) * delay_us;

    yi_ch32h417_delay_ticks(delay_ticks);
}

/**
 * @brief Block for milliseconds using core-local SysTick and update uptime.
 * @param delay_ms Requested delay in milliseconds.
 * @note Thread context only; requires successful yi_system_init().
 */
void yi_system_delay_ms(uint32_t delay_ms)
{
    /** HCLK ticks corresponding to the requested millisecond interval. */
    uint32_t delay_ticks = (HCLKClock / 1000U) * delay_ms;

    yi_ch32h417_delay_ticks(delay_ticks);
    yi_ch32h417_uptime_ms += delay_ms;
}

/** @brief Disable current-core machine interrupts without preserving state. */
void yi_system_irq_lock(void)
{
    (void)yi_riscv_irq_lock();
}

/** @brief Disable current-core interrupts and return the previous MIE state. */
uint32_t yi_system_irq_save(void)
{
    return yi_riscv_irq_lock();
}

/** @brief Restore the interrupt state saved by yi_system_irq_save(). */
void yi_system_irq_restore(uint32_t key)
{
    yi_riscv_irq_unlock(key);
}
