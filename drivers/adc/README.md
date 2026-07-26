# ADC drivers

`yi_adc.h` defines the common raw-sample interface. `yi_adc_read()` reads a
device's default channel and `yi_adc_channel_read()` selects an explicit
channel. Values are raw converter codes; voltage scaling remains device
specific because resolution, reference, gain, and bipolar encoding differ.

Implemented devices include ADC081C02, ADS1258, ADS1298, ADS7830, ADS8688,
and the STM32 ADC backend. See each device subdirectory for configuration and
conversion details.
