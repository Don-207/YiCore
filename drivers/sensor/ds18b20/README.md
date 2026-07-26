# DS18B20 driver

DS18B20 temperature sensor support on the YiCore 1-Wire bus. The driver uses
64-bit ROM selection, validates scratchpad CRC, supports normal and parasite
power, and returns temperature in millidegrees Celsius.

```c
yi_ds18b20_t sensor;
int32_t temperature_mc;

yi_ds18b20_init(&sensor, &bus, rom, false);
yi_ds18b20_start_conversion(&sensor);
yi_ds18b20_conversion_ready(&sensor, &ready);
yi_ds18b20_read_temperature(&sensor, &temperature_mc);
```

Parasite-powered conversions require a bus implementation with strong
pull-up support. Release it with `yi_ds18b20_end_strong_pullup()` after the
conversion time has elapsed.
