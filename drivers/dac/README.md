# DAC drivers

`yi_dac.h` provides `yi_dac_write()` for raw output codes. Device drivers may
add channel selection, voltage conversion, reference/gain control, synchronous
updates, or persistent storage.

Implemented devices include MCP4725, MCP4728, and GP8210S. Prefer volatile
write APIs for frequent updates; EEPROM persistence has finite endurance.
