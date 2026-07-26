/**
 * @file yi_partition.h
 * @brief YiCore partition interface.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#ifndef YI_PARTITION_H
#define YI_PARTITION_H

#include <stdint.h>

#include "yi_flash.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    const char *name; /**< Name value. */
    yi_device_t *flash; /**< Flash value. */
    uint32_t offset; /**< Offset value. */
    uint32_t size; /**< Size value. */} yi_partition_t;

/**
 * @brief Validate the module.
 * @param partition Partition value.
 */
int yi_partition_validate(const yi_partition_t *partition);
/**
 * @brief Read the module.
 * @param partition Partition value.
 * @param offset Byte offset from the start of the device.
 * @param data Driver runtime data.
 * @param length Number of bytes to process.
 */
int yi_partition_read(const yi_partition_t *partition, uint32_t offset,
                      void *data, uint32_t length);
/**
 * @brief Write the module.
 * @param partition Partition value.
 * @param offset Byte offset from the start of the device.
 * @param data Driver runtime data.
 * @param length Number of bytes to process.
 */
int yi_partition_write(const yi_partition_t *partition, uint32_t offset,
                       const void *data, uint32_t length);
/**
 * @brief Perform the yi partition erase operation.
 * @param partition Partition value.
 * @param offset Byte offset from the start of the device.
 * @param length Number of bytes to process.
 */
int yi_partition_erase(const yi_partition_t *partition, uint32_t offset,
                       uint32_t length);

#ifdef __cplusplus
}
#endif

#endif
