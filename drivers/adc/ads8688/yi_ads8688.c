/** @file yi_ads8688.c @brief YiCore ADS8688 implementation. */
#include "yi_ads8688.h"
#include "yi_system.h"
#include <string.h>
#define ADS8688_CMD_NOOP       0x0000U
#define ADS8688_CMD_STANDBY    0x8200U
#define ADS8688_CMD_POWER_DOWN 0x8300U
#define ADS8688_CMD_RESET      0x8500U
#define ADS8688_CMD_AUTO_RESET 0xA000U
#define ADS8688_CMD_MANUAL     0xC000U
#define ADS8688_REG_AUTO_SEQ   0x01U
#define ADS8688_REG_POWER_DOWN 0x02U
#define ADS8688_REG_RANGE_BASE 0x05U
#define ADS8688_REG_WRITE      0x0100U

/** @brief Validate an ADS8688 device object. */
static bool ads8688_valid(yi_device_t *dev)
{ return (dev != NULL) && (dev->config != NULL) && (dev->data != NULL); }
/** @brief Exchange one 32-clock command frame. */
static int ads8688_frame(const yi_ads8688_config_t *cfg, uint16_t command,
                         uint16_t *response)
{
    uint8_t tx[4] = {(uint8_t)(command >> 8), (uint8_t)command, 0U, 0U};
    uint8_t rx[4] = {0U}; int result = yi_spi_transceive(cfg->spi,
        &cfg->spi_config, tx, rx, sizeof(tx), cfg->transfer_timeout_ms);
    if((result == 0) && (response != NULL))
        *response = (uint16_t)(((uint16_t)rx[0] << 8) | rx[1]);
    return result;
}
/** @brief Check whether an input-range code is supported. */
static bool ads8688_range_valid(yi_ads8688_range_t range)
{ return (range <= YI_ADS8688_RANGE_PM_2V56) ||
         (range == YI_ADS8688_RANGE_0_10V24) || (range == YI_ADS8688_RANGE_0_5V12); }
/** @brief Write a program register. */
int yi_ads8688_write_register(yi_device_t *dev, uint8_t address, uint8_t value)
{
    const yi_ads8688_config_t *cfg; uint16_t command;
    if(!ads8688_valid(dev) || address > 0x7FU) return -1; cfg = dev->config;
    command = (uint16_t)(((uint16_t)address << 9) | ADS8688_REG_WRITE | value);
    if(ads8688_frame(cfg, command, NULL) != 0) { ((yi_ads8688_data_t *)dev->data)->error_count++; return -1; }
    return 0;
}
/** @brief Read a program register through the pipeline. */
int yi_ads8688_read_register(yi_device_t *dev, uint8_t address, uint8_t *value)
{
    const yi_ads8688_config_t *cfg; uint16_t response;
    if(!ads8688_valid(dev) || address > 0x7FU || value == NULL) return -1; cfg = dev->config;
    if(ads8688_frame(cfg, (uint16_t)address << 9, NULL) != 0 ||
       ads8688_frame(cfg, ADS8688_CMD_NOOP, &response) != 0) return -1;
    *value = (uint8_t)response; return 0;
}
/** @brief Send a standalone command. */
static int ads8688_command(yi_device_t *dev, uint16_t command)
{ return ads8688_frame((const yi_ads8688_config_t *)dev->config, command, NULL); }
/** @brief Reset the converter. */
int yi_ads8688_reset(yi_device_t *dev)
{ if(!ads8688_valid(dev) || ads8688_command(dev, ADS8688_CMD_RESET) != 0) return -1;
  yi_system_delay_us(20U); ((yi_ads8688_data_t *)dev->data)->auto_running = false; return 0; }
/** @brief Enter standby mode. */
int yi_ads8688_standby(yi_device_t *dev)
{ return ads8688_valid(dev) ? ads8688_command(dev, ADS8688_CMD_STANDBY) : -1; }
/** @brief Enter global power-down mode. */
int yi_ads8688_power_down(yi_device_t *dev)
{ return ads8688_valid(dev) ? ads8688_command(dev, ADS8688_CMD_POWER_DOWN) : -1; }
/** @brief Start the configured automatic sequence. */
int yi_ads8688_start_auto(yi_device_t *dev)
{ if(!ads8688_valid(dev) || ads8688_command(dev, ADS8688_CMD_AUTO_RESET) != 0) return -1;
  ((yi_ads8688_data_t *)dev->data)->auto_running = true; return 0; }
