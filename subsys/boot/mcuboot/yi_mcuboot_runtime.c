/**
 * @file yi_mcuboot_runtime.c
 * @brief Provide heap-free allocation and the minimal C runtime used by MCUboot.
 * @author Don
 * @date 2026-07-28
 * @version 1.0.0
 */

#include <stddef.h>
#include <stdint.h>

#define YI_MCUBOOT_ARENA_SIZE 2048U
#define YI_MCUBOOT_ALLOC_ALIGN 8U

static uint8_t yi_mcuboot_arena[YI_MCUBOOT_ARENA_SIZE]; /**< Fixed temporary allocation storage. */
static size_t yi_mcuboot_arena_used; /**< Next free aligned byte in the arena. */
static uint32_t yi_mcuboot_allocations; /**< Number of currently outstanding allocations. */

#if !defined(__ARMCC_VERSION) && !defined(__clang__)
typedef void (*yi_mcuboot_init_fn_t)(void);

extern yi_mcuboot_init_fn_t __preinit_array_start[]; /**< Linker-provided pre-init array start. */
extern yi_mcuboot_init_fn_t __preinit_array_end[]; /**< Linker-provided pre-init array end. */
extern yi_mcuboot_init_fn_t __init_array_start[]; /**< Linker-provided init array start. */
extern yi_mcuboot_init_fn_t __init_array_end[]; /**< Linker-provided init array end. */
#endif

/**
 * @brief Allocate aligned storage from the fixed boot-time arena.
 * @param size Requested byte count.
 * @return Allocated storage, or NULL if the request cannot fit.
 * @note Called only before jumping to the application and never from an ISR.
 */
void *yi_mcuboot_malloc(size_t size)
{
    size_t aligned_size; /**< Request rounded to the MCUboot alignment. */
    void *result; /**< Start address returned to the caller. */

    aligned_size = (size + (YI_MCUBOOT_ALLOC_ALIGN - 1U)) &
                   ~(YI_MCUBOOT_ALLOC_ALIGN - 1U);
    if((size == 0U) || (aligned_size > (YI_MCUBOOT_ARENA_SIZE -
                                        yi_mcuboot_arena_used)))
    {
        return NULL;
    }
    result = &yi_mcuboot_arena[yi_mcuboot_arena_used];
    yi_mcuboot_arena_used += aligned_size;
    yi_mcuboot_allocations++;
    return result;
}

/**
 * @brief Release one fixed-arena allocation.
 * @param pointer Allocation returned by yi_mcuboot_malloc; NULL is accepted.
 * @note The arena resets when the last outstanding allocation is released.
 */
void yi_mcuboot_free(void *pointer)
{
    if((pointer != NULL) && (yi_mcuboot_allocations != 0U))
    {
        yi_mcuboot_allocations--;
        if(yi_mcuboot_allocations == 0U)
        {
            yi_mcuboot_arena_used = 0U;
        }
    }
}

/**
 * @brief Copy non-overlapping bytes.
 * @param destination Output buffer.
 * @param source Input buffer.
 * @param length Byte count.
 * @return destination.
 */
void *memcpy(void *destination, const void *source, size_t length)
{
    uint8_t *output = destination; /**< Current output byte. */
    const uint8_t *input = source; /**< Current input byte. */

    while(length-- != 0U)
    {
        *output++ = *input++;
    }
    return destination;
}

/**
 * @brief Move bytes safely when buffers overlap.
 * @param destination Output buffer.
 * @param source Input buffer.
 * @param length Byte count.
 * @return destination.
 */
void *memmove(void *destination, const void *source, size_t length)
{
    uint8_t *output = destination; /**< Output byte cursor. */
    const uint8_t *input = source; /**< Input byte cursor. */

    if(output < input)
    {
        while(length-- != 0U)
        {
            *output++ = *input++;
        }
    }
    else
    {
        output += length;
        input += length;
        while(length-- != 0U)
        {
            *--output = *--input;
        }
    }
    return destination;
}

/**
 * @brief Fill a byte range.
 * @param destination Output buffer.
 * @param value Byte value promoted as int.
 * @param length Byte count.
 * @return destination.
 */
void *memset(void *destination, int value, size_t length)
{
    uint8_t *output = destination; /**< Current output byte. */

    while(length-- != 0U)
    {
        *output++ = (uint8_t)value;
    }
    return destination;
}

/**
 * @brief Compare two byte ranges.
 * @param left First byte range.
 * @param right Second byte range.
 * @param length Byte count.
 * @return Negative, zero, or positive comparison result.
 */
int memcmp(const void *left, const void *right, size_t length)
{
    const uint8_t *left_byte = left; /**< First range cursor. */
    const uint8_t *right_byte = right; /**< Second range cursor. */

    while(length-- != 0U)
    {
        if(*left_byte != *right_byte)
        {
            return (int)*left_byte - (int)*right_byte;
        }
        left_byte++;
        right_byte++;
    }
    return 0;
}

/**
 * @brief Compare null-terminated strings.
 * @param left First string.
 * @param right Second string.
 * @return Negative, zero, or positive comparison result.
 */
int strcmp(const char *left, const char *right)
{
    while((*left != '\0') && (*left == *right))
    {
        left++;
        right++;
    }
    return (int)(uint8_t)*left - (int)(uint8_t)*right;
}

/**
 * @brief Copy a null-terminated string.
 * @param destination Output string with sufficient storage.
 * @param source Input string.
 * @return destination.
 */
char *strcpy(char *destination, const char *source)
{
    char *output = destination; /**< Current output character. */

    do
    {
        *output++ = *source;
    } while(*source++ != '\0');
    return destination;
}

#if !defined(__ARMCC_VERSION) && !defined(__clang__)
/**
 * @brief Run linker-collected static initialization functions.
 * @note Called once by Reset_Handler before main; no destructors are needed.
 */
void __libc_init_array(void)
{
    yi_mcuboot_init_fn_t *function; /**< Current initialization entry. */

    for(function = __preinit_array_start;
        function < __preinit_array_end; function++)
    {
        (*function)();
    }
    for(function = __init_array_start;
        function < __init_array_end; function++)
    {
        (*function)();
    }
}
#endif
