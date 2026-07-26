# TSYS01 driver

High-accuracy I2C temperature sensor driver with reset, PROM coefficient
loading, optional PROM checksum validation, conversion timing, raw ADC access,
and polynomial temperature compensation.

```dts
temp0: temp-tsys01 {
    compatible = "te,tsys01";
    bus = <&i2c1>;
    address = <0x77>;
    conversion-delay-ms = <10>;
    reset-delay-ms = <3>;
    validate-prom-checksum;
    status = "okay";
};
```

```c
int32_t temperature_mc;
yi_tsys01_read(temp, &temperature_mc);
```
