/**
 * @file yi_eeprom.h
 * @brief YiCore eeprom interface.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#ifndef YI_EEPROM_H
#define YI_EEPROM_H

#include "yi_device.h"

typedef struct
{
    uint32_t size; /**< Size value. */
    uint32_t page_size; /**< Page size value. */} yi_eeprom_config_t;

typedef struct
{
    int (*read)(yi_device_t *dev, uint32_t offset,
                void *data, uint32_t length);
    int (*write)(yi_device_t *dev, uint32_t offset,
                 const void *data, uint32_t length);
} yi_eeprom_api_t;

/**
 * @brief Read the module.
 * @param dev Device instance.
 * @param offset Byte offset from the start of the device.
 * @param data Driver runtime data.
 * @param length Number of bytes to process.
 */
int yi_eeprom_read(yi_device_t *dev, uint32_t offset,
                   void *data, uint32_t length);
/**
 * @brief Write the module.
 * @param dev Device instance.
 * @param offset Byte offset from the start of the device.
 * @param data Driver runtime data.
 * @param length Number of bytes to process.
 */
int yi_eeprom_write(yi_device_t *dev, uint32_t offset,
                    const void *data, uint32_t length);
/**
 * @brief Get size.
 * @param dev Device instance.
 */
uint32_t yi_eeprom_get_size(const yi_device_t *dev);
/**
 * @brief Get page size.
 * @param dev Device instance.
 */
uint32_t yi_eeprom_get_page_size(const yi_device_t *dev);

#endif
