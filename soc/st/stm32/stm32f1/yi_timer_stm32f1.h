/**
 * @file yi_timer_stm32f1.h
 * @brief YiCore timer stm32f1 interface.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#ifndef YI_TIMER_STM32F1_H
#define YI_TIMER_STM32F1_H

#include "yi_timer.h"
#include "yi_stm32_periph.h"

typedef struct
{
    yi_device_t *self; /**< Self value. */
    TIM_TypeDef *instance; /**< Instance value. */
    yi_stm32_periph_clock_t clock; /**< Clock value. */
    uint8_t counter_bits; /**< Counter bits value. */
    uint32_t tick_frequency; /**< Tick frequency value. */
    IRQn_Type irqn; /**< Irqn value. */
    uint8_t irq_priority; /**< Irq priority value. */} yi_timer_config_t;

typedef struct
{
    TIM_HandleTypeDef htim; /**< Htim value. */
    volatile uint32_t period_count; /**< Period count value. */
    bool running; /**< Running value. */} yi_timer_data_t;

/**
 * @brief Initialize the module.
 * @param config Device configuration.
 */
int yi_timer_init(const void *config);

#define YI_TIMER_DEFINE_LEVEL(_name, _level, _priority, _config, _data) \
    YI_DEVICE_DEFINE_WITH_API(                                         \
        _name, _level, _priority, yi_timer_init,                       \
        &_config, &_data, NULL                                         \
    )

#endif
