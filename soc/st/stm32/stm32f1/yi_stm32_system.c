#include "yi_stm32_system.h"
#include "yi_system.h"
#include "stm32f1xx_hal.h"

int yi_system_init(void)
{
    RCC_OscInitTypeDef oscillator = {0};
    RCC_ClkInitTypeDef clock = {0};

    if(HAL_Init() != HAL_OK)
    {
        return -1;
    }

    oscillator.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    oscillator.HSEState = RCC_HSE_ON;
    oscillator.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    oscillator.HSIState = RCC_HSI_ON;
    oscillator.PLL.PLLState = RCC_PLL_ON;
    oscillator.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    oscillator.PLL.PLLMUL = RCC_PLL_MUL9;
    if(HAL_RCC_OscConfig(&oscillator) != HAL_OK)
    {
        return -1;
    }

    clock.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                      RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clock.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clock.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clock.APB1CLKDivider = RCC_HCLK_DIV2;
    clock.APB2CLKDivider = RCC_HCLK_DIV1;
    if(HAL_RCC_ClockConfig(&clock, FLASH_LATENCY_2) != HAL_OK)
    {
        return -1;
    }
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    return 0;
}

uint32_t yi_system_uptime_ms(void)
{
    return HAL_GetTick();
}

uint32_t yi_system_uptime_us(void)
{
    uint32_t cycles_per_us = SystemCoreClock / 1000000U;
    return (cycles_per_us != 0U) ? (DWT->CYCCNT / cycles_per_us) : 0U;
}

void yi_system_delay_ms(uint32_t delay_ms)
{
    HAL_Delay(delay_ms);
}

void yi_system_delay_us(uint32_t delay_us)
{
    uint32_t cycles_per_us = SystemCoreClock / 1000000U;
    uint32_t start = DWT->CYCCNT;
    uint32_t wait_cycles = delay_us * cycles_per_us;
    while((uint32_t)(DWT->CYCCNT - start) < wait_cycles) { }
}

void yi_system_irq_lock(void)
{
    __disable_irq();
}
