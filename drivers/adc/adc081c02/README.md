# ADC081C02 driver

This driver supports the ADC081C02 family through the YiCore I2C and ADC
interfaces. It reads the 8-bit conversion value from the device's 16-bit,
big-endian conversion register and supports its alert limit registers.

DeviceTree example:

```dts
adc0: adc081c02 {
    compatible = "ti,adc081c02";
    bus = <&i2c1>;
    address = <0x50>;
    reference-mv = <3300>;
    configuration = <0x20>;
    low-limit = <10>;
    high-limit = <240>;
    hysteresis = <4>;
    transfer-timeout-ms = <20>;
    init-level = "post-kernel";
    init-priority = <30>;
    status = "okay";
};
```

Use the address selected by the device's address pin/resistor arrangement.
`configuration` is written directly to the ADC configuration register, which
allows the conversion cycle and alert behavior to be selected according to
the datasheet.

Basic use:

```c
yi_device_t *adc = yi_device_get("adc0");
uint16_t raw;
uint16_t millivolts;

yi_adc_read(adc, &raw, 20U);
yi_adc081c02_read_mv(adc, &millivolts);
```

Runtime alert control:

```c
uint8_t status;

yi_adc081c02_set_limits(adc, 10U, 240U, 4U);
yi_adc081c02_set_configuration(adc, 0x20U);
yi_adc081c02_get_alert_status(adc, &status);
```

The voltage conversion uses `raw * reference-mv / 255`. Threshold values are
specified as normal 8-bit ADC codes; the driver handles register alignment.
