# DDS drivers

Direct digital synthesis drivers provide frequency, phase, waveform, reset,
and power controls. AD9834 uses the common SPI interface; AD9851 uses its
dedicated GPIO serial protocol. Frequency accuracy depends on the configured
master/reference clock, so the DeviceTree value must match the actual board.
