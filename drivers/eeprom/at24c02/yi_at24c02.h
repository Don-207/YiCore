/**
 * @file yi_at24c02.h
 * @brief YiCore at24c02 interface.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#ifndef YI_AT24C02_H
#define YI_AT24C02_H

#include "yi_eeprom.h"
#include "yi_i2c.h"

#define YI_AT24C02_SIZE       256U
#define YI_AT24C02_PAGE_SIZE  8U

typedef struct
{
    /* Must remain first for the common EEPROM geometry accessors. */
    yi_eeprom_config_t eeprom; /**< Eeprom value. */
    yi_device_t *self; /**< Self value. */
    yi_device_t *i2c; /**< I2c value. */
    uint8_t address; /**< Address value. */
    uint32_t transfer_timeout_ms; /**< Transfer timeout ms value. */
    uint32_t write_timeout_ms; /**< Write timeout ms value. */} yi_at24c02_config_t;

typedef struct
{
    uint32_t read_count; /**< Read count value. */
    uint32_t write_count; /**< Write count value. */
    uint32_t error_count; /**< Error count value. */} yi_at24c02_data_t;

/**
 * @brief Initialize the module.
 * @param config Device configuration.
 */
int yi_at24c02_init(const void *config);
extern const yi_eeprom_api_t yi_at24c02_api;

#define YI_AT24C02_DEFINE_LEVEL(_name, _level, _priority, _config, _data) \
    YI_DEVICE_DEFINE_WITH_API(                                           \
        _name, _level, _priority, yi_at24c02_init,                       \
        &_config, &_data, (const yi_device_api_t *)&yi_at24c02_api       \
    )

#endif
