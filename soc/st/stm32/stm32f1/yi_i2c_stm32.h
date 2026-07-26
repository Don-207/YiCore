/**
 * @file yi_i2c_stm32.h
 * @brief YiCore i2c stm32 interface.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#ifndef YI_I2C_STM32_H
#define YI_I2C_STM32_H

#include "yi_i2c.h"
#include "yi_stm32_periph.h"
#include "stm32f1xx_hal.h"

typedef struct
{
    yi_device_t *self; /**< Self value. */
    I2C_TypeDef *instance; /**< Instance value. */
    yi_stm32_periph_clock_t clock; /**< Clock value. */
    yi_device_t *scl_pin; /**< Scl pin value. */
    yi_device_t *sda_pin; /**< Sda pin value. */
    uint32_t clock_frequency; /**< Clock frequency value. */
    IRQn_Type event_irqn; /**< Event irqn value. */
    IRQn_Type error_irqn; /**< Error irqn value. */
    uint8_t irq_priority; /**< Irq priority value. */} yi_i2c_stm32_config_t;

typedef struct
{
    I2C_HandleTypeDef hi2c; /**< Hi2c value. */} yi_i2c_stm32_data_t;

/**
 * @brief Initialize the module.
 * @param config Device configuration.
 */
int yi_i2c_stm32_init(const void *config);
/**
 * @brief Perform the yi i2c stm32 event irq handler operation.
 * @param dev Device instance.
 */
void yi_i2c_stm32_event_irq_handler(yi_device_t *dev);
/**
 * @brief Perform the yi i2c stm32 error irq handler operation.
 * @param dev Device instance.
 */
void yi_i2c_stm32_error_irq_handler(yi_device_t *dev);
extern const yi_i2c_api_t yi_i2c_stm32_api;

#define YI_I2C_STM32_DEFINE_LEVEL(_name, _level, _priority, _config, _data) \
    YI_DEVICE_DEFINE_WITH_API(_name, _level, _priority,                  \
                              yi_i2c_stm32_init, &_config, &_data,       \
                              (const yi_device_api_t *)&yi_i2c_stm32_api)

#endif
