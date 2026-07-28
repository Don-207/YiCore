# GigaDevice SoC backends

This directory contains YiCore-owned GigaDevice integration code. Official
device headers, startup files and standard peripheral libraries belong below
`vendor/gigadevice/`.

Each supported family gets an isolated backend directory. Public YiCore driver
headers must not expose GD32 SDK types.
