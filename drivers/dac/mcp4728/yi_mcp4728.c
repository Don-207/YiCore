/** @file yi_mcp4728.c @brief YiCore MCP4728 implementation. */
#include "yi_mcp4728.h"
#include "yi_system.h"
#include <string.h>
#define MCP4728_CMD_MULTI_WRITE  0x40U
#define MCP4728_CMD_SINGLE_WRITE 0x58U
#define MCP4728_INTERNAL_REF_MV  2048U
/** @brief Validate an MCP4728 device object. */
static bool mcp4728_valid(yi_device_t *dev)
{ return dev != NULL && dev->config != NULL && dev->data != NULL; }
/** @brief Send a three-byte channel configuration command. */
static int mcp4728_write_config(yi_device_t *dev, uint8_t channel, uint8_t command,
    uint16_t value, bool internal, bool gain_2x, yi_mcp4728_power_t power)
{
    const yi_mcp4728_config_t *cfg = dev->config; uint8_t tx[3];
    if(channel >= 4U || value > 4095U || power > 3U) return -1;
    tx[0] = (uint8_t)(command | (channel << 1));
    tx[1] = (uint8_t)((internal ? 0x80U : 0U) | ((uint8_t)power << 5) |
                      (gain_2x ? 0x10U : 0U) | (value >> 8));
    tx[2] = (uint8_t)value;
    return yi_i2c_master_write(cfg->i2c, cfg->address, tx, sizeof(tx),
                               cfg->transfer_timeout_ms);
}
/** @brief Write one volatile channel value. */
int yi_mcp4728_write_channel(yi_device_t *dev, uint8_t channel, uint16_t value)
{
    yi_mcp4728_data_t *data; yi_mcp4728_channel_config_t *state;
    if(!mcp4728_valid(dev) || channel >= 4U || value > 4095U) return -1;
    data = dev->data; state = &data->channel[channel];
    if(mcp4728_write_config(dev, channel, MCP4728_CMD_MULTI_WRITE, value,
       state->internal_reference, state->gain_2x, state->power) != 0)
    { data->error_count++; return -1; }
    state->value = value; data->write_count++; return 0;
}
/** @brief Adapt the default channel to the generic DAC API. */
static int mcp4728_dac_write(yi_device_t *dev, uint16_t value)
{ const yi_mcp4728_config_t *cfg;
  if(!mcp4728_valid(dev)) return -1; cfg = dev->config;
  return yi_mcp4728_write_channel(dev, cfg->default_channel, value); }
/** @brief Configure one channel reference, gain, and power mode. */
int yi_mcp4728_configure_channel(yi_device_t *dev, uint8_t channel,
    bool internal_reference, bool gain_2x, yi_mcp4728_power_t power)
{
    yi_mcp4728_data_t *data; yi_mcp4728_channel_config_t *state;
    if(!mcp4728_valid(dev) || channel >= 4U || power > 3U) return -1;
    data = dev->data; state = &data->channel[channel];
    if(mcp4728_write_config(dev, channel, MCP4728_CMD_MULTI_WRITE, state->value,
       internal_reference, gain_2x, power) != 0) return -1;
    state->internal_reference = internal_reference; state->gain_2x = gain_2x;
    state->power = power; return 0;
}
/** @brief Convert millivolts and write one channel. */
int yi_mcp4728_write_channel_mv(yi_device_t *dev, uint8_t channel, uint16_t mv)
{
    const yi_mcp4728_config_t *cfg; yi_mcp4728_channel_config_t *state;
    uint32_t full_scale, code;
    if(!mcp4728_valid(dev) || channel >= 4U) return -1;
    cfg = dev->config; state = &((yi_mcp4728_data_t *)dev->data)->channel[channel];
    full_scale = state->internal_reference ? MCP4728_INTERNAL_REF_MV : cfg->vdd_mv;
    if(state->internal_reference && state->gain_2x) full_scale *= 2U;
    if(mv > full_scale) return -1;
    code = ((uint32_t)mv * 4095U + full_scale / 2U) / full_scale;
    return yi_mcp4728_write_channel(dev, channel, (uint16_t)code);
}
/** @brief Update all four channels in one fast-write transaction. */
int yi_mcp4728_write_all(yi_device_t *dev, const uint16_t values[4])
{
    const yi_mcp4728_config_t *cfg; yi_mcp4728_data_t *data; uint8_t tx[8];
    if(!mcp4728_valid(dev) || values == NULL) return -1; cfg = dev->config; data = dev->data;
    for(uint8_t i=0U;i<4U;i++) {
        if(values[i] > 4095U) return -1;
        tx[i*2U] = (uint8_t)(((uint8_t)data->channel[i].power << 4) | (values[i] >> 8));
        tx[i*2U+1U] = (uint8_t)values[i];
    }
    if(yi_i2c_master_write(cfg->i2c, cfg->address, tx, sizeof(tx),
                           cfg->transfer_timeout_ms) != 0) return -1;
    for(uint8_t i=0U;i<4U;i++) data->channel[i].value = values[i];
    data->write_count++; return 0;
}
/** @brief Persist one channel configuration in EEPROM. */
int yi_mcp4728_write_eeprom(yi_device_t *dev, uint8_t channel, uint16_t value,
    bool internal_reference, bool gain_2x, yi_mcp4728_power_t power)
{
    const yi_mcp4728_config_t *cfg; yi_mcp4728_data_t *data; uint8_t probe;
    uint32_t started;
    if(!mcp4728_valid(dev)) return -1; cfg = dev->config; data = dev->data;
    if(mcp4728_write_config(dev, channel, MCP4728_CMD_SINGLE_WRITE, value,
       internal_reference, gain_2x, power) != 0) return -1;
    started = yi_system_uptime_ms();
    do {
        if(yi_i2c_master_read(cfg->i2c, cfg->address, &probe, 1U,
                              cfg->transfer_timeout_ms) == 0) {
            data->channel[channel] = (yi_mcp4728_channel_config_t){value,
                internal_reference, gain_2x, power}; data->write_count++; return 0;
        }
        yi_system_delay_ms(1U);
    } while((uint32_t)(yi_system_uptime_ms()-started) < cfg->eeprom_timeout_ms);
    data->error_count++; return -1;
}
/** @brief Initialize all volatile channel configurations. */
int yi_mcp4728_init(const void *config)
{
    const yi_mcp4728_config_t *cfg = config; yi_mcp4728_data_t *data;
    if(cfg == NULL || cfg->self == NULL || cfg->self->data == NULL ||
       !yi_device_is_ready(cfg->i2c) || cfg->address < 0x60U || cfg->address > 0x67U ||
       cfg->default_channel >= 4U || cfg->vdd_mv == 0U ||
       cfg->transfer_timeout_ms == 0U || cfg->eeprom_timeout_ms == 0U) return -1;
    for(uint8_t i=0U;i<4U;i++) if(cfg->channel[i].value > 4095U ||
       cfg->channel[i].power > 3U) return -1;
    data = cfg->self->data; memset(data, 0, sizeof(*data));
    for(uint8_t i=0U;i<4U;i++) {
        data->channel[i] = cfg->channel[i];
        if(mcp4728_write_config(cfg->self, i, MCP4728_CMD_MULTI_WRITE,
           cfg->channel[i].value, cfg->channel[i].internal_reference,
           cfg->channel[i].gain_2x, cfg->channel[i].power) != 0) return -1;
    }
    data->write_count = 1U; return 0;
}
const yi_dac_api_t yi_mcp4728_api = {.write=mcp4728_dac_write};
