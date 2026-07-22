#ifndef YI_TIMER_H
#define YI_TIMER_H

#include "yi_device.h"
#include "stm32f1xx_hal.h"
#include "yi_stm32_periph.h"

typedef struct
{
    yi_device_t *self;
    TIM_TypeDef *instance;
    yi_stm32_periph_clock_t clock;
    uint8_t counter_bits;
    uint32_t tick_frequency;
    IRQn_Type irqn;
    uint8_t irq_priority;
} yi_timer_config_t;

typedef struct
{
    TIM_HandleTypeDef htim;
    volatile uint32_t period_count;
    bool running;
} yi_timer_data_t;

int yi_timer_init(const void *config);
int yi_timer_start(yi_device_t *dev);
int yi_timer_stop(yi_device_t *dev);
uint32_t yi_timer_get_period_count(const yi_device_t *dev);
void yi_timer_irq_handler(yi_device_t *dev);

#define YI_TIMER_DEFINE_LEVEL(_name, _level, _priority, _config, _data) \
    YI_DEVICE_DEFINE_WITH_API(                                        \
        _name, _level, _priority, yi_timer_init,                      \
        &_config, &_data, NULL                                        \
    )

#endif
