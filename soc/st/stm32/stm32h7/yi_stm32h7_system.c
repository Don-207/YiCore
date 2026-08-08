/**
 * @file yi_stm32h7_system.c
 * @brief Initialize STM32H743 system services with caches and the HSI PLL.
 * @author Don
 * @date 2026-07-31
 * @version 1.0.0
 */

#include "stm32h7xx_hal.h"
#include "yi_system.h"

/**
 * @brief Initialize caches, HAL, LDO supply, and the 400 MHz HSI PLL clock.
 * @return Zero on success, otherwise -1.
 * @note Uses the internal oscillator and assumes the MCU uses its LDO supply.
 */
int yi_system_init(void)
{
    RCC_OscInitTypeDef oscillator = {0};
    RCC_ClkInitTypeDef clock = {0};

    SCB_EnableICache();
    SCB_EnableDCache();

    if(HAL_Init() != HAL_OK)
    {
        return -1;
    }
    if(HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY) != HAL_OK)
    {
        return -1;
    }
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY))
    {
    }

    oscillator.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    oscillator.HSIState = RCC_HSI_ON;
    oscillator.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    oscillator.PLL.PLLState = RCC_PLL_ON;
    oscillator.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    oscillator.PLL.PLLM = 4U;
    oscillator.PLL.PLLN = 50U;
    oscillator.PLL.PLLP = 2U;
    oscillator.PLL.PLLQ = 4U;
    oscillator.PLL.PLLR = 2U;
    oscillator.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
    oscillator.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
    oscillator.PLL.PLLFRACN = 0U;
    if(HAL_RCC_OscConfig(&oscillator) != HAL_OK)
    {
        return -1;
    }

    clock.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                      RCC_CLOCKTYPE_D1PCLK1 | RCC_CLOCKTYPE_PCLK1 |
                      RCC_CLOCKTYPE_PCLK2 | RCC_CLOCKTYPE_D3PCLK1;
    clock.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clock.SYSCLKDivider = RCC_SYSCLK_DIV1;
    clock.AHBCLKDivider = RCC_HCLK_DIV2;
    clock.APB3CLKDivider = RCC_APB3_DIV2;
    clock.APB1CLKDivider = RCC_APB1_DIV2;
    clock.APB2CLKDivider = RCC_APB2_DIV2;
    clock.APB4CLKDivider = RCC_APB4_DIV2;
    if(HAL_RCC_ClockConfig(&clock, FLASH_LATENCY_4) != HAL_OK)
    {
        return -1;
    }

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    return 0;
}

/** @brief Return the HAL millisecond tick. */
uint32_t yi_system_uptime_ms(void)
{
    return HAL_GetTick();
}

/** @brief Return elapsed DWT time in microseconds since counter enable. */
uint32_t yi_system_uptime_us(void)
{
    uint32_t cycles_per_us = SystemCoreClock / 1000000U;
    return (cycles_per_us != 0U) ? (DWT->CYCCNT / cycles_per_us) : 0U;
}

/** @brief Delay in milliseconds using the HAL time base. */
void yi_system_delay_ms(uint32_t delay_ms)
{
    HAL_Delay(delay_ms);
}

/** @brief Busy-wait for a number of microseconds using DWT. */
void yi_system_delay_us(uint32_t delay_us)
{
    uint32_t cycles_per_us = SystemCoreClock / 1000000U;
    uint32_t start = DWT->CYCCNT;
    uint32_t wait_cycles = delay_us * cycles_per_us;

    while((uint32_t)(DWT->CYCCNT - start) < wait_cycles)
    {
    }
}

/** @brief Disable maskable interrupts. */
void yi_system_irq_lock(void)
{
    __disable_irq();
}

/** @brief Save PRIMASK and disable maskable interrupts. */
uint32_t yi_system_irq_save(void)
{
    uint32_t key = __get_PRIMASK();
    __disable_irq();
    return key;
}

/** @brief Restore interrupts from a previously saved PRIMASK value. */
void yi_system_irq_restore(uint32_t key)
{
    if((key & 1U) == 0U)
    {
        __enable_irq();
    }
}
