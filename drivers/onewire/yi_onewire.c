/**
 * @file yi_onewire.c
 * @brief YiCore onewire implementation.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#include "yi_onewire.h"

#include <string.h>

/**
 * @brief Perform the yi onewire bus valid operation.
 * @param bus Bus value.
 */
static bool yi_onewire_bus_valid(const yi_onewire_bus_t *bus)
{
    return (bus != NULL) && (bus->hal.drive_low != NULL) &&
           (bus->hal.release != NULL) && (bus->hal.read != NULL) &&
           (bus->hal.delay_us != NULL);
}

/**
 * @brief Perform the yi onewire enter operation.
 * @param bus Bus value.
 */
static void yi_onewire_enter(yi_onewire_bus_t *bus)
{
    if(bus->hal.critical_enter != NULL) { bus->hal.critical_enter(bus->hal.context); }
}

/**
 * @brief Perform the yi onewire exit operation.
 * @param bus Bus value.
 */
static void yi_onewire_exit(yi_onewire_bus_t *bus)
{
    if(bus->hal.critical_exit != NULL) { bus->hal.critical_exit(bus->hal.context); }
}

/**
 * @brief Perform the yi onewire delay operation.
 * @param bus Bus value.
 * @param delay_us Delay us value.
 */
static void yi_onewire_delay(yi_onewire_bus_t *bus, uint32_t delay_us)
{
    bus->hal.delay_us(bus->hal.context, delay_us);
}

/**
 * @brief Initialize the module.
 * @param bus Bus value.
 * @param hal Hal value.
 */
int yi_onewire_init(yi_onewire_bus_t *bus, const yi_onewire_hal_t *hal)
{
    if((bus == NULL) || (hal == NULL) || (hal->drive_low == NULL) ||
       (hal->release == NULL) || (hal->read == NULL) ||
       (hal->delay_us == NULL) ||
       ((hal->critical_enter == NULL) != (hal->critical_exit == NULL)))
    {
        return YI_ONEWIRE_ERROR_ARGUMENT;
    }

    bus->hal = *hal;
    if(bus->hal.release(bus->hal.context) != 0) { return YI_ONEWIRE_ERROR_IO; }
    return YI_ONEWIRE_OK;
}

/**
 * @brief Perform the yi onewire reset operation.
 * @param bus Bus value.
 */
int yi_onewire_reset(yi_onewire_bus_t *bus)
{
    int level;

    if(!yi_onewire_bus_valid(bus)) { return YI_ONEWIRE_ERROR_ARGUMENT; }
    if(bus->hal.release(bus->hal.context) != 0) { return YI_ONEWIRE_ERROR_IO; }
    yi_onewire_delay(bus, 5U);
    level = bus->hal.read(bus->hal.context);
    if(level < 0) { return YI_ONEWIRE_ERROR_IO; }
    if(level == 0) { return YI_ONEWIRE_ERROR_BUS; }

    yi_onewire_enter(bus);
    if(bus->hal.drive_low(bus->hal.context) != 0)
    {
        yi_onewire_exit(bus);
        return YI_ONEWIRE_ERROR_IO;
    }
    yi_onewire_delay(bus, 480U);
    if(bus->hal.release(bus->hal.context) != 0)
    {
        yi_onewire_exit(bus);
        return YI_ONEWIRE_ERROR_IO;
    }
    yi_onewire_delay(bus, 70U);
    level = bus->hal.read(bus->hal.context);
    yi_onewire_delay(bus, 410U);
    yi_onewire_exit(bus);

    if(level < 0) { return YI_ONEWIRE_ERROR_IO; }
    return (level == 0) ? YI_ONEWIRE_OK : YI_ONEWIRE_ERROR_NO_DEVICE;
}

/**
 * @brief Write bit.
 * @param bus Bus value.
 * @param value Value to process.
 */
