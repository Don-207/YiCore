# Timer interface

The timer interface provides counter configuration and backend operations for
MCU hardware timers. DeviceTree describes counter width, tick frequency,
clock dependency, interrupt, and priority. Applications should use the
configured tick frequency for conversions instead of assuming the peripheral
input clock.
