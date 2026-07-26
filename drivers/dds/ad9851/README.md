# AD9851 DDS driver

The driver uses the AD9851 four-wire serial interface and sends each 40-bit
update LSB first. Connect `W_CLK`, `FQ_UD`, `DATA` and `RESET` to four GPIO
outputs.

DeviceTree example for a common 30 MHz module using the internal 6x PLL:

```dts
dds1: dds-ad9851 {
    compatible = "adi,ad9851";
    w-clk-gpio = <&ad9851_w_clk_gpio>;
    fq-ud-gpio = <&ad9851_fq_ud_gpio>;
    data-gpio = <&ad9851_data_gpio>;
    reset-gpio = <&ad9851_reset_gpio>;
    reference-clock-frequency = <30000000>;
    clock-multiplier;
    pulse-delay-us = <1>;
    init-level = "application";
    init-priority = <20>;
    status = "okay";
};
```

Basic use:

```c
yi_device_t *dds = yi_device_get("dds1");

yi_ad9851_set_frequency(dds, 10000000U);
yi_ad9851_set_phase(dds, 900U); /* 90.0 degrees */
yi_ad9851_set_power_down(dds, false);
```

Use `yi_ad9851_set_frequency_phase()` when frequency and phase must take
effect on the same `FQ_UD` edge. Phase is expressed in tenths of a degree and
quantized to the chip's 5-bit (11.25 degree) resolution.

The configured reference clock is the external oscillator frequency before
the optional 6x PLL. With `clock-multiplier`, it must not exceed 30 MHz. Set
`pulse-delay-us` to zero when GPIO call overhead already satisfies the device
timing and faster updates are required.
