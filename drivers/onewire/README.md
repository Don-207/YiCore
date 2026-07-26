# YiCore 1-Wire driver

`yi_onewire` is the platform-independent Dallas/Maxim 1-Wire bus layer. It
contains reset/presence detection, LSB-first bit and byte transfers, ROM
selection, ROM search, and CRC-8. Device-specific commands remain in separate
drivers; `yi_ds18b20` is the first example.

The GPIO adapter must implement true open-drain behaviour. `drive_low()` drives
the wire low and `release()` puts the pin in high impedance; do not actively
drive the wire high. Use an external pull-up resistor. `delay_us()` must be a
blocking microsecond delay. Supplying the optional critical-section pair is
recommended when interrupt latency can violate a time slot.

```c
static yi_onewire_bus_t temperature_bus;

static const yi_onewire_hal_t temperature_hal = {
    .drive_low = board_ow_drive_low,
    .release = board_ow_release,
    .read = board_ow_read,
    .delay_us = board_delay_us,
    .critical_enter = board_ow_lock,
    .critical_exit = board_ow_unlock,
    .strong_pullup = board_ow_strong_pullup, /* NULL if unused */
    .context = &board_ow_pin,
};

yi_onewire_init(&temperature_bus, &temperature_hal);

yi_onewire_search_t search;
uint8_t rom[YI_ONEWIRE_ROM_SIZE];
yi_onewire_search_reset(&search);
while(yi_onewire_search_next(&temperature_bus, &search, rom) == YI_ONEWIRE_OK) {
    /* Register a driver based on rom[0], the family code. */
}
```

For an externally powered DS18B20, call `yi_ds18b20_start_conversion()`, poll
`yi_ds18b20_conversion_ready()`, then call `yi_ds18b20_read_temperature()`.
For parasite power, start conversion, keep the strong pull-up enabled for the
conversion time from the data sheet (up to 750 ms at 12-bit resolution), call
`yi_ds18b20_end_strong_pullup()`, and then read the temperature. Conversion
waiting is deliberately left to the application so the driver does not block
an RTOS task or force a particular timer API.
