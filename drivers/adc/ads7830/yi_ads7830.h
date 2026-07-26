/**
 * @file yi_ads7830.h
 * @brief YiCore ads7830 interface.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#ifndef YI_ADS7830_H
#define YI_ADS7830_H

#include "yi_adc.h"
#include "yi_i2c.h"

#define YI_ADS7830_CHANNEL_COUNT 8U

typedef struct
{
    yi_device_t *self; /**< Self value. */
    yi_device_t *i2c; /**< I2c value. */
    uint8_t address; /**< Address value. */
    uint8_t default_channel; /**< Default channel value. */
    bool internal_reference; /**< Internal reference value. */
    uint16_t reference_mv; /**< Reference mv value. */
    uint32_t transfer_timeout_ms; /**< Transfer timeout ms value. */} yi_ads7830_config_t;

typedef struct
{
    uint32_t read_count; /**< Read count value. */
    uint32_t error_count; /**< Error count value. */} yi_ads7830_data_t;

/**
 * @brief Initialize the module.
 * @param config Device configuration.
 */
int yi_ads7830_init(const void *config);
extern const yi_adc_api_t yi_ads7830_api;

#define YI_ADS7830_DEFINE_LEVEL(_name, _level, _priority, _config, _data) \
    YI_DEVICE_DEFINE_WITH_API(                                           \
        _name, _level, _priority, yi_ads7830_init,                       \
        &_config, &_data, (const yi_device_api_t *)&yi_ads7830_api       \
    )

#endif
