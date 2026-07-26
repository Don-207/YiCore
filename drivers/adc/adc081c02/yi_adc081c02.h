/**
 * @file yi_adc081c02.h
 * @brief YiCore ADC081C02 8-bit I2C ADC interface.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#ifndef YI_ADC081C02_H
#define YI_ADC081C02_H

#include "yi_adc.h"
#include "yi_i2c.h"

typedef struct {
    yi_device_t *self; /**< ADC device instance. */
    yi_device_t *i2c; /**< I2C bus device. */
    uint8_t address; /**< Seven-bit I2C address. */
    uint16_t reference_mv; /**< ADC reference in millivolts. */
    uint32_t transfer_timeout_ms; /**< I2C timeout. */
    uint8_t configuration; /**< Initial configuration register. */
    uint8_t low_limit; /**< Low alert threshold code. */
    uint8_t high_limit; /**< High alert threshold code. */
    uint8_t hysteresis; /**< Alert hysteresis code. */
} yi_adc081c02_config_t;

typedef struct {
    uint32_t read_count; /**< Successful sample count. */
    uint32_t error_count; /**< Transfer error count. */
} yi_adc081c02_data_t;

/** @brief Initialize the ADC081C02. */
int yi_adc081c02_init(const void *config);
/** @brief Read a sample in millivolts. */
int yi_adc081c02_read_mv(yi_device_t *dev, uint16_t *millivolts);
/** @brief Update the configuration register. */
int yi_adc081c02_set_configuration(yi_device_t *dev, uint8_t configuration);
/** @brief Set alert limits and hysteresis. */
int yi_adc081c02_set_limits(yi_device_t *dev, uint8_t low_limit,
                            uint8_t high_limit, uint8_t hysteresis);
/** @brief Read alert status. */
int yi_adc081c02_get_alert_status(yi_device_t *dev, uint8_t *status);
extern const yi_adc_api_t yi_adc081c02_api;

#define YI_ADC081C02_DEFINE_LEVEL(_name, _level, _priority, _config, _data) \
    YI_DEVICE_DEFINE_WITH_API(                                             \
        _name, _level, _priority, yi_adc081c02_init,                       \
        &_config, &_data, (const yi_device_api_t *)&yi_adc081c02_api)

#endif
