/**
 * @file yi_ds18b20.h
 * @brief YiCore ds18b20 interface.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#ifndef YI_DS18B20_H
#define YI_DS18B20_H

#include "yi_onewire.h"

#define YI_DS18B20_FAMILY_CODE 0x28U

typedef struct
{
    yi_onewire_bus_t *bus; /**< Bus value. */
    uint8_t rom[YI_ONEWIRE_ROM_SIZE]; /**< Rom value. */
    bool parasite_power; /**< Parasite power value. */} yi_ds18b20_t;

/**
 * @brief Initialize the module.
 * @param sensor Sensor value.
 * @param bus Bus value.
 * @param rom Rom value.
 * @param parasite_power Parasite power value.
 */
int yi_ds18b20_init(yi_ds18b20_t *sensor, yi_onewire_bus_t *bus,
                    const uint8_t rom[YI_ONEWIRE_ROM_SIZE],
                    bool parasite_power);
/**
 * @brief Start conversion.
 * @param sensor Sensor value.
 */
int yi_ds18b20_start_conversion(yi_ds18b20_t *sensor);
/**
 * @brief Perform the yi ds18b20 conversion ready operation.
 * @param sensor Sensor value.
 * @param ready Ready value.
 */
int yi_ds18b20_conversion_ready(yi_ds18b20_t *sensor, bool *ready);
/**
 * @brief Perform the yi ds18b20 end strong pullup operation.
 * @param sensor Sensor value.
 */
int yi_ds18b20_end_strong_pullup(yi_ds18b20_t *sensor);
/**
 * @brief Read temperature.
 * @param sensor Sensor value.
 * @param temperature_millidegrees_c Temperature millidegrees c value.
 */
int yi_ds18b20_read_temperature(yi_ds18b20_t *sensor,
                                int32_t *temperature_millidegrees_c);

#endif
