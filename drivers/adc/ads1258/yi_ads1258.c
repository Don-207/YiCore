/** @file yi_ads1258.c @brief YiCore ADS1258 implementation. */
#include "yi_ads1258.h"
#include "yi_system.h"
#include <string.h>

#define ADS1258_CMD_READ_DATA 0x30U
#define ADS1258_CMD_READ_REG  0x40U
#define ADS1258_CMD_WRITE_REG 0x60U
#define ADS1258_CMD_PULSE     0x80U
#define ADS1258_CMD_RESET     0xC0U
#define ADS1258_CONFIG0_STAT  (1U << 3)
#define ADS1258_REGISTER_COUNT 10U

/** @brief Validate an ADS1258 device object. */
static bool ads1258_valid(yi_device_t *dev)
{ return (dev != NULL) && (dev->config != NULL) && (dev->data != NULL); }

/** @brief Transfer one ADS1258 SPI frame. */
static int ads1258_transfer(const yi_ads1258_config_t *cfg, const uint8_t *tx,
                            uint8_t *rx, uint16_t length)
{ return yi_spi_transceive(cfg->spi, &cfg->spi_config, tx, rx, length,
                           cfg->transfer_timeout_ms); }

/** @brief Read an ADS1258 register. */
int yi_ads1258_read_register(yi_device_t *dev, uint8_t address, uint8_t *value)
{
    const yi_ads1258_config_t *cfg; uint8_t tx[2], rx[2] = {0U};
    if(!ads1258_valid(dev) || (value == NULL) || (address >= ADS1258_REGISTER_COUNT)) return -1;
    cfg = dev->config; tx[0] = (uint8_t)(ADS1258_CMD_READ_REG | address); tx[1] = 0U;
    if(ads1258_transfer(cfg, tx, rx, 2U) != 0) { ((yi_ads1258_data_t *)dev->data)->error_count++; return -1; }
    *value = rx[1]; return 0;
}

/** @brief Write an ADS1258 register. */
int yi_ads1258_write_register(yi_device_t *dev, uint8_t address, uint8_t value)
{
    const yi_ads1258_config_t *cfg; uint8_t tx[2];
    if(!ads1258_valid(dev) || (address >= YI_ADS1258_REG_ID)) return -1;
    cfg = dev->config; tx[0] = (uint8_t)(ADS1258_CMD_WRITE_REG | address); tx[1] = value;
    if(ads1258_transfer(cfg, tx, NULL, 2U) != 0) { ((yi_ads1258_data_t *)dev->data)->error_count++; return -1; }
    return 0;
}

/** @brief Send a command byte to the converter. */
static int ads1258_command(yi_device_t *dev, uint8_t command)
{ const yi_ads1258_config_t *cfg = dev->config; return ads1258_transfer(cfg, &command, NULL, 1U); }

/** @brief Reset the converter using GPIO or the reset command. */
int yi_ads1258_reset(yi_device_t *dev)
{
    const yi_ads1258_config_t *cfg;
    if(!ads1258_valid(dev)) return -1; cfg = dev->config;
    if(cfg->reset_gpio != NULL) {
        if(yi_gpio_set(cfg->reset_gpio, YI_GPIO_LOW) != 0) return -1;
        yi_system_delay_us(2U);
        if(yi_gpio_set(cfg->reset_gpio, YI_GPIO_HIGH) != 0) return -1;
    } else if(ads1258_command(dev, ADS1258_CMD_RESET) != 0) return -1;
    yi_system_delay_us(100U); return 0;
}

/** @brief Start continuous conversions. */
int yi_ads1258_start(yi_device_t *dev)
{
    const yi_ads1258_config_t *cfg; yi_ads1258_data_t *data;
    if(!ads1258_valid(dev)) return -1; cfg = dev->config; data = dev->data;
    if(cfg->start_gpio == NULL || yi_gpio_set(cfg->start_gpio, YI_GPIO_HIGH) != 0) return -1;
    data->running = true; return 0;
}
/** @brief Stop continuous conversions. */
int yi_ads1258_stop(yi_device_t *dev)
{
    const yi_ads1258_config_t *cfg; yi_ads1258_data_t *data;
    if(!ads1258_valid(dev)) return -1; cfg = dev->config; data = dev->data;
    if(cfg->start_gpio == NULL || yi_gpio_set(cfg->start_gpio, YI_GPIO_LOW) != 0) return -1;
    data->running = false; return 0;
}
/** @brief Trigger one pulse conversion. */
int yi_ads1258_pulse_convert(yi_device_t *dev)
{ return ads1258_valid(dev) ? ads1258_command(dev, ADS1258_CMD_PULSE) : -1; }

