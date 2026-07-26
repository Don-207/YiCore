# AD9834 DDS driver

The driver uses the YiCore SPI API in mode 2 and writes 16-bit AD9834 words
MSB first. Connect the device as follows:

- `FSYNC` to the configured `cs-gpio`
- `SCLK` and `SDATA` to SPI SCK and MOSI
- an external master clock to `MCLK`

DeviceTree example:

```dts
dds0: dds-ad9834 {
    compatible = "adi,ad9834";
    bus = <&spi1>;
    cs-gpio = <&ad9834_fsync_gpio>;
    spi-frequency = <1000000>;
    mclk-frequency = <75000000>;
    transfer-timeout-ms = <10>;
    init-level = "application";
    init-priority = <20>;
    status = "okay";
};
```

The device remains reset after initialization. Configure it and then enable
the output:

```c
yi_device_t *dds = yi_device_get("dds0");

yi_ad9834_set_frequency(dds, 0U, 1000000U);
yi_ad9834_set_phase(dds, 0U, 0U);
yi_ad9834_set_waveform(dds, YI_AD9834_WAVE_SINE);
yi_ad9834_select(dds, 0U, 0U);
yi_ad9834_set_reset(dds, false);
```

Phase is expressed in tenths of a degree (`0` through `3599`). Output
frequency is limited to half of the configured MCLK frequency.
