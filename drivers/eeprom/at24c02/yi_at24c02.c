#include "yi_at24c02.h"
#include "yi_system.h"
#include <stddef.h>

static bool yi_at24c02_range_valid(const yi_at24c02_config_t *cfg,
                                   uint32_t offset,
                                   uint32_t length)
{
    return (cfg != NULL) && (length > 0U) &&
           (offset < cfg->eeprom.size) &&
           (length <= (cfg->eeprom.size - offset));
}

static int yi_at24c02_wait_ready(const yi_at24c02_config_t *cfg,
                                 uint8_t memory_address)
{
    uint32_t started = yi_system_uptime_ms();

    do
    {
        if(yi_i2c_master_write(cfg->i2c, cfg->address,
                               &memory_address, 1U,
                               cfg->transfer_timeout_ms) == 0)
        {
            return 0;
        }
    } while((uint32_t)(yi_system_uptime_ms() - started) <
            cfg->write_timeout_ms);

    return -1;
}

int yi_at24c02_init(const void *config)
{
    const yi_at24c02_config_t *cfg = config;
    yi_at24c02_data_t *data;
    uint8_t probe;

    if((cfg == NULL) || (cfg->self == NULL) ||
       (cfg->self->data == NULL) || !yi_device_is_ready(cfg->i2c) ||
       (cfg->address > 0x7FU) ||
       (cfg->eeprom.size != YI_AT24C02_SIZE) ||
       (cfg->eeprom.page_size != YI_AT24C02_PAGE_SIZE) ||
       (cfg->transfer_timeout_ms == 0U) ||
       (cfg->write_timeout_ms == 0U))
    {
        return -1;
    }

    data = (yi_at24c02_data_t *)cfg->self->data;
    data->read_count = 0U;
    data->write_count = 0U;
    data->error_count = 0U;
    return yi_i2c_master_read(cfg->i2c, cfg->address, &probe, 1U,
                              cfg->transfer_timeout_ms);
}

static int yi_at24c02_read(yi_device_t *dev, uint32_t offset,
                           void *buffer, uint32_t length)
{
    const yi_at24c02_config_t *cfg = dev->config;
    yi_at24c02_data_t *data = dev->data;
    uint8_t memory_address;
    int result;

    if((buffer == NULL) || !yi_at24c02_range_valid(cfg, offset, length) ||
       (length > UINT16_MAX))
    {
        return -1;
    }

    memory_address = (uint8_t)offset;
    result = yi_i2c_master_write_read(
        cfg->i2c, cfg->address,
        &memory_address, 1U,
        (uint8_t *)buffer, (uint16_t)length,
        cfg->transfer_timeout_ms);
    if(result == 0)
    {
        data->read_count++;
    }
    else
    {
        data->error_count++;
    }
    return result;
}

static int yi_at24c02_write(yi_device_t *dev, uint32_t offset,
                            const void *buffer, uint32_t length)
{
    const yi_at24c02_config_t *cfg = dev->config;
    yi_at24c02_data_t *data = dev->data;
    const uint8_t *input = buffer;
    uint8_t frame[YI_AT24C02_PAGE_SIZE + 1U];

    if((buffer == NULL) || !yi_at24c02_range_valid(cfg, offset, length))
    {
        return -1;
    }

    while(length > 0U)
    {
        uint32_t page_remaining =
            YI_AT24C02_PAGE_SIZE - (offset % YI_AT24C02_PAGE_SIZE);
        uint8_t chunk = (length > page_remaining)
                        ? (uint8_t)page_remaining : (uint8_t)length;
        uint8_t memory_address = (uint8_t)offset;

        frame[0] = memory_address;
        for(uint8_t index = 0U; index < chunk; index++)
        {
            frame[index + 1U] = input[index];
        }
        if((yi_i2c_master_write(cfg->i2c, cfg->address,
                                frame, (uint16_t)(chunk + 1U),
                                cfg->transfer_timeout_ms) != 0) ||
           (yi_at24c02_wait_ready(cfg, memory_address) != 0))
        {
            data->error_count++;
            return -1;
        }

        offset += chunk;
        input += chunk;
        length -= chunk;
        data->write_count++;
    }
    return 0;
}

const yi_eeprom_api_t yi_at24c02_api =
{
    .read = yi_at24c02_read,
    .write = yi_at24c02_write
};