/** @brief Leave automatic-sequence state tracking. */
int yi_ads8688_stop_auto(yi_device_t *dev)
{ if(!ads8688_valid(dev)) return -1; ((yi_ads8688_data_t *)dev->data)->auto_running = false; return 0; }
/** @brief Clock out the next automatic-sequence result. */
int yi_ads8688_read_auto(yi_device_t *dev, uint16_t *value)
{
    const yi_ads8688_config_t *cfg; yi_ads8688_data_t *data;
    if(!ads8688_valid(dev) || value == NULL) return -1; cfg = dev->config; data = dev->data;
    if(!data->auto_running || ads8688_frame(cfg, ADS8688_CMD_NOOP, value) != 0) return -1;
    data->sample_count++; return 0;
}
/** @brief Select a channel and fetch its pipelined result. */
static int ads8688_read_channel(yi_device_t *dev, uint8_t channel,
                                uint16_t *value, uint32_t timeout_ms)
{
    const yi_ads8688_config_t *cfg; yi_ads8688_data_t *data; uint16_t command;
    (void)timeout_ms;
    if(!ads8688_valid(dev) || value == NULL || channel >= YI_ADS8688_CHANNEL_COUNT) return -1;
    cfg = dev->config; data = dev->data;
    if((cfg->power_down_mask & (1U << channel)) != 0U) return -1;
    command = (uint16_t)(ADS8688_CMD_MANUAL | ((uint16_t)channel << 9));
    if(ads8688_frame(cfg, command, NULL) != 0 ||
       ads8688_frame(cfg, ADS8688_CMD_NOOP, value) != 0) { data->error_count++; return -1; }
    data->active_channel = channel; data->sample_count++; return 0;
}
/** @brief Read the configured default channel. */
static int ads8688_read(yi_device_t *dev, uint16_t *value, uint32_t timeout_ms)
{ const yi_ads8688_config_t *cfg = dev->config;
  return ads8688_read_channel(dev, cfg->default_channel, value, timeout_ms); }
/** @brief Set one channel input range. */
int yi_ads8688_set_range(yi_device_t *dev, uint8_t channel, yi_ads8688_range_t range)
{ if(!ads8688_valid(dev) || channel >= 8U || !ads8688_range_valid(range)) return -1;
  return yi_ads8688_write_register(dev, (uint8_t)(ADS8688_REG_RANGE_BASE + channel), (uint8_t)range); }
/** @brief Initialize channel masks and input ranges. */
int yi_ads8688_init(const void *config)
{
    const yi_ads8688_config_t *cfg = config; yi_ads8688_data_t *data; uint8_t verify;
    if(cfg == NULL || cfg->self == NULL || cfg->self->data == NULL ||
       !yi_device_is_ready(cfg->spi) || !yi_device_is_ready(cfg->spi_config.cs_gpio) ||
       cfg->spi_config.mode != 1U || cfg->spi_config.frequency == 0U ||
       cfg->spi_config.frequency > 16000000U || cfg->transfer_timeout_ms == 0U ||
       cfg->default_channel >= 8U) return -1;
    for(uint8_t i=0U;i<8U;i++) if(!ads8688_range_valid(cfg->range[i])) return -1;
    data = cfg->self->data; memset(data, 0, sizeof(*data));
    if(yi_ads8688_reset(cfg->self) != 0 ||
       yi_ads8688_write_register(cfg->self, ADS8688_REG_AUTO_SEQ, cfg->auto_sequence_mask) != 0 ||
       yi_ads8688_write_register(cfg->self, ADS8688_REG_POWER_DOWN, cfg->power_down_mask) != 0) return -1;
    for(uint8_t i=0U;i<8U;i++) if(yi_ads8688_set_range(cfg->self, i, cfg->range[i]) != 0 ||
       yi_ads8688_read_register(cfg->self, (uint8_t)(ADS8688_REG_RANGE_BASE+i), &verify) != 0 ||
       verify != (uint8_t)cfg->range[i]) return -1;
    data->initialized = true; return 0;
}
const yi_adc_api_t yi_ads8688_api = {.read=ads8688_read,.read_channel=ads8688_read_channel};
