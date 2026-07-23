#include "yi_w25q64.h"
#include "yi_system.h"
#include <stddef.h>

#define W25Q64_CMD_READ_DATA          0x03U
#define W25Q64_CMD_PAGE_PROGRAM       0x02U
#define W25Q64_CMD_WRITE_ENABLE       0x06U
#define W25Q64_CMD_READ_STATUS1       0x05U
#define W25Q64_CMD_SECTOR_ERASE       0x20U
#define W25Q64_CMD_READ_JEDEC_ID      0x9FU
#define W25Q64_STATUS_BUSY            0x01U
#define W25Q64_COMMAND_SIZE           4U
#define W25Q64_TRANSFER_SIZE          (W25Q64_COMMAND_SIZE + \
                                       YI_W25Q64_PAGE_SIZE)
#define W25Q64_COMMAND_TIMEOUT_MS     20U

static bool yi_w25q64_range_valid(const yi_w25q64_config_t *cfg,
                                  uint32_t offset,
                                  uint32_t length)
{
    return (cfg != NULL) && (length > 0U) &&
           (offset < cfg->flash.size) &&
           (length <= (cfg->flash.size - offset));
}

static int yi_w25q64_transfer(const yi_w25q64_config_t *cfg,
                              const uint8_t *tx,
                              uint8_t *rx,
                              uint16_t length,
                              uint32_t timeout_ms)
{
    return yi_spi_transceive(cfg->spi, &cfg->spi_config,
                             tx, rx, length, timeout_ms);
}

static int yi_w25q64_write_enable(const yi_w25q64_config_t *cfg)
{
    const uint8_t command = W25Q64_CMD_WRITE_ENABLE;
    return yi_w25q64_transfer(cfg, &command, NULL, 1U,
                              W25Q64_COMMAND_TIMEOUT_MS);
}

static int yi_w25q64_wait_ready(const yi_w25q64_config_t *cfg,
                                uint32_t timeout_ms)
{
    const uint8_t tx[2] = {W25Q64_CMD_READ_STATUS1, 0xFFU};
    uint8_t rx[2];
    uint32_t started = yi_system_uptime_ms();

    do
    {
        if(yi_w25q64_transfer(cfg, tx, rx, sizeof(tx),
                              W25Q64_COMMAND_TIMEOUT_MS) != 0)
        {
            return -1;
        }
        if((rx[1] & W25Q64_STATUS_BUSY) == 0U)
        {
            return 0;
        }
    } while((uint32_t)(yi_system_uptime_ms() - started) < timeout_ms);

    return -1;
}

static uint32_t yi_w25q64_read_jedec_id(const yi_w25q64_config_t *cfg)
{
    const uint8_t tx[4] =
    {
        W25Q64_CMD_READ_JEDEC_ID, 0xFFU, 0xFFU, 0xFFU
    };
    uint8_t rx[4];

    if(yi_w25q64_transfer(cfg, tx, rx, sizeof(tx),
                          W25Q64_COMMAND_TIMEOUT_MS) != 0)
    {
        return 0U;
    }
    return ((uint32_t)rx[1] << 16U) |
           ((uint32_t)rx[2] << 8U) |
           (uint32_t)rx[3];
}

int yi_w25q64_init(const void *config)
{
    const yi_w25q64_config_t *cfg = config;
    yi_w25q64_data_t *data;

    if((cfg == NULL) || (cfg->self == NULL) || (cfg->self->data == NULL) ||
       !yi_device_is_ready(cfg->spi) ||
       (cfg->spi_config.cs_gpio == NULL) ||
       !yi_device_is_ready(cfg->spi_config.cs_gpio) ||
       (cfg->spi_config.frequency == 0U) ||
       (cfg->spi_config.mode > 3U) ||
       cfg->spi_config.cs_active_high ||
       (cfg->flash.base_address != 0U) ||
       (cfg->flash.size != YI_W25Q64_SIZE) ||
       (cfg->flash.erase_block_size != YI_W25Q64_SECTOR_SIZE) ||
       (cfg->flash.write_block_size != 1U) ||
       (cfg->program_timeout_ms == 0U) ||
       (cfg->erase_timeout_ms == 0U))
    {
        return -1;
    }

    data = (yi_w25q64_data_t *)cfg->self->data;
    data->jedec_id = yi_w25q64_read_jedec_id(cfg);
    data->error_count = 0U;
    return (data->jedec_id == YI_W25Q64_JEDEC_ID) ? 0 : -1;
}

