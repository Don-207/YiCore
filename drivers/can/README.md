# CAN interface

`yi_can.h` defines YiCore CAN frame, timing, filter, transmit, receive, and
state interfaces. Hardware-specific controller initialization and interrupt
handling belong to the selected SoC backend. Configure bitrate and pins in
DeviceTree and use only devices reported ready by the device manager.
