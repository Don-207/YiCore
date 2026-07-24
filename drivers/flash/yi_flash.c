#include "yi_flash.h"

static const yi_flash_api_t *yi_flash_api(const yi_device_t *dev)
{
    if(!yi_device_is_ready(dev) || (dev->api == NULL))
    {
        return NULL;
    }
    return (const yi_flash_api_t *)dev->api;
}

int yi_flash_read(yi_device_t *dev, uint32_t offset, void *data, uint32_t length)
{
    const yi_flash_api_t *api = yi_flash_api(dev);
    return ((api != NULL) && (api->read != NULL)) ?
           api->read(dev, offset, data, length) : -1;
}

int yi_flash_write(yi_device_t *dev, uint32_t offset, const void *data, uint32_t length)
{
    const yi_flash_api_t *api = yi_flash_api(dev);
    return ((api != NULL) && (api->write != NULL)) ?
           api->write(dev, offset, data, length) : -1;
}

int yi_flash_erase(yi_device_t *dev, uint32_t offset, uint32_t length)
{
    const yi_flash_api_t *api = yi_flash_api(dev);
    return ((api != NULL) && (api->erase != NULL)) ?
           api->erase(dev, offset, length) : -1;
}

uint32_t yi_flash_get_size(const yi_device_t *dev)
{
    const yi_flash_config_t *cfg = (dev != NULL) ? dev->config : NULL;
    return (cfg != NULL) ? cfg->size : 0U;
}

uint32_t yi_flash_get_erase_block_size(const yi_device_t *dev)
{
    const yi_flash_config_t *cfg = (dev != NULL) ? dev->config : NULL;
    return (cfg != NULL) ? cfg->erase_block_size : 0U;
}

uint32_t yi_flash_get_write_block_size(const yi_device_t *dev)
{
    const yi_flash_config_t *cfg = (dev != NULL) ? dev->config : NULL;
    return (cfg != NULL) ? cfg->write_block_size : 0U;
}
