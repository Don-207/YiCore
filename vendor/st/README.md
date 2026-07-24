# ST vendor dependencies

This directory contains upstream files imported with the STM32CubeMX-generated
STM32F103 example:

- `cmsis/`: ARM CMSIS plus the ST STM32F1 CMSIS device package
- `stm32cube/stm32f1xx_hal_driver/`: STM32F1 HAL and LL driver package

The files were moved without source changes from
`examples/stm32f103-dts-demo/Drivers/`. Their original copyright headers and
license files remain authoritative.

## Update policy

1. Import CMSIS and HAL from one matching STM32CubeF1 release.
2. Keep vendor files unmodified.
3. Record the STM32CubeF1 version and source in this file when upgrading.
4. Put required compatibility changes in `soc/st/stm32/`, not here.
5. Build and run the board-level peripheral regression tests after an update.

The exact STM32CubeF1 package version of the original generated example was not
recorded. Determine it from the original CubeMX package metadata before the
first vendor-library upgrade.
