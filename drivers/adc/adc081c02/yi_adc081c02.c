/**
 * @file yi_adc081c02.c
 * @brief YiCore ADC081C02 implementation.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#include "yi_adc081c02.h"

#define ADC081C02_REG_CONVERSION  0x00U
#define ADC081C02_REG_ALERT       0x01U
#define ADC081C02_REG_CONFIG      0x02U
#define ADC081C02_REG_LOW_LIMIT   0x03U
#define ADC081C02_REG_HIGH_LIMIT  0x04U
#define ADC081C02_REG_HYSTERESIS  0x05U
#define ADC081C02_RESULT_SHIFT    4U

/** @brief Write an eight-bit register. */
static int yi_adc081c02_write8(const yi_adc081c02_config_t *cfg,
                               uint8_t reg, uint8_t value)
{
    uint8_t tx[2] = {reg, value};
    return yi_i2c_master_write(cfg->i2c, cfg->address, tx, sizeof(tx),
                               cfg->transfer_timeout_ms);
}

/** @brief Encode and write an eight-bit threshold register. */
static int yi_adc081c02_write_limit(const yi_adc081c02_config_t *cfg,
                                    uint8_t reg, uint8_t value)
{
    uint16_t encoded = (uint16_t)value << ADC081C02_RESULT_SHIFT;
    uint8_t tx[3] = {reg, (uint8_t)(encoded >> 8), (uint8_t)encoded};
    return yi_i2c_master_write(cfg->i2c, cfg->address, tx, sizeof(tx),
                               cfg->transfer_timeout_ms);
}

/** @brief Read bytes from a selected register. */
static int yi_adc081c02_read_register(const yi_adc081c02_config_t *cfg,
                                      uint8_t reg, uint8_t *data,
                                      uint16_t length, uint32_t timeout_ms)
{
    return yi_i2c_master_write_read(cfg->i2c, cfg->address, &reg, 1U,
                                    data, length, timeout_ms);
}

/** @brief Read the raw eight-bit conversion result. */
static int yi_adc081c02_read(yi_device_t *dev, uint16_t *value,
                             uint32_t timeout_ms)
{
    const yi_adc081c02_config_t *cfg;
    yi_adc081c02_data_t *data;
    uint8_t rx[2];
    if((dev == NULL) || (dev->config == NULL) || (dev->data == NULL) ||
       (value == NULL) || (timeout_ms == 0U)) { return -1; }
    cfg = (const yi_adc081c02_config_t *)dev->config;
    data = (yi_adc081c02_data_t *)dev->data;
    if(yi_adc081c02_read_register(cfg, ADC081C02_REG_CONVERSION,
                                  rx, sizeof(rx), timeout_ms) != 0) {
        data->error_count++;
        return -1;
    }
    *value = (uint16_t)((((uint16_t)rx[0] << 8) | rx[1]) >>
                        ADC081C02_RESULT_SHIFT) & 0xFFU;
    data->read_count++;
    return 0;
}

/** @brief Read and convert the sample to millivolts. */
int yi_adc081c02_read_mv(yi_device_t *dev, uint16_t *millivolts)
{
    const yi_adc081c02_config_t *cfg;
    uint16_t raw;
    if(!yi_device_is_ready(dev) || (dev->config == NULL) ||
       (millivolts == NULL))
    { return -1; }
    cfg = (const yi_adc081c02_config_t *)dev->config;
    if(yi_adc081c02_read(dev, &raw, cfg->transfer_timeout_ms) != 0) { return -1; }
    *millivolts = (uint16_t)(((uint32_t)raw * cfg->reference_mv + 127U) / 255U);
    return 0;
}

/** @brief Write the runtime configuration register. */
int yi_adc081c02_set_configuration(yi_device_t *dev, uint8_t configuration)
{
    const yi_adc081c02_config_t *cfg;
    if(!yi_device_is_ready(dev) || (dev->config == NULL)) { return -1; }
    cfg = (const yi_adc081c02_config_t *)dev->config;
    return yi_adc081c02_write8(cfg, ADC081C02_REG_CONFIG, configuration);
}

/** @brief Configure alert limits and hysteresis. */
int yi_adc081c02_set_limits(yi_device_t *dev, uint8_t low_limit,
                            uint8_t high_limit, uint8_t hysteresis)
{
    const yi_adc081c02_config_t *cfg;
    if(!yi_device_is_ready(dev) || (dev->config == NULL) ||
       (low_limit > high_limit)) { return -1; }
    cfg = (const yi_adc081c02_config_t *)dev->config;
    if(yi_adc081c02_write_limit(cfg, ADC081C02_REG_LOW_LIMIT, low_limit) != 0)
    { return -1; }
    if(yi_adc081c02_write_limit(cfg, ADC081C02_REG_HIGH_LIMIT, high_limit) != 0)
    { return -1; }
    return yi_adc081c02_write_limit(cfg, ADC081C02_REG_HYSTERESIS, hysteresis);
}

/** @brief Read the alert status register. */
int yi_adc081c02_get_alert_status(yi_device_t *dev, uint8_t *status)
{
    const yi_adc081c02_config_t *cfg;
    if(!yi_device_is_ready(dev) || (dev->config == NULL) || (status == NULL))
    { return -1; }
    cfg = (const yi_adc081c02_config_t *)dev->config;
    return yi_adc081c02_read_register(cfg, ADC081C02_REG_ALERT, status, 1U,
                                      cfg->transfer_timeout_ms);
}

/** @brief Initialize configuration and alert thresholds. */
int yi_adc081c02_init(const void *config)
{
    const yi_adc081c02_config_t *cfg = config;
    yi_adc081c02_data_t *data;
    if((cfg == NULL) || (cfg->self == NULL) || (cfg->self->data == NULL) ||
       !yi_device_is_ready(cfg->i2c) || (cfg->address > 0x7FU) ||
       (cfg->reference_mv == 0U) || (cfg->transfer_timeout_ms == 0U) ||
       (cfg->low_limit > cfg->high_limit)) { return -1; }
    if((yi_adc081c02_write8(cfg, ADC081C02_REG_CONFIG, cfg->configuration) != 0) ||
       (yi_adc081c02_write_limit(cfg, ADC081C02_REG_LOW_LIMIT, cfg->low_limit) != 0) ||
       (yi_adc081c02_write_limit(cfg, ADC081C02_REG_HIGH_LIMIT, cfg->high_limit) != 0) ||
       (yi_adc081c02_write_limit(cfg, ADC081C02_REG_HYSTERESIS, cfg->hysteresis) != 0))
    { return -1; }
    data = (yi_adc081c02_data_t *)cfg->self->data;
    data->read_count = 0U; data->error_count = 0U;
    return 0;
}

const yi_adc_api_t yi_adc081c02_api = {
    .read = yi_adc081c02_read,
    .read_channel = NULL
};
