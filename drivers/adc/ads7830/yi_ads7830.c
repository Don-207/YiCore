/**
 * @file yi_ads7830.c
 * @brief YiCore ads7830 implementation.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#include "yi_ads7830.h"

/**
 * @brief Perform the yi ads7830 command operation.
 * @param cfg Device configuration.
 * @param channel Channel value.
 */
static uint8_t yi_ads7830_command(const yi_ads7830_config_t *cfg,
                                  uint8_t channel)
{
    uint8_t selection = (uint8_t)(((channel << 2U) | (channel >> 1U)) & 0x07U);
    uint8_t power = cfg->internal_reference ? 0x0CU : 0x04U;
    return (uint8_t)(0x80U | (selection << 4U) | power);
}

/**
 * @brief Read channel.
 * @param dev Device instance.
 * @param channel Channel value.
 * @param value Value to process.
 * @param timeout_ms Operation timeout in milliseconds.
 */
static int yi_ads7830_read_channel(yi_device_t *dev, uint8_t channel,
                                   uint16_t *value, uint32_t timeout_ms)
{
    const yi_ads7830_config_t *cfg;
    yi_ads7830_data_t *data;
    uint8_t command;
    uint8_t sample;
    int result;

    if((dev == NULL) || (dev->config == NULL) || (dev->data == NULL) ||
       (value == NULL) || (channel >= YI_ADS7830_CHANNEL_COUNT) ||
       (timeout_ms == 0U))
    {
        return -1;
    }
    cfg = (const yi_ads7830_config_t *)dev->config;
    data = (yi_ads7830_data_t *)dev->data;
    command = yi_ads7830_command(cfg, channel);
    result = yi_i2c_master_write_read(cfg->i2c, cfg->address,
                                      &command, 1U, &sample, 1U, timeout_ms);
    if(result != 0)
    {
        data->error_count++;
        return -1;
    }
    data->read_count++;
    *value = sample;
    return 0;
}

/**
 * @brief Read the module.
 * @param dev Device instance.
 * @param value Value to process.
 * @param timeout_ms Operation timeout in milliseconds.
 */
static int yi_ads7830_read(yi_device_t *dev, uint16_t *value,
                           uint32_t timeout_ms)
{
    const yi_ads7830_config_t *cfg = (const yi_ads7830_config_t *)dev->config;
    /**
     * @brief Read channel.
     * @param dev Device instance.
     * @param default_channel Default channel value.
     * @param value Value to process.
     * @param timeout_ms Operation timeout in milliseconds.
     */
    return yi_ads7830_read_channel(dev, cfg->default_channel,
                                   value, timeout_ms);
}

/**
 * @brief Initialize the module.
 * @param config Device configuration.
 */
int yi_ads7830_init(const void *config)
{
    const yi_ads7830_config_t *cfg = config;
    yi_ads7830_data_t *data;

    if((cfg == NULL) || (cfg->self == NULL) || (cfg->self->data == NULL) ||
       !yi_device_is_ready(cfg->i2c) || (cfg->address < 0x48U) ||
       (cfg->address > 0x4BU) ||
       (cfg->default_channel >= YI_ADS7830_CHANNEL_COUNT) ||
       (cfg->reference_mv == 0U) || (cfg->transfer_timeout_ms == 0U))
    {
        return -1;
    }
    data = (yi_ads7830_data_t *)cfg->self->data;
    data->read_count = 0U;
    data->error_count = 0U;
    return 0;
}

const yi_adc_api_t yi_ads7830_api =
{
    .read = yi_ads7830_read,
    .read_channel = yi_ads7830_read_channel
};
