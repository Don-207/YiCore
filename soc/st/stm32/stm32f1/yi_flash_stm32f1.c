#include "yi_flash_stm32f1.h"
#include "stm32f1xx_hal.h"
#include <string.h>

static int yi_stm32_flash_range_valid(const yi_flash_config_t *cfg,
                                      uint32_t offset, uint32_t length)
{
    return (cfg != NULL) && (length > 0U) &&
           (offset < cfg->size) && (length <= (cfg->size - offset));
}

int yi_stm32_flash_init(const void *config)
{
    const yi_flash_config_t *cfg = config;

    if((cfg == NULL) || (cfg->size == 0U) ||
       (cfg->erase_block_size == 0U) ||
       (cfg->write_block_size != 2U) ||
       ((cfg->base_address & 1U) != 0U) ||
       ((cfg->size % cfg->erase_block_size) != 0U))
    {
        return -1;
    }
    return 0;
}

static int yi_stm32_flash_read(yi_device_t *dev, uint32_t offset,
                               void *data, uint32_t length)
{
    const yi_flash_config_t *cfg = dev->config;

    if((data == NULL) ||
       !yi_stm32_flash_range_valid(cfg, offset, length))
    {
        return -1;
    }
    memcpy(data, (const void *)(uintptr_t)(cfg->base_address + offset), length);
    return 0;
}

static int yi_stm32_flash_write(yi_device_t *dev, uint32_t offset,
                                const void *data, uint32_t length)
{
    const yi_flash_config_t *cfg = dev->config;
    const uint8_t *bytes = data;
    uint32_t index;
    int result = 0;

    if((data == NULL) ||
       !yi_stm32_flash_range_valid(cfg, offset, length) ||
       ((offset % cfg->write_block_size) != 0U) ||
       ((length % cfg->write_block_size) != 0U) ||
       (HAL_FLASH_Unlock() != HAL_OK))
    {
        return -1;
    }
    for(index = 0U; index < length; index += 2U)
    {
        uint16_t value = (uint16_t)bytes[index] |
                         ((uint16_t)bytes[index + 1U] << 8U);
        if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,
                             cfg->base_address + offset + index,
                             value) != HAL_OK)
        {
            result = -1;
            break;
        }
    }
    if(HAL_FLASH_Lock() != HAL_OK)
    {
        result = -1;
    }
    return result;
}

static int yi_stm32_flash_erase(yi_device_t *dev, uint32_t offset,
                                uint32_t length)
{
    const yi_flash_config_t *cfg = dev->config;
    FLASH_EraseInitTypeDef erase = {0};
    uint32_t page_error = 0U;
    int result;

    if(!yi_stm32_flash_range_valid(cfg, offset, length) ||
       ((offset % cfg->erase_block_size) != 0U) ||
       ((length % cfg->erase_block_size) != 0U) ||
       (HAL_FLASH_Unlock() != HAL_OK))
    {
        return -1;
    }
    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.PageAddress = cfg->base_address + offset;
    erase.NbPages = length / cfg->erase_block_size;
    result = (HAL_FLASHEx_Erase(&erase, &page_error) == HAL_OK) ? 0 : -1;
    if(HAL_FLASH_Lock() != HAL_OK)
    {
        result = -1;
    }
    return result;
}

const yi_flash_api_t yi_stm32_flash_driver_api =
{
    .read = yi_stm32_flash_read,
    .write = yi_stm32_flash_write,
    .erase = yi_stm32_flash_erase
};
