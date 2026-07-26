# ADS1258 driver

This driver supports ADS1258 register access, channel scanning, pulse
conversion and 24-bit signed conversion data. SPI mode 1 is used and the
driver enables the status byte so each sample includes channel and condition
flags.

DeviceTree example:

```dts
adc1: adc-ads1258 {
    compatible = "ti,ads1258";
    bus = <&spi1>;
    cs-gpio = <&ads1258_cs_gpio>;
    start-gpio = <&ads1258_start_gpio>;
    reset-gpio = <&ads1258_reset_gpio>;
    drdy-gpio = <&ads1258_drdy_gpio>;
    spi-frequency = <1000000>;
    transfer-timeout-ms = <20>;
    config0 = <0x00>;
    config1 = <0x00>;
    muxsch = <0x00>;
    muxdif = <0x00>;
    single-ended-mask = <0xffff>;
    system-readings = <0x00>;
    init-level = "post-kernel";
    init-priority = <30>;
    status = "okay";
};
```

`config0`, `config1`, `muxsch`, `muxdif`, and `system-readings` correspond to
the ADS1258 register values. The driver forces the CONFIG0 STAT bit on because
channel identification is required to interpret automatic-scan results.

Basic automatic-scan use:

```c
yi_ads1258_sample_t sample;
bool ready;

yi_ads1258_start(adc);
if((yi_ads1258_data_ready(adc, &ready) == 0) && ready) {
    if(yi_ads1258_read_sample(adc, &sample) == 0 && sample.new_data) {
        /* sample.channel and sign-extended sample.value are valid. */
    }
}
```

For pulse conversion, keep START low and call `yi_ads1258_pulse_convert()`.
RESET and DRDY are optional; START is required. When RESET is omitted the
driver uses the RESET command. The SPI frequency is limited to 2 MHz by the
binding and runtime validation for conservative command timing.
