# File: wch-ch32h4xx.cmake
# Function: Build thin YiCore applications for the CH32H417 V3F core.
# Author: Don
# Date: 2026-07-31
# Version: 1.0.0

include(CMakeParseArguments)

# Build one CH32H417 V3F image from application and YiCore sources.
function(yi_platform_application)
    set(_options)
    set(_one_value NAME)
    set(_multi_value SOURCES INCLUDE_DIRS COMPILE_DEFINITIONS)
    cmake_parse_arguments(
        YI_PLATFORM "${_options}" "${_one_value}" "${_multi_value}" ${ARGN}
    )
    if(NOT YI_PLATFORM_NAME OR NOT YI_PLATFORM_SOURCES)
        message(FATAL_ERROR "CH32H4xx adapter requires NAME and SOURCES")
    endif()

    set(
        YI_HAL_WCH_ROOT "${YICORE_ROOT}/../modules/hal/wch"
        CACHE PATH "Root of the external YiHAL-WCH module"
    )
    if(NOT EXISTS "${YI_HAL_WCH_ROOT}/Peripheral/inc/ch32h417.h")
        message(FATAL_ERROR "YiHAL-WCH is missing; run 'yi update'")
    endif()

    set(_generated_sources)
    if(DEFINED YI_APP_OVERLAY AND EXISTS "${YI_APP_OVERLAY}")
        find_package(Python3 REQUIRED COMPONENTS Interpreter)
        set(_generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
        set(_resolved_dts "${_generated_dir}/app.dts")
        file(MAKE_DIRECTORY "${_generated_dir}")
        file(TO_CMAKE_PATH "${YI_APP_BOARD_DIR}/board.dts" _board_dts)
        file(TO_CMAKE_PATH "${YI_APP_OVERLAY}" _app_overlay)
        file(WRITE "${_resolved_dts}"
            "/include/ \"${_board_dts}\"\n"
            "/include/ \"${_app_overlay}\"\n")
        execute_process(
            COMMAND "${Python3_EXECUTABLE}" "${YICORE_ROOT}/scripts/yi_dts_gen.py"
                --dts "${_resolved_dts}"
                --bindings "${YICORE_ROOT}/dts/bindings"
                --output "${_generated_dir}"
                --soc-header "ch32h417.h"
            RESULT_VARIABLE _dts_result
        )
        if(NOT _dts_result EQUAL 0)
            message(FATAL_ERROR "CH32H417 Devicetree generation failed")
        endif()
        list(APPEND _generated_sources
            "${_generated_dir}/yi_generated.c"
            "${YICORE_ROOT}/core/yi_device.c"
            "${YICORE_ROOT}/drivers/led/yi_led.c"
            "${YICORE_ROOT}/soc/wch/ch32h4xx/yi_uart_ch32h4xx.c"
            "${YICORE_ROOT}/soc/wch/ch32h4xx/yi_gpio_ch32h4xx.c"
        )
    endif()

    set(_target "${YI_PLATFORM_NAME}.elf")
    set(_linker_script "${YI_HAL_WCH_ROOT}/Ld/V3F/Link_v3f.ld")
    add_executable(
        "${_target}"
        ${YI_PLATFORM_SOURCES}
        ${_generated_sources}
        "${YI_HAL_WCH_ROOT}/Startup/startup_ch32h417_v3f.S"
        "${YI_HAL_WCH_ROOT}/System/system_ch32h417.c"
        "${YI_HAL_WCH_ROOT}/Peripheral/src/ch32h417_flash.c"
        "${YI_HAL_WCH_ROOT}/Peripheral/src/ch32h417_gpio.c"
        "${YI_HAL_WCH_ROOT}/Peripheral/src/ch32h417_rcc.c"
        "${YI_HAL_WCH_ROOT}/Peripheral/src/ch32h417_usart.c"
        "${YICORE_ROOT}/arch/riscv/yi_riscv_irq.c"
        "${YICORE_ROOT}/core/yi_poll.c"
        "${YICORE_ROOT}/soc/wch/ch32h4xx/yi_ch32h417_system.c"
    )
    target_compile_definitions(
        "${_target}" PRIVATE Core_V3F YI_DEVICE_USE_AUTO_SECTION=1
    )
    target_compile_definitions(
        "${_target}" PRIVATE ${YI_PLATFORM_COMPILE_DEFINITIONS}
    )
    target_include_directories(
        "${_target}" PRIVATE
        "${YI_HAL_WCH_ROOT}/Core"
        "${YI_HAL_WCH_ROOT}/Peripheral/inc"
        "${YI_HAL_WCH_ROOT}/System"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${YICORE_ROOT}/arch/riscv"
        "${YICORE_ROOT}/core"
        "${YICORE_ROOT}/drivers/gpio"
        "${YICORE_ROOT}/drivers/led"
        "${YICORE_ROOT}/drivers/uart"
        "${_generated_dir}"
        ${YI_PLATFORM_INCLUDE_DIRS}
    )
    target_compile_options(
        "${_target}" PRIVATE
        -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore
        -Os -ffunction-sections -fdata-sections -fno-common
        -Wall -Wextra
    )
    set_property(TARGET "${_target}" PROPERTY LINK_DEPENDS "${_linker_script}")
    target_link_options(
        "${_target}" PRIVATE
        -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore
        -nostartfiles "-T${_linker_script}" -Wl,--gc-sections
        "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${YI_PLATFORM_NAME}.map"
        --specs=nano.specs --specs=nosys.specs -lm
    )
    add_custom_command(
        TARGET "${_target}" POST_BUILD
        COMMAND "${CMAKE_OBJCOPY}" -O ihex "$<TARGET_FILE:${_target}>"
                "${CMAKE_CURRENT_BINARY_DIR}/${YI_PLATFORM_NAME}.hex"
        COMMAND "${CMAKE_OBJCOPY}" -O binary "$<TARGET_FILE:${_target}>"
                "${CMAKE_CURRENT_BINARY_DIR}/${YI_PLATFORM_NAME}.bin"
        COMMAND "${CMAKE_SIZE}" "$<TARGET_FILE:${_target}>"
        VERBATIM
    )
endfunction()
