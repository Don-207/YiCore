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
    uint8_t fa_id;
    uint8_t fa_device_id;
    uint16_t pad16;
    uint32_t fa_off;
    uint32_t fa_size;
    const yi_partition_t *partition;
};

struct flash_sector
{
    uint32_t fs_off;
    uint32_t fs_size;
};

static inline uint8_t flash_area_get_id(const struct flash_area *fa)
{
    return fa->fa_id;
}

static inline uint8_t flash_area_get_device_id(const struct flash_area *fa)
{
    return fa->fa_device_id;
}

static inline uint32_t flash_area_get_off(const struct flash_area *fa)
{
    return fa->fa_off;
}

static inline uint32_t flash_area_get_size(const struct flash_area *fa)
{
    return fa->fa_size;
}

static inline uint32_t flash_sector_get_off(const struct flash_sector *sector)
{
    return sector->fs_off;
}

static inline uint32_t flash_sector_get_size(const struct flash_sector *sector)
{
    return sector->fs_size;
}

int yi_mcuboot_flash_map_set(const struct flash_area *areas, uint32_t count);

int flash_area_open(uint8_t id, const struct flash_area **area);
void flash_area_close(const struct flash_area *area);
int flash_area_read(const struct flash_area *area, uint32_t offset,
                    void *data, uint32_t length);
int flash_area_write(const struct flash_area *area, uint32_t offset,
                     const void *data, uint32_t length);
int flash_area_erase(const struct flash_area *area, uint32_t offset,
                     uint32_t length);
uint32_t flash_area_align(const struct flash_area *area);
uint8_t flash_area_erased_val(const struct flash_area *area);
int flash_area_get_sectors(int area_id, uint32_t *count,
                           struct flash_sector *sectors);
int flash_area_get_sector(const struct flash_area *area, uint32_t offset,
                          struct flash_sector *sector);
int flash_area_id_from_image_slot(int slot);
int flash_area_id_from_multi_image_slot(int image_index, int slot);
int flash_area_id_to_multi_image_slot(int image_index, int area_id);

#ifdef __cplusplus
}
#endif

#endif
