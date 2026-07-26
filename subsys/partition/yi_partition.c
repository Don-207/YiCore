#include "yi_partition.h"

static int yi_partition_range_valid(const yi_partition_t *partition,
                                    uint32_t offset, uint32_t length)
{
    return (partition != NULL) &&
           (offset <= partition->size) &&
           (length <= (partition->size - offset));
}

int yi_partition_validate(const yi_partition_t *partition)
{
    uint32_t flash_size;

    if((partition == NULL) || (partition->name == NULL) ||
       !yi_device_is_ready(partition->flash) || (partition->size == 0U))
    {
        return -1;
    }

    flash_size = yi_flash_get_size(partition->flash);
    if((partition->offset > flash_size) ||
       (partition->size > (flash_size - partition->offset)))
    {
        return -1;
    }

    return 0;
}

int yi_partition_read(const yi_partition_t *partition, uint32_t offset,
                      void *data, uint32_t length)
{
    if((yi_partition_validate(partition) != 0) ||
       !yi_partition_range_valid(partition, offset, length))
    {
        return -1;
    }

    return yi_flash_read(partition->flash, partition->offset + offset,
                         data, length);
}

int yi_partition_write(const yi_partition_t *partition, uint32_t offset,
                       const void *data, uint32_t length)
{
    if((yi_partition_validate(partition) != 0) ||
       !yi_partition_range_valid(partition, offset, length))
    {
        return -1;
    }

    return yi_flash_write(partition->flash, partition->offset + offset,
                          data, length);
}

int yi_partition_erase(const yi_partition_t *partition, uint32_t offset,
                       uint32_t length)
{
    if((yi_partition_validate(partition) != 0) ||
       !yi_partition_range_valid(partition, offset, length))
    {
        return -1;
    }

    return yi_flash_erase(partition->flash, partition->offset + offset,
                          length);
}
