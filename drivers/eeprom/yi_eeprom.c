/**
 * @file yi_eeprom.c
 * @brief YiCore eeprom implementation.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#include "yi_eeprom.h"
#include <stddef.h>

/**
 * @brief Perform the yi eeprom api operation.
 * @param dev Device instance.
 */
static const yi_eeprom_api_t *yi_eeprom_api(const yi_device_t *dev)
{
    if(!yi_device_is_ready(dev) || (dev->api == NULL))
    {
        return NULL;
    }
    return (const yi_eeprom_api_t *)dev->api;
}

/**
 * @brief Read the module.
 * @param dev Device instance.
 * @param offset Byte offset from the start of the device.
 * @param data Driver runtime data.
 * @param length Number of bytes to process.
 */
int yi_eeprom_read(yi_device_t *dev, uint32_t offset,
                   void *data, uint32_t length)
{
    const yi_eeprom_api_t *api = yi_eeprom_api(dev);
    return ((api != NULL) && (api->read != NULL))
           ? api->read(dev, offset, data, length) : -1;
}

/**
 * @brief Write the module.
 * @param dev Device instance.
 * @param offset Byte offset from the start of the device.
 * @param data Driver runtime data.
 * @param length Number of bytes to process.
 */
int yi_eeprom_write(yi_device_t *dev, uint32_t offset,
                    const void *data, uint32_t length)
{
    const yi_eeprom_api_t *api = yi_eeprom_api(dev);
    return ((api != NULL) && (api->write != NULL))
           ? api->write(dev, offset, data, length) : -1;
}

/**
 * @brief Get size.
 * @param dev Device instance.
 */
uint32_t yi_eeprom_get_size(const yi_device_t *dev)
{
    const yi_eeprom_config_t *cfg = (dev != NULL) ? dev->config : NULL;
    return (cfg != NULL) ? cfg->size : 0U;
}

/**
 * @brief Get page size.
 * @param dev Device instance.
 */
uint32_t yi_eeprom_get_page_size(const yi_device_t *dev)
{
    const yi_eeprom_config_t *cfg = (dev != NULL) ? dev->config : NULL;
    return (cfg != NULL) ? cfg->page_size : 0U;
}
