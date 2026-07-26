/** @file yi_mcp4728.h @brief YiCore MCP4728 quad DAC interface. */
#ifndef YI_MCP4728_H
#define YI_MCP4728_H
#include "yi_dac.h"
#include "yi_i2c.h"
#define YI_MCP4728_CHANNEL_COUNT 4U
#define YI_MCP4728_MAX_CODE 4095U
typedef enum { YI_MCP4728_POWER_NORMAL = 0U, YI_MCP4728_POWER_1K = 1U,
    YI_MCP4728_POWER_100K = 2U, YI_MCP4728_POWER_500K = 3U
} yi_mcp4728_power_t;
typedef struct { uint16_t value; /**< 12-bit DAC code. */
    bool internal_reference; /**< Use internal 2.048 V reference. */
    bool gain_2x; /**< Enable 2x gain. */
    yi_mcp4728_power_t power; /**< Channel power mode. */
} yi_mcp4728_channel_config_t;
typedef struct {
    yi_device_t *self; /**< DAC device instance. */
    yi_device_t *i2c; /**< I2C bus. */
    uint8_t address; /**< Seven-bit I2C address. */
    uint8_t default_channel; /**< Generic DAC default channel. */
    uint16_t vdd_mv; /**< External-reference supply voltage. */
    uint32_t transfer_timeout_ms; /**< I2C timeout. */
    uint32_t eeprom_timeout_ms; /**< EEPROM completion timeout. */
    yi_mcp4728_channel_config_t channel[YI_MCP4728_CHANNEL_COUNT]; /**< Channel defaults. */
} yi_mcp4728_config_t;
typedef struct { yi_mcp4728_channel_config_t channel[YI_MCP4728_CHANNEL_COUNT]; /**< Cached channels. */
    uint32_t write_count; /**< Successful write count. */
    uint32_t error_count; /**< Transfer error count. */
} yi_mcp4728_data_t;
/** @brief Initialize the MCP4728. */
int yi_mcp4728_init(const void *config);
/** @brief Write one channel code. */
int yi_mcp4728_write_channel(yi_device_t *dev, uint8_t channel, uint16_t value);
/** @brief Write one channel in millivolts. */
int yi_mcp4728_write_channel_mv(yi_device_t *dev, uint8_t channel, uint16_t mv);
/** @brief Configure reference, gain, and power mode. */
int yi_mcp4728_configure_channel(yi_device_t *dev, uint8_t channel,
    bool internal_reference, bool gain_2x, yi_mcp4728_power_t power);
/** @brief Update all channels in one transaction. */
int yi_mcp4728_write_all(yi_device_t *dev, const uint16_t values[4]);
/** @brief Persist one channel in EEPROM. */
int yi_mcp4728_write_eeprom(yi_device_t *dev, uint8_t channel, uint16_t value,
    bool internal_reference, bool gain_2x, yi_mcp4728_power_t power);
extern const yi_dac_api_t yi_mcp4728_api;
#define YI_MCP4728_DEFINE_LEVEL(_name, _level, _priority, _config, _data) \
 YI_DEVICE_DEFINE_WITH_API(_name, _level, _priority, yi_mcp4728_init, &_config, \
 &_data, (const yi_device_api_t *)&yi_mcp4728_api)
#endif
