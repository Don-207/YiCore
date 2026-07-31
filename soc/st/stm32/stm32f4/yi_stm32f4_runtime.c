/**
 * @file yi_stm32f4_runtime.c
 * @brief Provide STM32F407 Cortex-M exception hooks.
 * @author Don
 * @date 2026-07-31
 * @version 1.0.0
 */

#include "stm32f4xx_hal.h"

/**
 * @brief Advance the HAL millisecond time base.
 * @return None.
 * @note Runs in SysTick interrupt context and must remain bounded.
 */
void SysTick_Handler(void)
{
    HAL_IncTick();
}
