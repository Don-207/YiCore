#ifndef YI_W25Q64_H
#define YI_W25Q64_H

#include "yi_flash.h"
#include "yi_spi.h"

#define YI_W25Q64_SIZE              0x00800000U
#define YI_W25Q64_SECTOR_SIZE       4096U
#define YI_W25Q64_PAGE_SIZE         256U
#define YI_W25Q64_JEDEC_ID          0x00EF4017U

typedef struct
{
    /* Must remain first for the common yi_flash geometry accessors. */
    yi_flash_config_t flash;
    yi_device_t *self;
    yi_device_t *spi;
    yi_spi_transfer_config_t spi_config;
    uint32_t program_timeout_ms;
    uint32_t erase_timeout_ms;
} yi_w25q64_config_t;

typedef struct
{
    uint32_t jedec_id;
    uint32_t error_count;
} yi_w25q64_data_t;

int yi_w25q64_init(const void *config);
extern const yi_flash_api_t yi_w25q64_api;

#define YI_W25Q64_DEFINE_LEVEL(_name, _level, _priority, _config, _data) \
    YI_DEVICE_DEFINE_WITH_API(                                          \
        _name, _level, _priority, yi_w25q64_init,                       \
        &_config, &_data, (const yi_device_api_t *)&yi_w25q64_api       \
    )

#endif
