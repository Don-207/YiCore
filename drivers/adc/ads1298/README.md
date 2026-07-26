# ADS1298 driver

This driver supports a single ADS1298 on a standard (non-daisy-chain) SPI bus.
It uses SPI mode 1 and parses each conversion as a 24-bit status word followed
by eight signed 24-bit channel samples.

The SPI clock must not exceed twice `master_clock_hz`. This limit ensures that
an uninterrupted eight-bit SPI transfer satisfies the ADS1298 `tSDECODE`
requirement of four master-clock periods. A typical 2.048 MHz ADS1298 clock
therefore permits an SPI clock up to 4.096 MHz for command transfers.

Typical use:

```c
yi_ads1298_frame_t frame;
bool ready;

/* The YiCore device manager calls yi_ads1298_init() first. */
yi_ads1298_start(ads1298);
yi_ads1298_set_continuous(ads1298, true);

if((yi_ads1298_data_ready(ads1298, &ready) == 0) && ready) {
    yi_ads1298_read_frame(ads1298, &frame);
}
```

Register access is only allowed while continuous mode is disabled. To
reconfigure a running recorder, issue SDATAC with
`yi_ads1298_set_continuous(dev, false)`, change registers, and then enable
continuous mode again.

`frame.channel[]` contains sign-extended ADC codes. Convert a code to input
voltage using the actual reference voltage and configured PGA gain:

```
input_uV = code * (VREF_uV / gain) / 8388607
```

The optional RESET and START GPIO devices must be configured as outputs. DRDY
must be configured as an input and is active low. If START is omitted, the
driver uses START/STOP SPI commands. Hardware RESET is preferred, but the
driver can use the RESET opcode when the GPIO is omitted.
