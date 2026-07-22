#ifndef YI_I2C_STM32_H
#define YI_I2C_STM32_H

#include "yi_i2c.h"
#include "yi_stm32_periph.h"
#include "stm32f1xx_hal.h"

typedef struct
{
    yi_device_t *self;
    I2C_TypeDef *instance;
    yi_stm32_periph_clock_t clock;
    yi_device_t *scl_pin;
    yi_device_t *sda_pin;
    uint32_t clock_frequency;
    IRQn_Type event_irqn;
    IRQn_Type error_irqn;
    uint8_t irq_priority;
} yi_i2c_stm32_config_t;

typedef struct
{
    I2C_HandleTypeDef hi2c;
} yi_i2c_stm32_data_t;

int yi_i2c_stm32_init(const void *config);
void yi_i2c_stm32_event_irq_handler(yi_device_t *dev);
void yi_i2c_stm32_error_irq_handler(yi_device_t *dev);
extern const yi_i2c_api_t yi_i2c_stm32_api;

#define YI_I2C_STM32_DEFINE_LEVEL(_name, _level, _priority, _config, _data) \
    YI_DEVICE_DEFINE_WITH_API(_name, _level, _priority,                  \
                              yi_i2c_stm32_init, &_config, &_data,       \
                              (const yi_device_api_t *)&yi_i2c_stm32_api)

#endif
