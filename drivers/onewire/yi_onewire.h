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
    void *context;
} yi_onewire_hal_t;

typedef struct
{
    yi_onewire_hal_t hal;
} yi_onewire_bus_t;

typedef struct
{
    uint8_t rom[YI_ONEWIRE_ROM_SIZE];
    uint8_t last_discrepancy;
    bool last_device;
} yi_onewire_search_t;

int yi_onewire_init(yi_onewire_bus_t *bus, const yi_onewire_hal_t *hal);
int yi_onewire_reset(yi_onewire_bus_t *bus);
int yi_onewire_write_bit(yi_onewire_bus_t *bus, bool value);
int yi_onewire_read_bit(yi_onewire_bus_t *bus, bool *value);
int yi_onewire_write(yi_onewire_bus_t *bus, const uint8_t *data, size_t length);
int yi_onewire_read(yi_onewire_bus_t *bus, uint8_t *data, size_t length);
int yi_onewire_select(yi_onewire_bus_t *bus, const uint8_t rom[YI_ONEWIRE_ROM_SIZE]);
int yi_onewire_skip(yi_onewire_bus_t *bus);

void yi_onewire_search_reset(yi_onewire_search_t *search);
int yi_onewire_search_next(yi_onewire_bus_t *bus,
                           yi_onewire_search_t *search,
                           uint8_t rom[YI_ONEWIRE_ROM_SIZE]);

uint8_t yi_onewire_crc8(const uint8_t *data, size_t length);
bool yi_onewire_rom_valid(const uint8_t rom[YI_ONEWIRE_ROM_SIZE]);

#endif