int yi_onewire_write_bit(yi_onewire_bus_t *bus, bool value)
{
    int result = YI_ONEWIRE_OK;

    if(!yi_onewire_bus_valid(bus)) { return YI_ONEWIRE_ERROR_ARGUMENT; }
    yi_onewire_enter(bus);
    if(bus->hal.drive_low(bus->hal.context) != 0) { result = YI_ONEWIRE_ERROR_IO; }
    else
    {
        yi_onewire_delay(bus, value ? 6U : 60U);
        if(bus->hal.release(bus->hal.context) != 0) { result = YI_ONEWIRE_ERROR_IO; }
        yi_onewire_delay(bus, value ? 64U : 10U);
    }
    yi_onewire_exit(bus);
    return result;
}

/**
 * @brief Read bit.
 * @param bus Bus value.
 * @param value Value to process.
 */
int yi_onewire_read_bit(yi_onewire_bus_t *bus, bool *value)
{
    int level;

    if(!yi_onewire_bus_valid(bus) || (value == NULL))
    {
        return YI_ONEWIRE_ERROR_ARGUMENT;
    }
    yi_onewire_enter(bus);
    if(bus->hal.drive_low(bus->hal.context) != 0)
    {
        yi_onewire_exit(bus);
        return YI_ONEWIRE_ERROR_IO;
    }
    yi_onewire_delay(bus, 6U);
    if(bus->hal.release(bus->hal.context) != 0)
    {
        yi_onewire_exit(bus);
        return YI_ONEWIRE_ERROR_IO;
    }
    yi_onewire_delay(bus, 9U);
    level = bus->hal.read(bus->hal.context);
    yi_onewire_delay(bus, 55U);
    yi_onewire_exit(bus);
    if(level < 0) { return YI_ONEWIRE_ERROR_IO; }
    *value = level != 0;
    return YI_ONEWIRE_OK;
}

/**
 * @brief Write the module.
 * @param bus Bus value.
 * @param data Driver runtime data.
 * @param length Number of bytes to process.
 */
int yi_onewire_write(yi_onewire_bus_t *bus, const uint8_t *data, size_t length)
{
    if(!yi_onewire_bus_valid(bus) || ((data == NULL) && (length != 0U)))
    {
        return YI_ONEWIRE_ERROR_ARGUMENT;
    }
    for(size_t index = 0U; index < length; index++)
    {
        for(uint8_t bit = 0U; bit < 8U; bit++)
        {
            int result = yi_onewire_write_bit(bus, (data[index] & (1U << bit)) != 0U);
            if(result != YI_ONEWIRE_OK) { return result; }
        }
    }
    return YI_ONEWIRE_OK;
}

/**
 * @brief Read the module.
 * @param bus Bus value.
 * @param data Driver runtime data.
 * @param length Number of bytes to process.
 */
int yi_onewire_read(yi_onewire_bus_t *bus, uint8_t *data, size_t length)
{
    if(!yi_onewire_bus_valid(bus) || ((data == NULL) && (length != 0U)))
    {
        return YI_ONEWIRE_ERROR_ARGUMENT;
    }
    for(size_t index = 0U; index < length; index++)
    {
        uint8_t value = 0U;
        for(uint8_t bit = 0U; bit < 8U; bit++)
        {
            bool sampled;
            int result = yi_onewire_read_bit(bus, &sampled);
            if(result != YI_ONEWIRE_OK) { return result; }
            if(sampled) { value |= (uint8_t)(1U << bit); }
        }
        data[index] = value;
    }
    return YI_ONEWIRE_OK;
}

/**
 * @brief Perform the yi onewire select operation.
 * @param bus Bus value.
 * @param rom Rom value.
 */
int yi_onewire_select(yi_onewire_bus_t *bus, const uint8_t rom[YI_ONEWIRE_ROM_SIZE])
{
    const uint8_t command = YI_ONEWIRE_CMD_MATCH_ROM;
    int result;

    if(rom == NULL) { return YI_ONEWIRE_ERROR_ARGUMENT; }
    result = yi_onewire_write(bus, &command, 1U);
    return (result == YI_ONEWIRE_OK) ? yi_onewire_write(bus, rom, YI_ONEWIRE_ROM_SIZE) : result;
}

/**
 * @brief Perform the yi onewire skip operation.
 * @param bus Bus value.
 */
