/**
 * @file yi_soft_i2c.h
 * @brief YiCore soft i2c interface.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#ifndef YI_SOFT_I2C_H
#define YI_SOFT_I2C_H

#include "yi_i2c.h"

typedef struct
{
    yi_device_t *scl_gpio; /**< Scl gpio value. */
    yi_device_t *sda_gpio; /**< Sda gpio value. */
    uint32_t clock_frequency; /**< Clock frequency value. */
    uint32_t half_period_us; /**< Half period us value. */
    uint32_t stretch_timeout_us; /**< Stretch timeout us value. */
    uint8_t recovery_clocks; /**< Recovery clocks value. */} yi_soft_i2c_config_t;

typedef struct
{
    uint32_t transfer_count; /**< Transfer count value. */
    uint32_t error_count; /**< Error count value. */} yi_soft_i2c_data_t;

/**
 * @brief Initialize the module.
 * @param config Device configuration.
 */
int yi_soft_i2c_init(const void *config);
extern const yi_i2c_api_t yi_soft_i2c_api;

#define YI_SOFT_I2C_DEFINE_LEVEL(_name, _level, _priority, _config, _data) \
    YI_DEVICE_DEFINE_WITH_API(_name, _level, _priority,                  \
                              yi_soft_i2c_init, &_config, &_data,        \
                              (const yi_device_api_t *)&yi_soft_i2c_api)

#endif
