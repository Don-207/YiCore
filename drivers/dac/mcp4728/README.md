# MCP4728 driver

This driver supports all four MCP4728 12-bit DAC channels, per-channel
reference and gain settings, power-down resistors, fast simultaneous updates
and explicit EEPROM writes.

DeviceTree example:

```dts
dac1: dac-mcp4728 {
    compatible = "microchip,mcp4728";
    bus = <&i2c1>;
    address = <0x60>;
    default-channel = <0>;
    vdd-mv = <3300>;
    channel0-value = <0>;
    channel0-internal-reference;
    channel0-gain-2x;
    channel1-value = <1024>;
    channel2-value = <2048>;
    channel3-value = <4095>;
    transfer-timeout-ms = <20>;
    eeprom-timeout-ms = <100>;
    status = "okay";
};
```

`yi_dac_write()` writes the configured default channel. Channel-specific and
simultaneous updates are also available:

```c
uint16_t values[4] = {0U, 1024U, 2048U, 4095U};

yi_dac_write(dac, 2048U);
yi_mcp4728_write_channel(dac, 2U, 3000U);
yi_mcp4728_write_channel_mv(dac, 0U, 2500U);
yi_mcp4728_write_all(dac, values);
```

EEPROM is only changed by `yi_mcp4728_write_eeprom()`. Its endurance is
finite, so normal waveform or control updates should use the volatile APIs.
