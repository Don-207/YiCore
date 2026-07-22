#include "yi_stm32_periph.h"

static volatile uint32_t *yi_stm32_enable_register(yi_stm32_bus_t bus)
{
    switch(bus)
    {
    case YI_STM32_BUS_APB1:
    case YI_STM32_BUS_APB1_TIMER:
        return &RCC->APB1ENR;
    case YI_STM32_BUS_APB2:
    case YI_STM32_BUS_APB2_TIMER:
        return &RCC->APB2ENR;
    default:
        return NULL;
    }
}

int yi_stm32_periph_clock_enable(const yi_stm32_periph_clock_t *clock)
{
    volatile uint32_t *reg;

    if((clock == NULL) || (clock->enable_mask == 0U))
    {
        return -1;
    }
    reg = yi_stm32_enable_register(clock->bus);
    if(reg == NULL)
    {
        return -1;
    }
    *reg |= clock->enable_mask;
    (void)*reg;
    return 0;
}

int yi_stm32_periph_clock_disable(const yi_stm32_periph_clock_t *clock)
{
    volatile uint32_t *reg;

    if(clock == NULL)
    {
        return -1;
    }
    reg = yi_stm32_enable_register(clock->bus);
    if(reg == NULL)
    {
        return -1;
    }
    *reg &= ~clock->enable_mask;
    return 0;
}

uint32_t yi_stm32_periph_clock_rate(const yi_stm32_periph_clock_t *clock)
{
    uint32_t pclk;
    uint32_t divider;

    if(clock == NULL)
    {
        return 0U;
    }
    if((clock->bus == YI_STM32_BUS_APB1) ||
       (clock->bus == YI_STM32_BUS_APB1_TIMER))
    {
        pclk = HAL_RCC_GetPCLK1Freq();
        divider = RCC->CFGR & RCC_CFGR_PPRE1;
    }
    else if((clock->bus == YI_STM32_BUS_APB2) ||
            (clock->bus == YI_STM32_BUS_APB2_TIMER))
    {
        pclk = HAL_RCC_GetPCLK2Freq();
        divider = RCC->CFGR & RCC_CFGR_PPRE2;
    }
    else
    {
        return 0U;
    }

    if((clock->bus == YI_STM32_BUS_APB1_TIMER) ||
       (clock->bus == YI_STM32_BUS_APB2_TIMER))
    {
        return (divider == 0U) ? pclk : (pclk * 2U);
    }
    return pclk;
}
