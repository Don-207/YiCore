/**
 * @file yi_stm32f1_runtime.c
 * @brief Provide shared Cortex-M exception hooks for STM32F1 applications.
 * @author Don
 * @date 2026-07-28
 * @version 1.0.0
 */

#include "stm32f1xx_hal.h"

/**
 * @brief Advance the HAL millisecond time base.
 * @return None.
 * @note Runs in SysTick interrupt context and must remain bounded.
 */
void SysTick_Handler(void)
{
    HAL_IncTick();
}
