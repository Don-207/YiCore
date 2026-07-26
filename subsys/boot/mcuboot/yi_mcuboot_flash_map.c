#include "flash_map_backend/flash_map_backend.h"

#include <stddef.h>

static const struct flash_area *yi_flash_areas;
static uint32_t yi_flash_area_count;

static int yi_mcuboot_area_valid(const struct flash_area *area)
{
    return (area != NULL) && (area->partition != NULL) &&
           (area->fa_off == area->partition->offset) &&
           (area->fa_size == area->partition->size) &&
           (yi_partition_validate(area->partition) == 0);
}

int yi_mcuboot_flash_map_set(const struct flash_area *areas, uint32_t count)
{
    uint32_t i;
    uint32_t j;

    if((areas == NULL) || (count == 0U))
    {
        return -1;
    }

    for(i = 0U; i < count; i++)
    {
        if(!yi_mcuboot_area_valid(&areas[i]))
        {
            return -1;
        }

        for(j = 0U; j < i; j++)
        {
            if(areas[j].fa_id == areas[i].fa_id)
            {
                return -1;
            }

            if(areas[j].partition->flash == areas[i].partition->flash)
            {
                uint32_t first_end = areas[j].fa_off + areas[j].fa_size;
                uint32_t second_end = areas[i].fa_off + areas[i].fa_size;

                if((areas[j].fa_off < second_end) &&
                   (areas[i].fa_off < first_end))
                {
                    return -1;
                }
            }
        }
    }

    yi_flash_areas = areas;
    yi_flash_area_count = count;
    return 0;
}

int flash_area_open(uint8_t id, const struct flash_area **area)
{
    uint32_t i;

    if(area == NULL)
    {
        return -1;
    }

    *area = NULL;
    for(i = 0U; i < yi_flash_area_count; i++)
    {
        if(yi_flash_areas[i].fa_id == id)
        {
            *area = &yi_flash_areas[i];
            return 0;
        }
    }

    return -1;
}

void flash_area_close(const struct flash_area *area)
{
    (void)area;
}

int flash_area_read(const struct flash_area *area, uint32_t offset,
                    void *data, uint32_t length)
{
    return yi_mcuboot_area_valid(area) ?
           yi_partition_read(area->partition, offset, data, length) : -1;
}

int flash_area_write(const struct flash_area *area, uint32_t offset,
                     const void *data, uint32_t length)
{
    return yi_mcuboot_area_valid(area) ?
           yi_partition_write(area->partition, offset, data, length) : -1;
}

int flash_area_erase(const struct flash_area *area, uint32_t offset,
                     uint32_t length)
{
    return yi_mcuboot_area_valid(area) ?
           yi_partition_erase(area->partition, offset, length) : -1;
}

uint32_t flash_area_align(const struct flash_area *area)
{
    return yi_mcuboot_area_valid(area) ?
           yi_flash_get_write_block_size(area->partition->flash) : 0U;
}

uint8_t flash_area_erased_val(const struct flash_area *area)
{
    (void)area;
    return 0xFFU;
}

int flash_area_get_sectors(int area_id, uint32_t *count,
                           struct flash_sector *sectors)
{
    const struct flash_area *area;
    uint32_t capacity;
    uint32_t erase_size;
    uint32_t required;
    uint32_t written;

    if((count == NULL) || (flash_area_open((uint8_t)area_id, &area) != 0))
    {
        return -1;
    }

    erase_size = yi_flash_get_erase_block_size(area->partition->flash);
    if((erase_size == 0U) || ((area->fa_off % erase_size) != 0U) ||
       ((area->fa_size % erase_size) != 0U))
    {
        return -1;
    }

    capacity = *count;
    required = area->fa_size / erase_size;
    written = (capacity < required) ? capacity : required;
    if((written != 0U) && (sectors == NULL))
    {
        return -1;
    }

    for(uint32_t i = 0U; i < written; i++)
    {
        sectors[i].fs_off = i * erase_size;
        sectors[i].fs_size = erase_size;
    }

    *count = required;
    return 0;
}

int flash_area_get_sector(const struct flash_area *area, uint32_t offset,
                          struct flash_sector *sector)
{
    uint32_t erase_size;

    if(!yi_mcuboot_area_valid(area) || (sector == NULL) ||
       (offset >= area->fa_size))
    {
        return -1;
    }

    erase_size = yi_flash_get_erase_block_size(area->partition->flash);
    if((erase_size == 0U) || ((area->fa_off % erase_size) != 0U))
    {
        return -1;
    }

    sector->fs_off = (offset / erase_size) * erase_size;
    sector->fs_size = erase_size;
    return 0;
}

int flash_area_id_from_image_slot(int slot)
{
    return flash_area_id_from_multi_image_slot(0, slot);
}

int flash_area_id_from_multi_image_slot(int image_index, int slot)
{
    if((image_index != 0) || ((slot != 0) && (slot != 1)))
    {
        return -1;
    }

    return (slot == 0) ? FLASH_AREA_IMAGE_PRIMARY(0) :
                         FLASH_AREA_IMAGE_SECONDARY(0);
}

int flash_area_id_to_multi_image_slot(int image_index, int area_id)
{
    if(image_index != 0)
    {
        return -1;
    }

    if(area_id == FLASH_AREA_IMAGE_PRIMARY(0))
    {
        return 0;
    }
    if(area_id == FLASH_AREA_IMAGE_SECONDARY(0))
    {
        return 1;
    }

    return -1;
}
