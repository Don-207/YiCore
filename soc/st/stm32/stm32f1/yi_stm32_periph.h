/**
 * @file yi_stm32_periph.h
 * @brief YiCore stm32 periph interface.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

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
    yi_stm32_bus_t bus; /**< Bus value. */
    uint32_t enable_mask; /**< Enable mask value. */} yi_stm32_periph_clock_t;

/**
 * @brief Enable the module.
 * @param clock Clock value.
 */
int yi_stm32_periph_clock_enable(const yi_stm32_periph_clock_t *clock);
/**
 * @brief Disable the module.
 * @param clock Clock value.
 */
int yi_stm32_periph_clock_disable(const yi_stm32_periph_clock_t *clock);
/**
 * @brief Perform the yi stm32 periph clock rate operation.
 * @param clock Clock value.
 */
uint32_t yi_stm32_periph_clock_rate(const yi_stm32_periph_clock_t *clock);

#endif
