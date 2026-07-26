/**
 * @file yi_onewire.h
 * @brief YiCore onewire interface.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#ifndef YI_ONEWIRE_H
#define YI_ONEWIRE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define YI_ONEWIRE_ROM_SIZE 8U

#define YI_ONEWIRE_CMD_SEARCH_ROM 0xF0U
#define YI_ONEWIRE_CMD_READ_ROM   0x33U
#define YI_ONEWIRE_CMD_MATCH_ROM  0x55U
#define YI_ONEWIRE_CMD_SKIP_ROM   0xCCU

typedef enum
{
    YI_ONEWIRE_OK = 0,
    YI_ONEWIRE_DONE = 1,
    YI_ONEWIRE_ERROR_ARGUMENT = -1,
    YI_ONEWIRE_ERROR_IO = -2,
    YI_ONEWIRE_ERROR_NO_DEVICE = -3,
    YI_ONEWIRE_ERROR_BUS = -4,
    YI_ONEWIRE_ERROR_CRC = -5,
    YI_ONEWIRE_ERROR_UNSUPPORTED = -6
} yi_onewire_result_t;

/* The pin must behave as open drain: drive_low asserts it and release
 * switches it to high impedance. An external pull-up is required. */
typedef struct
{
    int (*drive_low)(void *context);
    int (*release)(void *context);
    int (*read)(void *context);       /* Returns 0, 1, or a negative error. */
    void (*delay_us)(void *context, uint32_t delay_us);
    void (*critical_enter)(void *context); /* Optional, but recommended. */
    void (*critical_exit)(void *context);  /* Optional, but recommended. */
    int (*strong_pullup)(void *context, bool enable); /* Optional. */
    void *context; /**< Context value. */} yi_onewire_hal_t;

typedef struct
{
    yi_onewire_hal_t hal; /**< Hal value. */} yi_onewire_bus_t;

typedef struct
{
    uint8_t rom[YI_ONEWIRE_ROM_SIZE]; /**< Rom value. */
    uint8_t last_discrepancy; /**< Last discrepancy value. */
    bool last_device; /**< Last device value. */} yi_onewire_search_t;

/**
 * @brief Initialize the module.
 * @param bus Bus value.
 * @param hal Hal value.
 */
int yi_onewire_init(yi_onewire_bus_t *bus, const yi_onewire_hal_t *hal);
/**
 * @brief Perform the yi onewire reset operation.
 * @param bus Bus value.
 */
int yi_onewire_reset(yi_onewire_bus_t *bus);
/**
 * @brief Write bit.
 * @param bus Bus value.
 * @param value Value to process.
 */
int yi_onewire_write_bit(yi_onewire_bus_t *bus, bool value);
/**
 * @brief Read bit.
 * @param bus Bus value.
 * @param value Value to process.
 */
int yi_onewire_read_bit(yi_onewire_bus_t *bus, bool *value);
/**
 * @brief Write the module.
 * @param bus Bus value.
 * @param data Driver runtime data.
 * @param length Number of bytes to process.
 */
int yi_onewire_write(yi_onewire_bus_t *bus, const uint8_t *data, size_t length);
/**
 * @brief Read the module.
 * @param bus Bus value.
 * @param data Driver runtime data.
 * @param length Number of bytes to process.
 */
int yi_onewire_read(yi_onewire_bus_t *bus, uint8_t *data, size_t length);
/**
 * @brief Perform the yi onewire select operation.
 * @param bus Bus value.
 * @param rom Rom value.
 */
int yi_onewire_select(yi_onewire_bus_t *bus, const uint8_t rom[YI_ONEWIRE_ROM_SIZE]);
/**
 * @brief Perform the yi onewire skip operation.
 * @param bus Bus value.
 */
int yi_onewire_skip(yi_onewire_bus_t *bus);

/**
 * @brief Perform the yi onewire search reset operation.
 * @param search Search value.
 */
void yi_onewire_search_reset(yi_onewire_search_t *search);
/**
 * @brief Perform the yi onewire search next operation.
 * @param bus Bus value.
 * @param search Search value.
 * @param rom Rom value.
 */
int yi_onewire_search_next(yi_onewire_bus_t *bus,
                           yi_onewire_search_t *search,
                           uint8_t rom[YI_ONEWIRE_ROM_SIZE]);

/**
 * @brief Perform the yi onewire crc8 operation.
 * @param data Driver runtime data.
 * @param length Number of bytes to process.
 */
uint8_t yi_onewire_crc8(const uint8_t *data, size_t length);
/**
 * @brief Perform the yi onewire rom valid operation.
 * @param rom Rom value.
 */
bool yi_onewire_rom_valid(const uint8_t rom[YI_ONEWIRE_ROM_SIZE]);

#endif
