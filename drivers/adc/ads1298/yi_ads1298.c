#include "yi_ads1298.h"

#include <stddef.h>
#include <string.h>

#include "yi_system.h"

#define ADS1298_CMD_RREG    0x20U
#define ADS1298_CMD_WREG    0x40U

#define ADS1298_CONFIG1_HR       (1U << 7)
#define ADS1298_CONFIG3_RESERVED (1U << 6)
#define ADS1298_CONFIG3_REFBUF   (1U << 7)
#define ADS1298_CONFIG3_VREF_4V  (1U << 5)
#define ADS1298_CHANNEL_PD       (1U << 7)

static bool ads1298_valid_device(yi_device_t *dev)
{
    return (dev != NULL) && (dev->config != NULL) && (dev->data != NULL);
}

static void ads1298_clock_delay(const yi_ads1298_config_t *cfg,
                                uint32_t clock_cycles)
{
    uint32_t delay_us = (uint32_t)(((uint64_t)clock_cycles * 1000000ULL +
                                    cfg->master_clock_hz - 1U) /
                                   cfg->master_clock_hz);
    yi_system_delay_us(delay_us);
}

static int ads1298_transfer(const yi_ads1298_config_t *cfg,
                            const uint8_t *tx, uint8_t *rx, uint16_t length)
{
    return yi_spi_transceive(cfg->spi, &cfg->spi_config, tx, rx, length,
                             cfg->transfer_timeout_ms);
}

static int ads1298_command_raw(const yi_ads1298_config_t *cfg, uint8_t command)
{
    int result = ads1298_transfer(cfg, &command, NULL, 1U);
    if(result == 0)
    {
        ads1298_clock_delay(cfg, (command == YI_ADS1298_CMD_RESET) ? 18U : 4U);
    }
    return result;
}

int yi_ads1298_command(yi_device_t *dev, yi_ads1298_command_t command)
{
    const yi_ads1298_config_t *cfg;
    yi_ads1298_data_t *data;

    if(!ads1298_valid_device(dev)) { return -1; }
    cfg = (const yi_ads1298_config_t *)dev->config;
    data = (yi_ads1298_data_t *)dev->data;
    if(data->continuous && (command != YI_ADS1298_CMD_SDATAC)) { return -1; }
    if(ads1298_command_raw(cfg, (uint8_t)command) != 0)
    {
        data->error_count++;
        return -1;
    }
    if(command == YI_ADS1298_CMD_RDATAC) { data->continuous = true; }
    else if(command == YI_ADS1298_CMD_SDATAC) { data->continuous = false; }
    else if(command == YI_ADS1298_CMD_START) { data->running = true; }
    else if(command == YI_ADS1298_CMD_STOP) { data->running = false; }
    return 0;
}

int yi_ads1298_read_registers(yi_device_t *dev, uint8_t address,
                              uint8_t *values, uint8_t count)
{
    const yi_ads1298_config_t *cfg;
    yi_ads1298_data_t *data;
    uint8_t tx[YI_ADS1298_REGISTER_COUNT + 2U] = {0U};
    uint8_t rx[YI_ADS1298_REGISTER_COUNT + 2U] = {0U};

    if(!ads1298_valid_device(dev) || (values == NULL) || (count == 0U) ||
       (address >= YI_ADS1298_REGISTER_COUNT) ||
       ((uint16_t)address + count > YI_ADS1298_REGISTER_COUNT))
    {
        return -1;
    }
    cfg = (const yi_ads1298_config_t *)dev->config;
    data = (yi_ads1298_data_t *)dev->data;
    if(data->continuous) { return -1; }
    tx[0] = (uint8_t)(ADS1298_CMD_RREG | address);
    tx[1] = (uint8_t)(count - 1U);
    if(ads1298_transfer(cfg, tx, rx, (uint16_t)count + 2U) != 0)
    {
        data->error_count++;
        return -1;
    }
    memcpy(values, &rx[2], count);
    return 0;
}

int yi_ads1298_write_registers(yi_device_t *dev, uint8_t address,
                               const uint8_t *values, uint8_t count)
{
    const yi_ads1298_config_t *cfg;
    yi_ads1298_data_t *data;
    uint8_t tx[YI_ADS1298_REGISTER_COUNT + 2U];

    if(!ads1298_valid_device(dev) || (values == NULL) || (count == 0U) ||
       (address == YI_ADS1298_REG_ID) ||
       (address >= YI_ADS1298_REGISTER_COUNT) ||
       ((uint16_t)address + count > YI_ADS1298_REGISTER_COUNT))
    {
        return -1;
    }
    cfg = (const yi_ads1298_config_t *)dev->config;
    data = (yi_ads1298_data_t *)dev->data;
    if(data->continuous) { return -1; }
    tx[0] = (uint8_t)(ADS1298_CMD_WREG | address);
    tx[1] = (uint8_t)(count - 1U);
    memcpy(&tx[2], values, count);
    if(ads1298_transfer(cfg, tx, NULL, (uint16_t)count + 2U) != 0)
    {
        data->error_count++;
        return -1;
    }
    return 0;
}

