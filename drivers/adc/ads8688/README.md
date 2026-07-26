# ADS8688 driver

The driver supports eight 16-bit channels, manual channel reads, automatic
sequencing, channel power-down, per-channel input ranges, standby and global
power-down. It uses SPI mode 1 and 32-clock command frames.

DeviceTree range codes follow the ADS8688 program register encoding:

- `0`: +/-10.24 V
- `1`: +/-5.12 V
- `2`: +/-2.56 V
- `5`: 0 to 10.24 V
- `6`: 0 to 5.12 V

Example:

```dts
adc2: adc-ads8688 {
    compatible = "ti,ads8688";
    bus = <&spi1>;
    cs-gpio = <&ads8688_cs_gpio>;
    spi-frequency = <1000000>;
    transfer-timeout-ms = <20>;
    default-channel = <0>;
    auto-sequence-mask = <0xff>;
    power-down-mask = <0x00>;
    channel0-range = <0>;
    channel1-range = <0>;
    channel2-range = <1>;
    channel3-range = <1>;
    channel4-range = <5>;
    channel5-range = <5>;
    channel6-range = <6>;
    channel7-range = <6>;
    status = "okay";
};
```

Manual reads use the common ADC API:

```c
uint16_t code;
yi_adc_channel_read(adc, 3U, &code, 20U);
```

The ADS8688 conversion interface is pipelined. The driver sends the manual
channel command and a following NOP frame before returning the result.

Automatic sequence use:

```c
yi_ads8688_start_auto(adc);
yi_ads8688_read_auto(adc, &code);
yi_ads8688_stop_auto(adc);
```
