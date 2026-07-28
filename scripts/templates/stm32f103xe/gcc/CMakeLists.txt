# File: CMakeLists.txt
# Function: Build YiProduct application, bootloader, or test with one GCC entry.
# Author: Don
# Date: 2026-07-27
# Version: 1.0.0

cmake_minimum_required(VERSION 3.20)
project(YiProduct LANGUAGES C ASM)

set(YI_PRODUCT_IMAGE "application" CACHE STRING
    "Firmware image to build: application, bootloader, or test")
set_property(CACHE YI_PRODUCT_IMAGE PROPERTY STRINGS application bootloader test)

set(_YI_PRODUCT_IMAGES application bootloader test)
if(NOT YI_PRODUCT_IMAGE IN_LIST _YI_PRODUCT_IMAGES)
    message(FATAL_ERROR "YI_PRODUCT_IMAGE must be application, bootloader, or test")
endif()

get_filename_component(YI_PRODUCT_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
set(YICORE_ROOT "${YI_PRODUCT_ROOT}/YiCore")
set(COMMON_ROOT "${YI_PRODUCT_ROOT}/firmware/common")
set(IMAGE_ROOT "${YI_PRODUCT_ROOT}/firmware/images/${YI_PRODUCT_IMAGE}")
set(GENERATED_ROOT "${IMAGE_ROOT}/generated")

find_package(Python3 REQUIRED COMPONENTS Interpreter)
file(MAKE_DIRECTORY "${GENERATED_ROOT}")
execute_process(
    COMMAND "${Python3_EXECUTABLE}" "${YICORE_ROOT}/scripts/yi_dts_gen.py"
            --dts "${IMAGE_ROOT}/app.dts"
            --bindings "${YICORE_ROOT}/dts/bindings"
            --output "${GENERATED_ROOT}"
    COMMAND_ERROR_IS_FATAL ANY
)
execute_process(
    COMMAND "${Python3_EXECUTABLE}" "${YICORE_ROOT}/scripts/yi_build_info_gen.py"
            --image "${YI_PRODUCT_IMAGE}"
            --version-file "${IMAGE_ROOT}/VERSION"
            --output "${GENERATED_ROOT}/yi_build_info.c"
    COMMAND_ERROR_IS_FATAL ANY
)

set(YI_PRODUCT_SOURCES
    "${IMAGE_ROOT}/main.c"
    "${COMMON_ROOT}/Core/Src/gpio.c"
    "${COMMON_ROOT}/Core/Src/stm32f1xx_it.c"
    "${COMMON_ROOT}/Core/Src/stm32f1xx_hal_msp.c"
    "${COMMON_ROOT}/Core/Src/system_stm32f1xx.c"
    "${YICORE_ROOT}/vendor/st/cmsis/Device/ST/STM32F1xx/Source/Templates/gcc/startup_stm32f103xe.s"
    "${YICORE_ROOT}/vendor/st/stm32cube/stm32f1xx_hal_driver/Src/stm32f1xx_hal.c"
    "${YICORE_ROOT}/vendor/st/stm32cube/stm32f1xx_hal_driver/Src/stm32f1xx_hal_can.c"
    "${YICORE_ROOT}/vendor/st/stm32cube/stm32f1xx_hal_driver/Src/stm32f1xx_hal_cortex.c"
    "${YICORE_ROOT}/vendor/st/stm32cube/stm32f1xx_hal_driver/Src/stm32f1xx_hal_dma.c"
    "${YICORE_ROOT}/vendor/st/stm32cube/stm32f1xx_hal_driver/Src/stm32f1xx_hal_exti.c"
    "${YICORE_ROOT}/vendor/st/stm32cube/stm32f1xx_hal_driver/Src/stm32f1xx_hal_flash.c"
    "${YICORE_ROOT}/vendor/st/stm32cube/stm32f1xx_hal_driver/Src/stm32f1xx_hal_flash_ex.c"
    "${YICORE_ROOT}/vendor/st/stm32cube/stm32f1xx_hal_driver/Src/stm32f1xx_hal_gpio.c"
    "${YICORE_ROOT}/vendor/st/stm32cube/stm32f1xx_hal_driver/Src/stm32f1xx_hal_gpio_ex.c"
    "${YICORE_ROOT}/vendor/st/stm32cube/stm32f1xx_hal_driver/Src/stm32f1xx_hal_i2c.c"
    "${YICORE_ROOT}/vendor/st/stm32cube/stm32f1xx_hal_driver/Src/stm32f1xx_hal_pwr.c"
    "${YICORE_ROOT}/vendor/st/stm32cube/stm32f1xx_hal_driver/Src/stm32f1xx_hal_rcc.c"
    "${YICORE_ROOT}/vendor/st/stm32cube/stm32f1xx_hal_driver/Src/stm32f1xx_hal_rcc_ex.c"
    "${YICORE_ROOT}/vendor/st/stm32cube/stm32f1xx_hal_driver/Src/stm32f1xx_hal_spi.c"
    "${YICORE_ROOT}/vendor/st/stm32cube/stm32f1xx_hal_driver/Src/stm32f1xx_hal_tim.c"
    "${YICORE_ROOT}/vendor/st/stm32cube/stm32f1xx_hal_driver/Src/stm32f1xx_hal_tim_ex.c"
    "${YICORE_ROOT}/vendor/st/stm32cube/stm32f1xx_hal_driver/Src/stm32f1xx_hal_uart.c"
    "${YICORE_ROOT}/core/yi_device.c"
    "${YICORE_ROOT}/core/yi_build_info.c"
    "${YICORE_ROOT}/drivers/led/yi_led.c"
    "${YICORE_ROOT}/drivers/uart/yi_uart_dma_lwrb.c"
    "${YICORE_ROOT}/drivers/flash/yi_flash.c"
    "${YICORE_ROOT}/drivers/eeprom/yi_eeprom.c"
    "${YICORE_ROOT}/drivers/eeprom/at24c02/yi_at24c02.c"
    "${YICORE_ROOT}/drivers/spi/yi_spi.c"
    "${YICORE_ROOT}/drivers/spi/yi_soft_spi.c"
    "${YICORE_ROOT}/drivers/i2c/yi_i2c.c"
    "${YICORE_ROOT}/drivers/i2c/yi_soft_i2c.c"
    "${YICORE_ROOT}/drivers/adc/yi_adc.c"
    "${YICORE_ROOT}/drivers/adc/ads1298/yi_ads1298.c"
    "${YICORE_ROOT}/soc/st/stm32/stm32f1/yi_uart_stm32.c"
    "${YICORE_ROOT}/soc/st/stm32/stm32f1/yi_gpio_stm32f1.c"
    "${YICORE_ROOT}/soc/st/stm32/stm32f1/yi_pinmux_stm32f1.c"
    "${YICORE_ROOT}/soc/st/stm32/stm32f1/yi_clock_stm32f1.c"
    "${YICORE_ROOT}/soc/st/stm32/stm32f1/yi_flash_stm32f1.c"
    "${YICORE_ROOT}/soc/st/stm32/stm32f1/yi_timer_stm32f1.c"
    "${YICORE_ROOT}/soc/st/stm32/stm32f1/yi_can_stm32f1.c"
    "${YICORE_ROOT}/soc/st/stm32/stm32f1/yi_stm32_periph.c"
    "${YICORE_ROOT}/soc/st/stm32/stm32f1/yi_stm32_system.c"
    "${YICORE_ROOT}/soc/st/stm32/stm32f1/yi_i2c_stm32.c"
    "${YICORE_ROOT}/soc/st/stm32/stm32f1/yi_spi_stm32.c"
    "${YICORE_ROOT}/soc/st/stm32/stm32f1/yi_adc_stm32f1.c"
    "${YICORE_ROOT}/subsys/console/yi_console.c"
    "${YICORE_ROOT}/subsys/log/yi_log.c"
    "${YICORE_ROOT}/subsys/timer/yi_soft_timer.c"
    "${YICORE_ROOT}/ports/rtt/yi_rtt.c"
    "${YICORE_ROOT}/third_party/RTT/SEGGER_RTT.c"
    "${YICORE_ROOT}/third_party/lwrb/lwrb/src/lwrb/lwrb.c"
    "${GENERATED_ROOT}/yi_generated.c"
    "${GENERATED_ROOT}/yi_build_info.c"
)

add_executable("yi-product-${YI_PRODUCT_IMAGE}.elf" ${YI_PRODUCT_SOURCES})
target_compile_definitions("yi-product-${YI_PRODUCT_IMAGE}.elf" PRIVATE
    USE_HAL_DRIVER STM32F103xE LWRB_DISABLE_ATOMIC)
target_include_directories("yi-product-${YI_PRODUCT_IMAGE}.elf" PRIVATE
    "${COMMON_ROOT}/Core/Inc"
    "${IMAGE_ROOT}/App"
    "${GENERATED_ROOT}"
    "${YICORE_ROOT}/vendor/st/stm32cube/stm32f1xx_hal_driver/Inc"
    "${YICORE_ROOT}/vendor/st/stm32cube/stm32f1xx_hal_driver/Inc/Legacy"
    "${YICORE_ROOT}/vendor/st/cmsis/Device/ST/STM32F1xx/Include"
    "${YICORE_ROOT}/vendor/st/cmsis/Include"
    "${YICORE_ROOT}/core"
    "${YICORE_ROOT}/drivers/led"
    "${YICORE_ROOT}/drivers/uart"
    "${YICORE_ROOT}/drivers/gpio"
    "${YICORE_ROOT}/drivers/pinmux"
    "${YICORE_ROOT}/drivers/clock"
    "${YICORE_ROOT}/drivers/flash"
    "${YICORE_ROOT}/drivers/eeprom"
    "${YICORE_ROOT}/drivers/eeprom/at24c02"
    "${YICORE_ROOT}/drivers/timer"
    "${YICORE_ROOT}/drivers/spi"
    "${YICORE_ROOT}/drivers/i2c"
    "${YICORE_ROOT}/drivers/can"
    "${YICORE_ROOT}/drivers/adc"
    "${YICORE_ROOT}/drivers/adc/ads1298"
    "${YICORE_ROOT}/subsys/console"
    "${YICORE_ROOT}/subsys/log"
    "${YICORE_ROOT}/subsys/timer"
    "${YICORE_ROOT}/ports/rtt"
    "${YICORE_ROOT}/soc/st/stm32/stm32f1"
    "${YICORE_ROOT}/third_party/RTT"
    "${YICORE_ROOT}/third_party/lwrb/lwrb/src/include"
)

target_compile_options("yi-product-${YI_PRODUCT_IMAGE}.elf" PRIVATE
    -mcpu=cortex-m3 -mthumb -mfloat-abi=soft
    -ffunction-sections -fdata-sections -fno-common
    -Wall -Wextra)
target_link_options("yi-product-${YI_PRODUCT_IMAGE}.elf" PRIVATE
    -mcpu=cortex-m3 -mthumb -mfloat-abi=soft
    -T"${YI_PRODUCT_ROOT}/firmware/linker/gcc/stm32f103xe-standalone.ld"
    -Wl,--gc-sections
    -Wl,-Map="${CMAKE_BINARY_DIR}/yi-product-${YI_PRODUCT_IMAGE}.map"
    --specs=nosys.specs)

add_custom_command(TARGET "yi-product-${YI_PRODUCT_IMAGE}.elf" POST_BUILD
    COMMAND "${CMAKE_OBJCOPY}" -O ihex
            "$<TARGET_FILE:yi-product-${YI_PRODUCT_IMAGE}.elf>"
            "${CMAKE_BINARY_DIR}/yi-product-${YI_PRODUCT_IMAGE}.hex"
    COMMAND "${CMAKE_OBJCOPY}" -O binary
            "$<TARGET_FILE:yi-product-${YI_PRODUCT_IMAGE}.elf>"
            "${CMAKE_BINARY_DIR}/yi-product-${YI_PRODUCT_IMAGE}.bin"
    COMMAND "${CMAKE_SIZE}" "$<TARGET_FILE:yi-product-${YI_PRODUCT_IMAGE}.elf>"
)
