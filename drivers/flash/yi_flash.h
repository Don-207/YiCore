#ifndef YI_FLASH_H
#define YI_FLASH_H

#include "yi_device.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint32_t base_address;
    uint32_t size;
    uint32_t erase_block_size;
    uint32_t write_block_size;
} yi_flash_config_t;

typedef struct
{
    int (*read)(yi_device_t *dev, uint32_t offset, void *data, uint32_t length);
    int (*write)(yi_device_t *dev, uint32_t offset, const void *data, uint32_t length);
    int (*erase)(yi_device_t *dev, uint32_t offset, uint32_t length);
} yi_flash_api_t;

int yi_flash_read(yi_device_t *dev, uint32_t offset, void *data, uint32_t length);
int yi_flash_write(yi_device_t *dev, uint32_t offset, const void *data, uint32_t length);
int yi_flash_erase(yi_device_t *dev, uint32_t offset, uint32_t length);
uint32_t yi_flash_get_size(const yi_device_t *dev);
uint32_t yi_flash_get_erase_block_size(const yi_device_t *dev);
uint32_t yi_flash_get_write_block_size(const yi_device_t *dev);

int yi_stm32_flash_init(const void *config);
extern const yi_flash_api_t yi_stm32_flash_driver_api;

#define YI_STM32_FLASH_DEFINE_LEVEL(_name, _level, _priority, _config) \
    YI_DEVICE_DEFINE_WITH_API(                                        \
        _name, _level, _priority, yi_stm32_flash_init,                \
        &_config, NULL, (const yi_device_api_t *)&yi_stm32_flash_driver_api \
    )

#ifdef __cplusplus
}
#endif

#endif
