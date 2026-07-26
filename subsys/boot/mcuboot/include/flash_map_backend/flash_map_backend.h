/**
 * @file flash_map_backend.h
 * @brief YiCore flash map backend interface.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#ifndef YI_MCUBOOT_FLASH_MAP_BACKEND_H
#define YI_MCUBOOT_FLASH_MAP_BACKEND_H

#include <stdint.h>

#include "yi_partition.h"

#ifdef __cplusplus
extern "C" {
#endif

/* YiCore currently supports one MCUboot image with primary and secondary slots. */
#define FLASH_AREA_BOOTLOADER              0
#define FLASH_AREA_IMAGE_PRIMARY(_image)   (1 + ((_image) * 2))
#define FLASH_AREA_IMAGE_SECONDARY(_image) (2 + ((_image) * 2))
#define FLASH_AREA_IMAGE_SCRATCH           3

struct flash_area
{
    uint8_t fa_id; /**< Fa id value. */
    uint8_t fa_device_id; /**< Fa device id value. */
    uint16_t pad16; /**< Pad16 value. */
    uint32_t fa_off; /**< Fa off value. */
    uint32_t fa_size; /**< Fa size value. */
    const yi_partition_t *partition; /**< Partition value. */};

struct flash_sector
{
    uint32_t fs_off; /**< Fs off value. */
    uint32_t fs_size; /**< Fs size value. */};

/**
 * @brief Get id.
 * @param fa Fa value.
 */
static inline uint8_t flash_area_get_id(const struct flash_area *fa)
{
    return fa->fa_id;
}

/**
 * @brief Get device id.
 * @param fa Fa value.
 */
static inline uint8_t flash_area_get_device_id(const struct flash_area *fa)
{
    return fa->fa_device_id;
}

/**
 * @brief Get off.
 * @param fa Fa value.
 */
static inline uint32_t flash_area_get_off(const struct flash_area *fa)
{
    return fa->fa_off;
}

/**
 * @brief Get size.
 * @param fa Fa value.
 */
static inline uint32_t flash_area_get_size(const struct flash_area *fa)
{
    return fa->fa_size;
}

/**
 * @brief Get off.
 * @param sector Sector value.
 */
static inline uint32_t flash_sector_get_off(const struct flash_sector *sector)
{
    return sector->fs_off;
}

/**
 * @brief Get size.
 * @param sector Sector value.
 */
static inline uint32_t flash_sector_get_size(const struct flash_sector *sector)
{
    return sector->fs_size;
}

/**
 * @brief Set the module.
 * @param areas Areas value.
 * @param count Count value.
 */
int yi_mcuboot_flash_map_set(const struct flash_area *areas, uint32_t count);

/**
 * @brief Perform the flash area open operation.
 * @param id Id value.
 * @param area Area value.
 */
int flash_area_open(uint8_t id, const struct flash_area **area);
/**
 * @brief Perform the flash area close operation.
 * @param area Area value.
 */
void flash_area_close(const struct flash_area *area);
/**
 * @brief Read the module.
 * @param area Area value.
 * @param offset Byte offset from the start of the device.
 * @param data Driver runtime data.
 * @param length Number of bytes to process.
 */
int flash_area_read(const struct flash_area *area, uint32_t offset,
                    void *data, uint32_t length);
/**
 * @brief Write the module.
 * @param area Area value.
 * @param offset Byte offset from the start of the device.
 * @param data Driver runtime data.
 * @param length Number of bytes to process.
 */
int flash_area_write(const struct flash_area *area, uint32_t offset,
                     const void *data, uint32_t length);
/**
 * @brief Perform the flash area erase operation.
 * @param area Area value.
 * @param offset Byte offset from the start of the device.
 * @param length Number of bytes to process.
 */
int flash_area_erase(const struct flash_area *area, uint32_t offset,
                     uint32_t length);
/**
 * @brief Perform the flash area align operation.
 * @param area Area value.
 */
uint32_t flash_area_align(const struct flash_area *area);
/**
 * @brief Perform the flash area erased val operation.
 * @param area Area value.
 */
uint8_t flash_area_erased_val(const struct flash_area *area);
/**
 * @brief Get sectors.
 * @param area_id Area id value.
 * @param count Count value.
 * @param sectors Sectors value.
 */
int flash_area_get_sectors(int area_id, uint32_t *count,
                           struct flash_sector *sectors);
/**
 * @brief Get sector.
 * @param area Area value.
 * @param offset Byte offset from the start of the device.
 * @param sector Sector value.
 */
int flash_area_get_sector(const struct flash_area *area, uint32_t offset,
                          struct flash_sector *sector);
/**
 * @brief Perform the flash area id from image slot operation.
 * @param slot Slot value.
 */
int flash_area_id_from_image_slot(int slot);
/**
 * @brief Perform the flash area id from multi image slot operation.
 * @param image_index Image index value.
 * @param slot Slot value.
 */
int flash_area_id_from_multi_image_slot(int image_index, int slot);
/**
 * @brief Perform the flash area id to multi image slot operation.
 * @param image_index Image index value.
 * @param area_id Area id value.
 */
int flash_area_id_to_multi_image_slot(int image_index, int area_id);

#ifdef __cplusplus
}
#endif

#endif
