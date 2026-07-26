# EEPROM drivers

The common EEPROM API provides bounded byte reads and writes using device
geometry supplied by each backend. Callers should account for finite write
endurance and avoid rewriting unchanged data.

The current external EEPROM implementation is AT24C02; its subdirectory
documents page splitting and write-completion polling.
