#ifndef YI_PARTITION_H
#define YI_PARTITION_H

#include <stdint.h>

#include "yi_flash.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    const char *name;
    yi_device_t *flash;
    uint32_t offset;
    uint32_t size;
} yi_partition_t;

int yi_partition_validate(const yi_partition_t *partition);
int yi_partition_read(const yi_partition_t *partition, uint32_t offset,
                      void *data, uint32_t length);
int yi_partition_write(const yi_partition_t *partition, uint32_t offset,
                       const void *data, uint32_t length);
int yi_partition_erase(const yi_partition_t *partition, uint32_t offset,
                       uint32_t length);

#ifdef __cplusplus
}
#endif

#endif
