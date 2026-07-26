# Clock interface

The clock driver represents peripheral clock gates and buses used as explicit
DeviceTree dependencies. Peripheral drivers enable their clock before touching
registers, which keeps initialization ordering deterministic. Clock IDs and
enable masks are SoC-specific and generated from the board description.
