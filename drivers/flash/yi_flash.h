/**
 * @file yi_flash.h
 * @brief YiCore flash interface.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#ifndef YI_FLASH_H
#define YI_FLASH_H

#include "yi_device.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint32_t base_address; /**< Base address value. */
    uint32_t size; /**< Size value. */
    uint32_t erase_block_size; /**< Erase block size value. */
    uint32_t write_block_size; /**< Write block size value. */} yi_flash_config_t;

typedef struct
{
    int (*read)(yi_device_t *dev, uint32_t offset, void *data, uint32_t length);
    int (*write)(yi_device_t *dev, uint32_t offset, const void *data, uint32_t length);
    int (*erase)(yi_device_t *dev, uint32_t offset, uint32_t length);
} yi_flash_api_t;

/**
 * @brief Read the module.
 * @param dev Device instance.
 * @param offset Byte offset from the start of the device.
 * @param data Driver runtime data.
 * @param length Number of bytes to process.
 */
int yi_flash_read(yi_device_t *dev, uint32_t offset, void *data, uint32_t length);
/**
 * @brief Write the module.
 * @param dev Device instance.
 * @param offset Byte offset from the start of the device.
 * @param data Driver runtime data.
 * @param length Number of bytes to process.
 */
int yi_flash_write(yi_device_t *dev, uint32_t offset, const void *data, uint32_t length);
/**
 * @brief Perform the yi flash erase operation.
 * @param dev Device instance.
 * @param offset Byte offset from the start of the device.
 * @param length Number of bytes to process.
 */
int yi_flash_erase(yi_device_t *dev, uint32_t offset, uint32_t length);
/**
 * @brief Get size.
 * @param dev Device instance.
 */
uint32_t yi_flash_get_size(const yi_device_t *dev);
/**
 * @brief Get erase block size.
 * @param dev Device instance.
 */
uint32_t yi_flash_get_erase_block_size(const yi_device_t *dev);
/**
 * @brief Get write block size.
 * @param dev Device instance.
 */
uint32_t yi_flash_get_write_block_size(const yi_device_t *dev);

#ifdef __cplusplus
}
#endif

#endif