/** @brief Read the active-low DRDY input. */
int yi_ads1258_data_ready(yi_device_t *dev, bool *ready)
{
    const yi_ads1258_config_t *cfg; int level;
    if(!ads1258_valid(dev) || (ready == NULL)) return -1; cfg = dev->config;
    if(cfg->drdy_gpio == NULL || (level = yi_gpio_get(cfg->drdy_gpio)) < 0) return -1;
    *ready = level == YI_GPIO_LOW; return 0;
}

/** @brief Read and decode one status-tagged 24-bit sample. */
int yi_ads1258_read_sample(yi_device_t *dev, yi_ads1258_sample_t *sample)
{
    const yi_ads1258_config_t *cfg; yi_ads1258_data_t *data;
    uint8_t tx[5] = {ADS1258_CMD_READ_DATA, 0U, 0U, 0U, 0U}; uint8_t rx[5] = {0U};
    int32_t value;
    if(!ads1258_valid(dev) || (sample == NULL)) return -1; cfg = dev->config; data = dev->data;
    if(ads1258_transfer(cfg, tx, rx, sizeof(tx)) != 0) { data->error_count++; return -1; }
    value = ((int32_t)rx[2] << 16) | ((int32_t)rx[3] << 8) | rx[4];
    if((value & 0x00800000L) != 0L) value |= (int32_t)0xFF000000L;
    sample->new_data = (rx[1] & 0x80U) != 0U;
    sample->overflow = (rx[1] & 0x40U) != 0U;
    sample->supply = (rx[1] & 0x20U) != 0U;
    sample->channel = rx[1] & 0x1FU; sample->value = value;
    data->sample_count++; return 0;
}

/** @brief Initialize and verify the ADS1258 registers. */
int yi_ads1258_init(const void *config)
{
    const yi_ads1258_config_t *cfg = config; yi_ads1258_data_t *data;
    uint8_t regs[7], verify;
    if((cfg == NULL) || (cfg->self == NULL) || (cfg->self->data == NULL) ||
       !yi_device_is_ready(cfg->spi) || !yi_device_is_ready(cfg->spi_config.cs_gpio) ||
       ((cfg->reset_gpio != NULL) && !yi_device_is_ready(cfg->reset_gpio)) ||
       (cfg->start_gpio == NULL) || !yi_device_is_ready(cfg->start_gpio) ||
       ((cfg->drdy_gpio != NULL) && !yi_device_is_ready(cfg->drdy_gpio)) ||
       (cfg->spi_config.mode != 1U) || (cfg->spi_config.frequency == 0U) ||
       (cfg->spi_config.frequency > 2000000U) || (cfg->transfer_timeout_ms == 0U)) return -1;
    data = cfg->self->data; memset(data, 0, sizeof(*data));
    (void)yi_gpio_set(cfg->start_gpio, YI_GPIO_LOW);
    if(yi_ads1258_reset(cfg->self) != 0 ||
       yi_ads1258_read_register(cfg->self, YI_ADS1258_REG_ID, &data->device_id) != 0 ||
       (data->device_id & 0xE0U) != 0x80U) return -1;
    regs[0] = cfg->config0 | ADS1258_CONFIG0_STAT; regs[1] = cfg->config1;
    regs[2] = cfg->muxsch; regs[3] = cfg->muxdif;
    regs[4] = (uint8_t)cfg->single_ended_mask;
    regs[5] = (uint8_t)(cfg->single_ended_mask >> 8); regs[6] = cfg->system_readings;
    for(uint8_t i = 0U; i < 7U; i++) {
        if(yi_ads1258_write_register(cfg->self, i, regs[i]) != 0 ||
           yi_ads1258_read_register(cfg->self, i, &verify) != 0 || verify != regs[i]) return -1;
    }
    data->initialized = true; return 0;
}
