# AT24C02 driver

256-byte I2C EEPROM driver with 8-byte page handling, bounds checking, write
completion polling, and the common YiCore EEPROM API.

```dts
eeprom0: eeprom-at24c02 {
    compatible = "atmel,24c02";
    bus = <&i2c1>;
    address = <0x50>;
    transfer-timeout-ms = <20>;
    write-timeout-ms = <20>;
    status = "okay";
};
```

Writes crossing page boundaries are split automatically. Do not exceed the
device's specified EEPROM endurance for frequently updated data.
