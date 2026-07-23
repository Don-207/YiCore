#ifndef YI_AT24C02_H
#define YI_AT24C02_H

#include "yi_eeprom.h"
#include "yi_i2c.h"

#define YI_AT24C02_SIZE       256U
#define YI_AT24C02_PAGE_SIZE  8U

typedef struct
{
    /* Must remain first for the common EEPROM geometry accessors. */
    yi_eeprom_config_t eeprom;
    yi_device_t *self;
    yi_device_t *i2c;
    uint8_t address;
    uint32_t transfer_timeout_ms;
    uint32_t write_timeout_ms;
} yi_at24c02_config_t;

typedef struct
{
    uint32_t read_count;
    uint32_t write_count;
    uint32_t error_count;
} yi_at24c02_data_t;

int yi_at24c02_init(const void *config);
extern const yi_eeprom_api_t yi_at24c02_api;

#define YI_AT24C02_DEFINE_LEVEL(_name, _level, _priority, _config, _data) \
    YI_DEVICE_DEFINE_WITH_API(                                           \
        _name, _level, _priority, yi_at24c02_init,                       \
        &_config, &_data, (const yi_device_api_t *)&yi_at24c02_api       \
    )

#endif
