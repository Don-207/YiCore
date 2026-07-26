/**
 * @file yi_tsys01.h
 * @brief YiCore tsys01 interface.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#ifndef YI_TSYS01_H
#define YI_TSYS01_H

#include <stdbool.h>
#include "yi_i2c.h"

#define YI_TSYS01_PROM_WORD_COUNT 8U
#define YI_TSYS01_DEFAULT_ADDRESS 0x77U

typedef struct
{
    yi_device_t *self; /**< Self value. */
    yi_device_t *i2c; /**< I2c value. */
    uint8_t address; /**< Address value. */
    uint32_t transfer_timeout_ms; /**< Transfer timeout ms value. */
    uint32_t conversion_delay_ms; /**< Conversion delay ms value. */
    uint32_t reset_delay_ms; /**< Reset delay ms value. */
    bool validate_prom_checksum; /**< Validate prom checksum value. */} yi_tsys01_config_t;

typedef struct
{
    uint16_t prom[YI_TSYS01_PROM_WORD_COUNT]; /**< Prom value. */
    uint32_t read_count; /**< Read count value. */
    uint32_t error_count; /**< Error count value. */
    uint8_t initialized; /**< Initialized value. */} yi_tsys01_data_t;

/**
 * @brief Initialize the module.
 * @param config Device configuration.
 */
int yi_tsys01_init(const void *config);
/**
 * @brief Read raw.
 * @param dev Device instance.
 * @param adc_raw Adc raw value.
 */
int yi_tsys01_read_raw(yi_device_t *dev, uint32_t *adc_raw);
/**
 * @brief Read the module.
 * @param dev Device instance.
 * @param temperature_mc Temperature mc value.
 */
int yi_tsys01_read(yi_device_t *dev, int32_t *temperature_mc);

#define YI_TSYS01_DEFINE_LEVEL(_name, _level, _priority, _config, _data) \
    YI_DEVICE_DEFINE_WITH_API(                                          \
        _name, _level, _priority, yi_tsys01_init,                       \
        &_config, &_data, NULL                                          \
    )

#endif
