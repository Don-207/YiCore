# W25Q64 driver

8 MiB SPI NOR flash driver with JEDEC-ID verification, arbitrary reads,
page-split programming, 4 KiB sector erase, and busy polling.

```dts
flash0: flash-w25q64 {
    compatible = "winbond,w25q64";
    bus = <&spi1>;
    cs-gpio = <&flash_cs_gpio>;
    spi-frequency = <8000000>;
    spi-mode = <0>;
    program-timeout-ms = <100>;
    erase-timeout-ms = <5000>;
    status = "okay";
};
```

Erase offsets and lengths must be 4096-byte aligned. Programming does not
change zero bits back to one; erase the affected sector first when required.
