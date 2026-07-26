/** @file yi_mcp4725.h @brief YiCore MCP4725 DAC interface. */
#ifndef YI_MCP4725_H
#define YI_MCP4725_H
#include "yi_dac.h"
#include "yi_i2c.h"
#define YI_MCP4725_MAX_CODE 4095U
typedef enum { YI_MCP4725_POWER_NORMAL = 0U, YI_MCP4725_POWER_1K = 1U,
    YI_MCP4725_POWER_100K = 2U, YI_MCP4725_POWER_500K = 3U
} yi_mcp4725_power_t;
typedef struct {
    yi_device_t *self; /**< DAC device instance. */
    yi_device_t *i2c; /**< I2C bus. */
    uint8_t address; /**< Seven-bit I2C address. */
    uint16_t reference_mv; /**< Full-scale voltage in millivolts. */
    uint16_t default_value; /**< Initial 12-bit code. */
    uint32_t transfer_timeout_ms; /**< I2C timeout. */
    uint32_t eeprom_timeout_ms; /**< EEPROM completion timeout. */
} yi_mcp4725_config_t;
typedef struct { uint16_t value; /**< Cached DAC code. */
    yi_mcp4725_power_t power; /**< Cached power mode. */
    uint32_t write_count; /**< Successful write count. */
    uint32_t error_count; /**< Transfer error count. */
} yi_mcp4725_data_t;
/** @brief Initialize the MCP4725. */
int yi_mcp4725_init(const void *config);
/** @brief Read output and status. */
int yi_mcp4725_read(yi_device_t *dev, uint16_t *value,
                    yi_mcp4725_power_t *power, bool *eeprom_ready);
/** @brief Write an output voltage in millivolts. */
int yi_mcp4725_write_mv(yi_device_t *dev, uint16_t millivolts);
/** @brief Select a power mode. */
int yi_mcp4725_set_power(yi_device_t *dev, yi_mcp4725_power_t power);
/** @brief Persist output state in EEPROM. */
int yi_mcp4725_write_eeprom(yi_device_t *dev, uint16_t value,
                            yi_mcp4725_power_t power);
extern const yi_dac_api_t yi_mcp4725_api;
#define YI_MCP4725_DEFINE_LEVEL(_name, _level, _priority, _config, _data) \
 YI_DEVICE_DEFINE_WITH_API(_name, _level, _priority, yi_mcp4725_init, &_config, \
 &_data, (const yi_device_api_t *)&yi_mcp4725_api)
#endif
