/**
 * @file os_malloc.h
 * @brief Route MCUboot temporary allocations to a fixed boot-time arena.
 * @author Don
 * @date 2026-07-28
 * @version 1.0.0
 */

#ifndef YI_MCUBOOT_OS_MALLOC_H
#define YI_MCUBOOT_OS_MALLOC_H

#include <stddef.h>

/**
 * @brief Allocate aligned storage from the fixed MCUboot arena.
 * @param size Requested byte count.
 * @return Allocated storage, or NULL when the arena is exhausted.
 * @note Bootloader main-loop context only; this allocator is not thread-safe.
 */
void *yi_mcuboot_malloc(size_t size);

/**
 * @brief Release an arena allocation.
 * @param pointer Storage previously returned by yi_mcuboot_malloc.
 * @note Storage is reclaimed together when all outstanding allocations close.
 */
void yi_mcuboot_free(void *pointer);

#define malloc(size) yi_mcuboot_malloc(size)
#define free(pointer) yi_mcuboot_free(pointer)
#define os_malloc(size) yi_mcuboot_malloc(size)
#define os_free(pointer) yi_mcuboot_free(pointer)

#endif