static int yi_w25q64_read(yi_device_t *dev, uint32_t offset,
                           void *buffer, uint32_t length)
{
    const yi_w25q64_config_t *cfg = dev->config;
    uint8_t *output = buffer;
    uint8_t tx[W25Q64_TRANSFER_SIZE];
    uint8_t rx[W25Q64_TRANSFER_SIZE];

    if((buffer == NULL) || !yi_w25q64_range_valid(cfg, offset, length))
    {
        return -1;
    }

    while(length > 0U)
    {
        uint16_t chunk = (length > YI_W25Q64_PAGE_SIZE)
                         ? YI_W25Q64_PAGE_SIZE : (uint16_t)length;
        uint16_t transfer_length = (uint16_t)(chunk + W25Q64_COMMAND_SIZE);

        tx[0] = W25Q64_CMD_READ_DATA;
        tx[1] = (uint8_t)(offset >> 16U);
        tx[2] = (uint8_t)(offset >> 8U);
        tx[3] = (uint8_t)offset;
        for(uint16_t index = W25Q64_COMMAND_SIZE;
            index < transfer_length; index++)
        {
            tx[index] = 0xFFU;
        }
        if(yi_w25q64_transfer(cfg, tx, rx, transfer_length,
                              W25Q64_COMMAND_TIMEOUT_MS) != 0)
        {
            return -1;
        }
        for(uint16_t index = 0U; index < chunk; index++)
        {
            output[index] = rx[index + W25Q64_COMMAND_SIZE];
        }
        offset += chunk;
        output += chunk;
        length -= chunk;
    }
    return 0;
}

static int yi_w25q64_write(yi_device_t *dev, uint32_t offset,
                            const void *buffer, uint32_t length)
{
    const yi_w25q64_config_t *cfg = dev->config;
    const uint8_t *input = buffer;
    uint8_t frame[W25Q64_TRANSFER_SIZE];

    if((buffer == NULL) || !yi_w25q64_range_valid(cfg, offset, length))
    {
        return -1;
    }

    while(length > 0U)
    {
        uint32_t page_remaining =
            YI_W25Q64_PAGE_SIZE - (offset % YI_W25Q64_PAGE_SIZE);
        uint16_t chunk = (length > page_remaining)
                         ? (uint16_t)page_remaining : (uint16_t)length;

        if(yi_w25q64_write_enable(cfg) != 0)
        {
            return -1;
        }
        frame[0] = W25Q64_CMD_PAGE_PROGRAM;
        frame[1] = (uint8_t)(offset >> 16U);
        frame[2] = (uint8_t)(offset >> 8U);
        frame[3] = (uint8_t)offset;
        for(uint16_t index = 0U; index < chunk; index++)
        {
            frame[index + W25Q64_COMMAND_SIZE] = input[index];
        }
        if((yi_w25q64_transfer(
                cfg, frame, NULL,
                (uint16_t)(chunk + W25Q64_COMMAND_SIZE),
                cfg->program_timeout_ms) != 0) ||
           (yi_w25q64_wait_ready(cfg, cfg->program_timeout_ms) != 0))
        {
            return -1;
        }
        offset += chunk;
        input += chunk;
        length -= chunk;
    }
    return 0;
}

static int yi_w25q64_erase(yi_device_t *dev,
                            uint32_t offset,
                            uint32_t length)
{
    const yi_w25q64_config_t *cfg = dev->config;

    if(!yi_w25q64_range_valid(cfg, offset, length) ||
       ((offset % YI_W25Q64_SECTOR_SIZE) != 0U) ||
       ((length % YI_W25Q64_SECTOR_SIZE) != 0U))
    {
        return -1;
    }

    while(length > 0U)
    {
        const uint8_t command[4] =
        {
            W25Q64_CMD_SECTOR_ERASE,
            (uint8_t)(offset >> 16U),
            (uint8_t)(offset >> 8U),
            (uint8_t)offset
        };

        if((yi_w25q64_write_enable(cfg) != 0) ||
           (yi_w25q64_transfer(cfg, command, NULL, sizeof(command),
                                W25Q64_COMMAND_TIMEOUT_MS) != 0) ||
           (yi_w25q64_wait_ready(cfg, cfg->erase_timeout_ms) != 0))
        {
            return -1;
        }
        offset += YI_W25Q64_SECTOR_SIZE;
        length -= YI_W25Q64_SECTOR_SIZE;
    }
    return 0;
}

const yi_flash_api_t yi_w25q64_api =
{
    .read = yi_w25q64_read,
    .write = yi_w25q64_write,
    .erase = yi_w25q64_erase
};