int yi_ads1298_start(yi_device_t *dev)
{
    const yi_ads1298_config_t *cfg;
    yi_ads1298_data_t *data;
    if(!ads1298_valid_device(dev)) { return -1; }
    cfg = (const yi_ads1298_config_t *)dev->config;
    data = (yi_ads1298_data_t *)dev->data;
    if(cfg->start_gpio != NULL)
    {
        if(yi_gpio_set(cfg->start_gpio, YI_GPIO_HIGH) != 0)
        {
            data->error_count++;
            return -1;
        }
        data->running = true;
        return 0;
    }
    return yi_ads1298_command(dev, YI_ADS1298_CMD_START);
}

int yi_ads1298_stop(yi_device_t *dev)
{
    const yi_ads1298_config_t *cfg;
    yi_ads1298_data_t *data;
    if(!ads1298_valid_device(dev)) { return -1; }
    cfg = (const yi_ads1298_config_t *)dev->config;
    data = (yi_ads1298_data_t *)dev->data;
    if(cfg->start_gpio != NULL)
    {
        if(yi_gpio_set(cfg->start_gpio, YI_GPIO_LOW) != 0)
        {
            data->error_count++;
            return -1;
        }
        data->running = false;
        return 0;
    }
    return yi_ads1298_command(dev, YI_ADS1298_CMD_STOP);
}

int yi_ads1298_set_continuous(yi_device_t *dev, bool enable)
{
    return yi_ads1298_command(dev, enable ? YI_ADS1298_CMD_RDATAC :
                                            YI_ADS1298_CMD_SDATAC);
}

int yi_ads1298_data_ready(yi_device_t *dev, bool *ready)
{
    const yi_ads1298_config_t *cfg;
    int level;
    if(!ads1298_valid_device(dev) || (ready == NULL)) { return -1; }
    cfg = (const yi_ads1298_config_t *)dev->config;
    if(cfg->drdy_gpio == NULL) { return -1; }
    level = yi_gpio_get(cfg->drdy_gpio);
    if(level < 0) { return -1; }
    *ready = level == YI_GPIO_LOW;
    return 0;
}

static int32_t ads1298_signed24(const uint8_t *bytes)
{
    int32_t value = ((int32_t)bytes[0] << 16U) |
                    ((int32_t)bytes[1] << 8U) | bytes[2];
    if((value & 0x00800000L) != 0L) { value |= (int32_t)0xFF000000L; }
    return value;
}

int yi_ads1298_read_frame(yi_device_t *dev, yi_ads1298_frame_t *frame)
{
    const yi_ads1298_config_t *cfg;
    yi_ads1298_data_t *data;
    uint8_t tx[YI_ADS1298_FRAME_SIZE + 1U] = {0U};
    uint8_t rx[YI_ADS1298_FRAME_SIZE + 1U] = {0U};
    const uint8_t *raw;
    uint16_t length;

    if(!ads1298_valid_device(dev) || (frame == NULL)) { return -1; }
    cfg = (const yi_ads1298_config_t *)dev->config;
    data = (yi_ads1298_data_t *)dev->data;
    if(data->continuous)
    {
        raw = rx;
        length = YI_ADS1298_FRAME_SIZE;
    }
    else
    {
        tx[0] = YI_ADS1298_CMD_RDATA;
        raw = &rx[1];
        length = YI_ADS1298_FRAME_SIZE + 1U;
    }
    if(ads1298_transfer(cfg, tx, rx, length) != 0)
    {
        data->error_count++;
        return -1;
    }
    frame->status = ((uint32_t)raw[0] << 16U) |
                    ((uint32_t)raw[1] << 8U) | raw[2];
    for(uint8_t channel = 0U; channel < YI_ADS1298_CHANNEL_COUNT; channel++)
    {
        frame->channel[channel] = ads1298_signed24(&raw[3U + channel * 3U]);
    }
    data->frame_count++;
    return 0;
}

