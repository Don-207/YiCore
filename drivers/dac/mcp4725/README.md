# MCP4725 driver

This driver supports MCP4725 12-bit DAC fast writes, voltage conversion,
power-down resistor selection, readback and explicit EEPROM writes. Normal
`yi_dac_write()` calls do not write EEPROM.

DeviceTree example:

```dts
dac0: dac-mcp4725 {
    compatible = "microchip,mcp4725";
    bus = <&i2c1>;
    address = <0x60>;
    reference-mv = <3300>;
    default-value = <0>;
    transfer-timeout-ms = <20>;
    eeprom-timeout-ms = <100>;
    init-level = "post-kernel";
    init-priority = <30>;
    status = "okay";
};
```

Basic output control:

```c
yi_device_t *dac = yi_device_get("dac0");

yi_dac_write(dac, 2048U);          /* raw 12-bit code */
yi_mcp4725_write_mv(dac, 1650U);   /* uses reference-mv */
```

Power-down modes select the internal resistor connected to the output:

```c
yi_mcp4725_set_power(dac, YI_MCP4725_POWER_100K);
yi_mcp4725_set_power(dac, YI_MCP4725_POWER_NORMAL);
```

Persist a power-up value only when needed:

```c
yi_mcp4725_write_eeprom(dac, 2048U, YI_MCP4725_POWER_NORMAL);
```

EEPROM endurance is finite. The persistent API polls the device RDY flag and
returns an error when `eeprom-timeout-ms` expires.
