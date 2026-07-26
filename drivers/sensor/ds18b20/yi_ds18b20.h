#ifndef YI_DS18B20_H
#define YI_DS18B20_H

#include "yi_onewire.h"

#define YI_DS18B20_FAMILY_CODE 0x28U

typedef struct
{
    yi_onewire_bus_t *bus;
    uint8_t rom[YI_ONEWIRE_ROM_SIZE];
    bool parasite_power;
} yi_ds18b20_t;

int yi_ds18b20_init(yi_ds18b20_t *sensor, yi_onewire_bus_t *bus,
                    const uint8_t rom[YI_ONEWIRE_ROM_SIZE],
                    bool parasite_power);
int yi_ds18b20_start_conversion(yi_ds18b20_t *sensor);
int yi_ds18b20_conversion_ready(yi_ds18b20_t *sensor, bool *ready);
int yi_ds18b20_end_strong_pullup(yi_ds18b20_t *sensor);
int yi_ds18b20_read_temperature(yi_ds18b20_t *sensor,
                                int32_t *temperature_millidegrees_c);

#endif
