# GP8210S driver

The driver provides two 15-bit voltage-output channels over I2C. It supports
0-5 V and 0-10 V ranges, raw codes, millivolt conversion and one-transaction
dual-channel updates. The default I2C address is `0x58`.

```dts
dac2: dac-gp8210s {
    compatible = "guestgood,gp8210s";
    bus = <&i2c1>;
    address = <0x58>;
    default-channel = <0>;
    output-range-mv = <10000>;
    channel0-value = <0>;
    channel1-value = <0>;
    transfer-timeout-ms = <20>;
    status = "okay";
};
```

```c
yi_dac_write(dac, 16384U);
yi_gp8210s_write_channel_mv(dac, 1U, 7500U);
yi_gp8210s_write_all(dac, 1000U, 2000U);
```

The chip's nonvolatile store sequence uses a non-standard partial-address I2C
waveform that the YiCore I2C API cannot represent. This driver deliberately
does not expose persistence; all output APIs are volatile and safe for
frequent updates.