int yi_onewire_skip(yi_onewire_bus_t *bus)
{
    const uint8_t command = YI_ONEWIRE_CMD_SKIP_ROM;
    /**
     * @brief Write the module.
     * @param bus Bus value.
     * @param command Command value.
     * @param U U value.
     */
    return yi_onewire_write(bus, &command, 1U);
}

/**
 * @brief Perform the yi onewire search reset operation.
 * @param search Search value.
 */
void yi_onewire_search_reset(yi_onewire_search_t *search)
{
    if(search != NULL) { memset(search, 0, sizeof(*search)); }
}

/**
 * @brief Perform the yi onewire search next operation.
 * @param bus Bus value.
 * @param search Search value.
 * @param rom Rom value.
 */
int yi_onewire_search_next(yi_onewire_bus_t *bus, yi_onewire_search_t *search,
                           uint8_t rom[YI_ONEWIRE_ROM_SIZE])
{
    const uint8_t command = YI_ONEWIRE_CMD_SEARCH_ROM;
    uint8_t next_discrepancy = 0U;
    int result;

    if(!yi_onewire_bus_valid(bus) || (search == NULL) || (rom == NULL))
    {
        return YI_ONEWIRE_ERROR_ARGUMENT;
    }
    if(search->last_device) { return YI_ONEWIRE_DONE; }
    result = yi_onewire_reset(bus);
    if(result != YI_ONEWIRE_OK) { return result; }
    result = yi_onewire_write(bus, &command, 1U);
    if(result != YI_ONEWIRE_OK) { return result; }

    for(uint8_t position = 1U; position <= 64U; position++)
    {
        bool bit;
        bool complement;
        bool direction;
        uint8_t byte_index = (uint8_t)((position - 1U) / 8U);
        uint8_t mask = (uint8_t)(1U << ((position - 1U) % 8U));

        if((yi_onewire_read_bit(bus, &bit) != YI_ONEWIRE_OK) ||
           (yi_onewire_read_bit(bus, &complement) != YI_ONEWIRE_OK))
        {
            return YI_ONEWIRE_ERROR_IO;
        }
        if(bit && complement) { return YI_ONEWIRE_ERROR_BUS; }
        if(bit != complement) { direction = bit; }
        else
        {
            if(position < search->last_discrepancy)
            {
                direction = (search->rom[byte_index] & mask) != 0U;
            }
            else { direction = position == search->last_discrepancy; }
            if(!direction) { next_discrepancy = position; }
        }
        if(direction) { search->rom[byte_index] |= mask; }
        else { search->rom[byte_index] &= (uint8_t)~mask; }
        result = yi_onewire_write_bit(bus, direction);
        if(result != YI_ONEWIRE_OK) { return result; }
    }

    if(!yi_onewire_rom_valid(search->rom)) { return YI_ONEWIRE_ERROR_CRC; }
    search->last_discrepancy = next_discrepancy;
    search->last_device = next_discrepancy == 0U;
    memcpy(rom, search->rom, YI_ONEWIRE_ROM_SIZE);
    return YI_ONEWIRE_OK;
}

/**
 * @brief Perform the yi onewire crc8 operation.
 * @param data Driver runtime data.
 * @param length Number of bytes to process.
 */
uint8_t yi_onewire_crc8(const uint8_t *data, size_t length)
{
    uint8_t crc = 0U;
    if(data == NULL) { return 0U; }
    for(size_t index = 0U; index < length; index++)
    {
        uint8_t value = data[index];
        for(uint8_t bit = 0U; bit < 8U; bit++)
        {
            bool mix = ((crc ^ value) & 1U) != 0U;
            crc >>= 1U;
            if(mix) { crc ^= 0x8CU; }
            value >>= 1U;
        }
    }
    return crc;
}

/**
 * @brief Perform the yi onewire rom valid operation.
 * @param rom Rom value.
 */
bool yi_onewire_rom_valid(const uint8_t rom[YI_ONEWIRE_ROM_SIZE])
{
    return (rom != NULL) && (yi_onewire_crc8(rom, YI_ONEWIRE_ROM_SIZE - 1U) == rom[7]);
}
