# MAX31856 driver

SPI thermocouple converter driver supporting B/E/J/K/N/R/S/T types, 50/60 Hz
filtering, averaging, open-circuit timing, signed temperature conversion, and
fault-status reporting. The device uses SPI mode 1.

```dts
tc0: thermocouple-max31856 {
    compatible = "maxim,max31856";
    bus = <&spi1>;
    cs-gpio = <&tc_cs_gpio>;
    thermocouple-type = "k";
    filter-hz = <50>;
    average-samples = <4>;
    open-circuit-ms = <100>;
    status = "okay";
};
```

```c
int32_t temperature_mc;
uint8_t faults;
yi_max31856_read(tc, &temperature_mc, &faults);
```