int yi_ads1298_init(const void *config)
{
    const yi_ads1298_config_t *cfg = (const yi_ads1298_config_t *)config;
    yi_ads1298_data_t *data;
    uint8_t global_registers[3U];
    uint8_t channel_registers[YI_ADS1298_CHANNEL_COUNT];
    uint8_t verify[YI_ADS1298_CHANNEL_COUNT];

    if((cfg == NULL) || (cfg->self == NULL) || (cfg->self->data == NULL) ||
       !yi_device_is_ready(cfg->spi) ||
       !yi_device_is_ready(cfg->spi_config.cs_gpio) ||
       ((cfg->reset_gpio != NULL) && !yi_device_is_ready(cfg->reset_gpio)) ||
       ((cfg->start_gpio != NULL) && !yi_device_is_ready(cfg->start_gpio)) ||
       ((cfg->drdy_gpio != NULL) && !yi_device_is_ready(cfg->drdy_gpio)) ||
       (cfg->spi_config.mode != 1U) || (cfg->spi_config.frequency == 0U) ||
       (cfg->master_clock_hz == 0U) ||
       ((uint64_t)cfg->spi_config.frequency >
        ((uint64_t)cfg->master_clock_hz * 2ULL)) ||
       (cfg->transfer_timeout_ms == 0U) ||
       (cfg->data_rate > YI_ADS1298_DATA_RATE_6))
    {
        return -1;
    }
    for(uint8_t channel = 0U; channel < YI_ADS1298_CHANNEL_COUNT; channel++)
    {
        if((cfg->channels[channel].gain > YI_ADS1298_GAIN_12) ||
           (cfg->channels[channel].mux > YI_ADS1298_MUX_RLD_DRN))
        {
            return -1;
        }
    }

    data = (yi_ads1298_data_t *)cfg->self->data;
    memset(data, 0, sizeof(*data));
    if(cfg->start_gpio != NULL) { (void)yi_gpio_set(cfg->start_gpio, YI_GPIO_LOW); }
    if(cfg->reset_gpio != NULL)
    {
        if(yi_gpio_set(cfg->reset_gpio, YI_GPIO_LOW) != 0)
        {
            return -1;
        }
        ads1298_clock_delay(cfg, 2U);
        if(yi_gpio_set(cfg->reset_gpio, YI_GPIO_HIGH) != 0) { return -1; }
        ads1298_clock_delay(cfg, 18U);
    }
    else if(ads1298_command_raw(cfg, YI_ADS1298_CMD_RESET) != 0) { return -1; }
    /* Reset returns the part to RDATAC; register commands require SDATAC. */
    if(ads1298_command_raw(cfg, YI_ADS1298_CMD_SDATAC) != 0) { return -1; }

    if(yi_ads1298_read_registers(cfg->self, YI_ADS1298_REG_ID,
                                 &data->device_id, 1U) != 0)
    {
        return -1;
    }
    if((data->device_id & 0xE7U) != 0x82U) { return -1; }

    global_registers[0] =
        (uint8_t)((cfg->high_resolution ? ADS1298_CONFIG1_HR : 0U) |
                  cfg->data_rate);
    global_registers[1] = 0x40U;
    global_registers[2] =
        (uint8_t)(ADS1298_CONFIG3_RESERVED |
                  (cfg->internal_reference ? ADS1298_CONFIG3_REFBUF : 0U) |
                  (cfg->reference_4v ? ADS1298_CONFIG3_VREF_4V : 0U));
    for(uint8_t channel = 0U; channel < YI_ADS1298_CHANNEL_COUNT; channel++)
    {
        channel_registers[channel] =
            (uint8_t)((cfg->channels[channel].power_down ? ADS1298_CHANNEL_PD : 0U) |
                      ((uint8_t)cfg->channels[channel].gain << 4U) |
                      cfg->channels[channel].mux);
    }
    if((yi_ads1298_write_registers(cfg->self, YI_ADS1298_REG_CONFIG1,
                                   global_registers,
                                   sizeof(global_registers)) != 0) ||
       (yi_ads1298_read_registers(cfg->self, YI_ADS1298_REG_CONFIG1,
                                  verify, sizeof(global_registers)) != 0) ||
       (memcmp(global_registers, verify, sizeof(global_registers)) != 0) ||
       (yi_ads1298_write_registers(cfg->self, YI_ADS1298_REG_CH1SET,
                                   channel_registers,
                                   sizeof(channel_registers)) != 0) ||
       (yi_ads1298_read_registers(cfg->self, YI_ADS1298_REG_CH1SET,
                                  verify, sizeof(channel_registers)) != 0) ||
       (memcmp(channel_registers, verify, sizeof(channel_registers)) != 0))
    {
        return -1;
    }
    data->initialized = true;
    return 0;
}
