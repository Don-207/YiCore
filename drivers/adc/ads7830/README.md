# ADS7830 driver

Eight-channel 8-bit I2C ADC driver integrated with `yi_adc_read()` and
`yi_adc_channel_read()`. It supports internal/external reference selection,
channel remapping required by the ADS7830 command format, and configurable
transfer timeout.

```dts
adc0: adc-ads7830 {
    compatible = "ti,ads7830";
    bus = <&i2c1>;
    address = <0x48>;
    default-channel = <0>;
    internal-reference;
    reference-mv = <2500>;
    transfer-timeout-ms = <20>;
    status = "okay";
};
```

```c
uint16_t code;
yi_adc_channel_read(adc, 3U, &code, 20U);
```
