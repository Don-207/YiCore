# File: st-stm32f1.cmake
# Function: Build thin YiCore applications for STM32F103xE boards.
# Author: Don
# Date: 2026-07-28
# Version: 1.0.0

include(CMakeParseArguments)

# Build one STM32F1 application from app sources and the selected board.
function(yi_platform_application)
    set(_options)
    set(_one_value NAME)
    set(_multi_value SOURCES)
    cmake_parse_arguments(
        YI_PLATFORM
        "${_options}"
        "${_one_value}"
        "${_multi_value}"
        ${ARGN}
    )
    if(NOT YI_PLATFORM_NAME OR NOT YI_PLATFORM_SOURCES)
        message(FATAL_ERROR "STM32F1 adapter requires NAME and SOURCES")
    endif()

    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    set(_generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
    set(_resolved_dts "${_generated_dir}/app.dts")
    file(MAKE_DIRECTORY "${_generated_dir}")
    file(TO_CMAKE_PATH "${YI_APP_BOARD_DIR}/board.dts" _board_dts)
    file(TO_CMAKE_PATH "${YI_APP_OVERLAY}" _app_overlay)
    file(
        WRITE "${_resolved_dts}"
        "/include/ \"${_board_dts}\"\n"
        "/include/ \"${_app_overlay}\"\n"
    )
    execute_process(
        COMMAND
            "${Python3_EXECUTABLE}"
            "${YICORE_ROOT}/scripts/yi_dts_gen.py"
            --dts "${_resolved_dts}"
            --bindings "${YICORE_ROOT}/dts/bindings"
            --output "${_generated_dir}"
        COMMAND_ERROR_IS_FATAL ANY
    )
    execute_process(
        COMMAND
            "${Python3_EXECUTABLE}"
            "${YICORE_ROOT}/scripts/yi_build_info_gen.py"
            --image application
            --version-file "${YI_APP_VERSION_FILE}"
            --output "${_generated_dir}/yi_build_info.c"
        COMMAND_ERROR_IS_FATAL ANY
    )

    set(
        YI_HAL_ST_ROOT
        "${YICORE_ROOT}/vendor/st"
        CACHE PATH
        "Root of the ST vendor module"
    )
    set(
        YI_CMSIS_ROOT
        "${YI_HAL_ST_ROOT}/cmsis"
        CACHE PATH
        "Root of CMSIS Core headers"
    )
    set(_hal_root
        "${YI_HAL_ST_ROOT}/stm32cube/stm32f1xx_hal_driver")
    set(_device_root
        "${YI_HAL_ST_ROOT}/cmsis/Device/ST/STM32F1xx")
    set(_soc_root "${YICORE_ROOT}/soc/st/stm32/stm32f1")
    set(_target "${YI_PLATFORM_NAME}.elf")
    set(_sources
        ${YI_PLATFORM_SOURCES}
        "${_device_root}/Source/Templates/system_stm32f1xx.c"
        "${_device_root}/Source/Templates/gcc/startup_stm32f103xe.s"
        "${_hal_root}/Src/stm32f1xx_hal.c"
        "${_hal_root}/Src/stm32f1xx_hal_adc.c"
        "${_hal_root}/Src/stm32f1xx_hal_adc_ex.c"
        "${_hal_root}/Src/stm32f1xx_hal_can.c"
        "${_hal_root}/Src/stm32f1xx_hal_cortex.c"
        "${_hal_root}/Src/stm32f1xx_hal_dma.c"
        "${_hal_root}/Src/stm32f1xx_hal_exti.c"
        "${_hal_root}/Src/stm32f1xx_hal_flash.c"
        "${_hal_root}/Src/stm32f1xx_hal_flash_ex.c"
        "${_hal_root}/Src/stm32f1xx_hal_gpio.c"
        "${_hal_root}/Src/stm32f1xx_hal_gpio_ex.c"
        "${_hal_root}/Src/stm32f1xx_hal_i2c.c"
        "${_hal_root}/Src/stm32f1xx_hal_pwr.c"
        "${_hal_root}/Src/stm32f1xx_hal_rcc.c"
        "${_hal_root}/Src/stm32f1xx_hal_rcc_ex.c"
        "${_hal_root}/Src/stm32f1xx_hal_spi.c"
        "${_hal_root}/Src/stm32f1xx_hal_tim.c"
        "${_hal_root}/Src/stm32f1xx_hal_tim_ex.c"
        "${_hal_root}/Src/stm32f1xx_hal_uart.c"
        "${YICORE_ROOT}/core/yi_device.c"
        "${YICORE_ROOT}/core/yi_build_info.c"
        "${YICORE_ROOT}/drivers/adc/yi_adc.c"
        "${YICORE_ROOT}/drivers/adc/ads7830/yi_ads7830.c"
        "${YICORE_ROOT}/drivers/eeprom/yi_eeprom.c"
        "${YICORE_ROOT}/drivers/eeprom/at24c02/yi_at24c02.c"
        "${YICORE_ROOT}/drivers/flash/yi_flash.c"
        "${YICORE_ROOT}/drivers/i2c/yi_i2c.c"
        "${YICORE_ROOT}/drivers/i2c/yi_soft_i2c.c"
        "${YICORE_ROOT}/drivers/led/yi_led.c"
        "${YICORE_ROOT}/drivers/spi/yi_spi.c"
        "${YICORE_ROOT}/drivers/spi/yi_soft_spi.c"
        "${YICORE_ROOT}/drivers/uart/yi_uart_dma_lwrb.c"
        "${_soc_root}/yi_adc_stm32f1.c"
        "${_soc_root}/yi_can_stm32f1.c"
        "${_soc_root}/yi_clock_stm32f1.c"
        "${_soc_root}/yi_flash_stm32f1.c"
        "${_soc_root}/yi_gpio_stm32f1.c"
        "${_soc_root}/yi_i2c_stm32.c"
        "${_soc_root}/yi_pinmux_stm32f1.c"
        "${_soc_root}/yi_spi_stm32.c"
        "${_soc_root}/yi_stm32_periph.c"
        "${_soc_root}/yi_stm32_system.c"
        "${_soc_root}/yi_stm32f1_runtime.c"
        "${_soc_root}/yi_timer_stm32f1.c"
        "${_soc_root}/yi_uart_stm32.c"
        "${YICORE_ROOT}/subsys/console/yi_console.c"
        "${YICORE_ROOT}/subsys/log/yi_log.c"
        "${YICORE_ROOT}/subsys/timer/yi_soft_timer.c"
        "${YICORE_ROOT}/ports/rtt/yi_rtt.c"
        "${YICORE_ROOT}/ports/newlib/yi_newlib_syscalls.c"
        "${YICORE_ROOT}/third_party/RTT/SEGGER_RTT.c"
        "${YICORE_ROOT}/third_party/lwrb/lwrb/src/lwrb/lwrb.c"
        "${_generated_dir}/yi_generated.c"
        "${_generated_dir}/yi_build_info.c"
    )
    add_executable("${_target}" ${_sources})
    target_compile_definitions(
        "${_target}" PRIVATE
        USE_HAL_DRIVER STM32F103xE LWRB_DISABLE_ATOMIC
    )

    set(_include_dirs
        "${_generated_dir}"
        "${_soc_root}"
        "${_hal_root}/Inc"
        "${_hal_root}/Inc/Legacy"
        "${_device_root}/Include"
        "${YI_CMSIS_ROOT}/Include"
        "${YICORE_ROOT}/core"
        "${YICORE_ROOT}/drivers/adc"
        "${YICORE_ROOT}/drivers/adc/ads7830"
        "${YICORE_ROOT}/drivers/can"
        "${YICORE_ROOT}/drivers/clock"
        "${YICORE_ROOT}/drivers/eeprom"
        "${YICORE_ROOT}/drivers/eeprom/at24c02"
        "${YICORE_ROOT}/drivers/flash"
        "${YICORE_ROOT}/drivers/gpio"
        "${YICORE_ROOT}/drivers/i2c"
        "${YICORE_ROOT}/drivers/led"
        "${YICORE_ROOT}/drivers/pinmux"
        "${YICORE_ROOT}/drivers/spi"
        "${YICORE_ROOT}/drivers/timer"
        "${YICORE_ROOT}/drivers/uart"
        "${YICORE_ROOT}/subsys/console"
        "${YICORE_ROOT}/subsys/log"
        "${YICORE_ROOT}/subsys/timer"
        "${YICORE_ROOT}/ports/rtt"
        "${YICORE_ROOT}/ports/newlib"
        "${YICORE_ROOT}/third_party/RTT"
        "${YICORE_ROOT}/third_party/lwrb/lwrb/src/include"
    )
    target_include_directories("${_target}" PRIVATE ${_include_dirs})
    target_compile_options(
        "${_target}" PRIVATE
        -mcpu=cortex-m3 -mthumb -mfloat-abi=soft
        -Os -ffunction-sections -fdata-sections -fno-common
        -Wall -Wextra
    )

    set(
        _linker_script
        "${YICORE_ROOT}/scripts/templates/stm32f103xe/linker/stm32f103xe-standalone.ld"
    )
    set_property(
        TARGET "${_target}" PROPERTY LINK_DEPENDS "${_linker_script}"
    )
    target_link_options(
        "${_target}" PRIVATE
        -mcpu=cortex-m3 -mthumb -mfloat-abi=soft
        "-T${_linker_script}"
        -Wl,--gc-sections
        -Wl,--no-warn-rwx-segments
        "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${YI_PLATFORM_NAME}.map"
        --specs=nosys.specs
    )

    add_custom_command(
        TARGET "${_target}" POST_BUILD
        COMMAND
            "${CMAKE_OBJCOPY}" -O ihex
            "$<TARGET_FILE:${_target}>"
            "${CMAKE_CURRENT_BINARY_DIR}/${YI_PLATFORM_NAME}.hex"
        COMMAND
            "${CMAKE_OBJCOPY}" -O binary
            "$<TARGET_FILE:${_target}>"
            "${CMAKE_CURRENT_BINARY_DIR}/${YI_PLATFORM_NAME}.bin"
        COMMAND "${CMAKE_SIZE}" "$<TARGET_FILE:${_target}>"
        VERBATIM
    )
endfunction()
