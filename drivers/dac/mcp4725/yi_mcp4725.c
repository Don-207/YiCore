/** @file yi_mcp4725.c @brief YiCore MCP4725 implementation. */
#include "yi_mcp4725.h"
#include "yi_system.h"
#define MCP4725_CMD_WRITE_DAC_EEPROM 0x60U
/** @brief Validate an MCP4725 device object. */
static bool mcp4725_valid(yi_device_t *dev)
{ return (dev != NULL) && (dev->config != NULL) && (dev->data != NULL); }
/** @brief Perform a volatile two-byte fast write. */
static int mcp4725_fast_write(yi_device_t *dev, uint16_t value,
                              yi_mcp4725_power_t power)
{
    const yi_mcp4725_config_t *cfg = dev->config; yi_mcp4725_data_t *data = dev->data;
    uint8_t tx[2];
    if(value > YI_MCP4725_MAX_CODE || power > YI_MCP4725_POWER_500K) return -1;
    tx[0] = (uint8_t)(((uint8_t)power << 4) | (value >> 8)); tx[1] = (uint8_t)value;
    if(yi_i2c_master_write(cfg->i2c, cfg->address, tx, sizeof(tx),
                           cfg->transfer_timeout_ms) != 0) { data->error_count++; return -1; }
    data->value = value; data->power = power; data->write_count++; return 0;
}
/** @brief Adapt MCP4725 output to the generic DAC API. */
static int mcp4725_dac_write(yi_device_t *dev, uint16_t value)
{ if(!mcp4725_valid(dev)) return -1;
  return mcp4725_fast_write(dev, value, ((yi_mcp4725_data_t *)dev->data)->power); }
/** @brief Read DAC value, power mode, and EEPROM-ready state. */
int yi_mcp4725_read(yi_device_t *dev, uint16_t *value,
                    yi_mcp4725_power_t *power, bool *eeprom_ready)
{
    const yi_mcp4725_config_t *cfg; uint8_t rx[3];
    if(!mcp4725_valid(dev) || value == NULL) return -1; cfg = dev->config;
    if(yi_i2c_master_read(cfg->i2c, cfg->address, rx, sizeof(rx),
                          cfg->transfer_timeout_ms) != 0) return -1;
    *value = (uint16_t)(((uint16_t)rx[1] << 4) | (rx[2] >> 4));
    if(power != NULL) *power = (yi_mcp4725_power_t)((rx[0] >> 1) & 3U);
    if(eeprom_ready != NULL) *eeprom_ready = (rx[0] & 0x80U) != 0U;
    return 0;
}
/** @brief Convert millivolts to a 12-bit code and write it. */
int yi_mcp4725_write_mv(yi_device_t *dev, uint16_t millivolts)
{
    const yi_mcp4725_config_t *cfg; uint32_t value;
    if(!mcp4725_valid(dev)) return -1; cfg = dev->config;
    if(millivolts > cfg->reference_mv) return -1;
    value = ((uint32_t)millivolts * YI_MCP4725_MAX_CODE + cfg->reference_mv / 2U) /
            cfg->reference_mv;
    return mcp4725_dac_write(dev, (uint16_t)value);
}
/** @brief Select normal output or a power-down resistor. */
int yi_mcp4725_set_power(yi_device_t *dev, yi_mcp4725_power_t power)
{ if(!mcp4725_valid(dev)) return -1;
  return mcp4725_fast_write(dev, ((yi_mcp4725_data_t *)dev->data)->value, power); }
/** @brief Persist a DAC value and power mode in EEPROM. */
int yi_mcp4725_write_eeprom(yi_device_t *dev, uint16_t value,
                            yi_mcp4725_power_t power)
{
    const yi_mcp4725_config_t *cfg; yi_mcp4725_data_t *data; uint8_t tx[3];
    uint16_t readback; bool ready; uint32_t started;
    if(!mcp4725_valid(dev) || value > 4095U || power > 3U) return -1;
    cfg = dev->config; data = dev->data;
    tx[0] = (uint8_t)(MCP4725_CMD_WRITE_DAC_EEPROM | ((uint8_t)power << 1));
    tx[1] = (uint8_t)(value >> 4); tx[2] = (uint8_t)(value << 4);
    if(yi_i2c_master_write(cfg->i2c, cfg->address, tx, sizeof(tx),
                           cfg->transfer_timeout_ms) != 0) return -1;
    started = yi_system_uptime_ms();
    do {
        if(yi_mcp4725_read(dev, &readback, NULL, &ready) == 0 && ready) {
            data->value = value; data->power = power; data->write_count++; return 0;
        }
        yi_system_delay_ms(1U);
    } while((uint32_t)(yi_system_uptime_ms() - started) < cfg->eeprom_timeout_ms);
    data->error_count++; return -1;
}
/** @brief Probe and initialize the MCP4725 output. */
int yi_mcp4725_init(const void *config)
{
    const yi_mcp4725_config_t *cfg = config; yi_mcp4725_data_t *data;
    uint16_t probe;
    if(cfg == NULL || cfg->self == NULL || cfg->self->data == NULL ||
       !yi_device_is_ready(cfg->i2c) || cfg->address > 0x7FU ||
       cfg->reference_mv == 0U || cfg->default_value > 4095U ||
       cfg->transfer_timeout_ms == 0U || cfg->eeprom_timeout_ms == 0U) return -1;
    data = cfg->self->data; data->write_count = 0U; data->error_count = 0U;
    if(yi_mcp4725_read(cfg->self, &probe, &data->power, NULL) != 0) return -1;
    data->value = probe;
    return mcp4725_fast_write(cfg->self, cfg->default_value, YI_MCP4725_POWER_NORMAL);
}
const yi_dac_api_t yi_mcp4725_api = {.write = mcp4725_dac_write};
