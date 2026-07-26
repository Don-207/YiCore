#include "yi_ds18b20.h"

#include <string.h>

#define DS18B20_CMD_CONVERT_T       0x44U
#define DS18B20_CMD_READ_SCRATCHPAD 0xBEU
#define DS18B20_SCRATCHPAD_SIZE     9U

static int yi_ds18b20_address(yi_ds18b20_t *sensor)
{
    int result = yi_onewire_reset(sensor->bus);
    return (result == YI_ONEWIRE_OK) ? yi_onewire_select(sensor->bus, sensor->rom) : result;
}

int yi_ds18b20_init(yi_ds18b20_t *sensor, yi_onewire_bus_t *bus,
                    const uint8_t rom[YI_ONEWIRE_ROM_SIZE], bool parasite_power)
{
    if((sensor == NULL) || (bus == NULL) || !yi_onewire_rom_valid(rom) ||
       (rom[0] != YI_DS18B20_FAMILY_CODE) ||
       (parasite_power && (bus->hal.strong_pullup == NULL)))
    {
        return YI_ONEWIRE_ERROR_ARGUMENT;
    }
    sensor->bus = bus;
    memcpy(sensor->rom, rom, YI_ONEWIRE_ROM_SIZE);
    sensor->parasite_power = parasite_power;
    return YI_ONEWIRE_OK;
}

int yi_ds18b20_start_conversion(yi_ds18b20_t *sensor)
{
    const uint8_t command = DS18B20_CMD_CONVERT_T;
    int result;
    if((sensor == NULL) || (sensor->bus == NULL)) { return YI_ONEWIRE_ERROR_ARGUMENT; }
    result = yi_ds18b20_address(sensor);
    if(result == YI_ONEWIRE_OK) { result = yi_onewire_write(sensor->bus, &command, 1U); }
    if((result == YI_ONEWIRE_OK) && sensor->parasite_power &&
       (sensor->bus->hal.strong_pullup(sensor->bus->hal.context, true) != 0))
    {
        result = YI_ONEWIRE_ERROR_IO;
    }
    return result;
}

int yi_ds18b20_conversion_ready(yi_ds18b20_t *sensor, bool *ready)
{
    if((sensor == NULL) || (sensor->bus == NULL) || (ready == NULL))
    {
        return YI_ONEWIRE_ERROR_ARGUMENT;
    }
    if(sensor->parasite_power) { return YI_ONEWIRE_ERROR_UNSUPPORTED; }
    return yi_onewire_read_bit(sensor->bus, ready);
}

int yi_ds18b20_end_strong_pullup(yi_ds18b20_t *sensor)
{
    if((sensor == NULL) || (sensor->bus == NULL)) { return YI_ONEWIRE_ERROR_ARGUMENT; }
    if(!sensor->parasite_power) { return YI_ONEWIRE_OK; }
    return (sensor->bus->hal.strong_pullup(sensor->bus->hal.context, false) == 0) ?
           YI_ONEWIRE_OK : YI_ONEWIRE_ERROR_IO;
}

int yi_ds18b20_read_temperature(yi_ds18b20_t *sensor,
                                int32_t *temperature_millidegrees_c)
{
    const uint8_t command = DS18B20_CMD_READ_SCRATCHPAD;
    uint8_t scratchpad[DS18B20_SCRATCHPAD_SIZE];
    int16_t raw;
    int32_t scaled;
    int result;

    if((sensor == NULL) || (sensor->bus == NULL) ||
       (temperature_millidegrees_c == NULL))
    {
        return YI_ONEWIRE_ERROR_ARGUMENT;
    }
    result = yi_ds18b20_address(sensor);
    if(result == YI_ONEWIRE_OK) { result = yi_onewire_write(sensor->bus, &command, 1U); }
    if(result == YI_ONEWIRE_OK) { result = yi_onewire_read(sensor->bus, scratchpad, sizeof(scratchpad)); }
    if(result != YI_ONEWIRE_OK) { return result; }
    if(yi_onewire_crc8(scratchpad, sizeof(scratchpad) - 1U) != scratchpad[8])
    {
        return YI_ONEWIRE_ERROR_CRC;
    }
    raw = (int16_t)((uint16_t)scratchpad[0] | ((uint16_t)scratchpad[1] << 8U));
    scaled = (int32_t)raw * 1000;
    *temperature_millidegrees_c = (scaled >= 0) ?
                                     (scaled + 8) / 16 :
                                     (scaled - 8) / 16;
    return YI_ONEWIRE_OK;
}
