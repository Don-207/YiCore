# Arm architecture reservation

This directory is reserved for Cortex-M behavior shared by ST, GigaDevice and
other Arm MCU vendors. CMSIS-Core may be used internally, while device headers
and peripheral register definitions remain in each vendor SoC backend.

Code should move here only after at least two SoC backends share the same
tested behavior; do not relocate working STM32 code merely to populate the
directory.
