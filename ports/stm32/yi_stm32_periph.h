#ifndef YI_STM32_PERIPH_H
#define YI_STM32_PERIPH_H

#include <stdint.h>
#include "stm32f1xx_hal.h"

typedef enum
{
    YI_STM32_BUS_APB1 = 0,
    YI_STM32_BUS_APB2,
    YI_STM32_BUS_APB1_TIMER,
    YI_STM32_BUS_APB2_TIMER
} yi_stm32_bus_t;

typedef struct
{
    yi_stm32_bus_t bus;
    uint32_t enable_mask;
} yi_stm32_periph_clock_t;

int yi_stm32_periph_clock_enable(const yi_stm32_periph_clock_t *clock);
int yi_stm32_periph_clock_disable(const yi_stm32_periph_clock_t *clock);
uint32_t yi_stm32_periph_clock_rate(const yi_stm32_periph_clock_t *clock);

#endif
