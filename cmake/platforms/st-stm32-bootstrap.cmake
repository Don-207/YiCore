# File: st-stm32-bootstrap.cmake
# Function: Build a minimal YiCore STM32 platform image from CMSIS and HAL.
# Author: Don
# Date: 2026-07-31
# Version: 1.0.0

include(CMakeParseArguments)

# Build a minimal STM32 application while a full peripheral backend is staged.
function(yi_stm32_bootstrap_application)
    set(_options)
    set(_one_value NAME)
    set(_multi_value SOURCES)
    cmake_parse_arguments(
        YI_BOOT "${_options}" "${_one_value}" "${_multi_value}" ${ARGN}
    )
    if(NOT YI_BOOT_NAME OR NOT YI_BOOT_SOURCES)
        message(FATAL_ERROR "STM32 bootstrap adapter requires NAME and SOURCES")
    endif()

    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    set(_generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
    set(_resolved_dts "${_generated_dir}/app.dts")
    file(MAKE_DIRECTORY "${_generated_dir}")
    file(TO_CMAKE_PATH "${YI_APP_BOARD_DIR}/board.dts" _board_dts)
    file(TO_CMAKE_PATH "${YI_APP_OVERLAY}" _app_overlay)
    file(
        WRITE "${_resolved_dts}"
        "/include/ \"${_board_dts}\"\n/include/ \"${_app_overlay}\"\n"
    )
    execute_process(
        COMMAND
            "${Python3_EXECUTABLE}" "${YICORE_ROOT}/scripts/yi_dts_gen.py"
            --dts "${_resolved_dts}"
            --bindings "${YICORE_ROOT}/dts/bindings"
            --output "${_generated_dir}"
            --soc-header "${YI_ST_SOC_HEADER}"
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
        YI_HAL_ST_ROOT "${YICORE_ROOT}/../modules/hal/st"
        CACHE PATH "Root of the external YiHAL-ST module"
    )
    set(_cmsis_root "${YI_HAL_ST_ROOT}/cmsis")
    set(_device_root
        "${_cmsis_root}/Device/ST/${YI_ST_DEVICE_DIRECTORY}")
    set(_hal_root
        "${YI_HAL_ST_ROOT}/stm32cube/${YI_ST_HAL_DIRECTORY}")
    if(NOT EXISTS "${_device_root}/Include/${YI_ST_DEVICE_HEADER}")
        message(FATAL_ERROR "Missing CMSIS device package at ${_device_root}")
    endif()
    if(NOT EXISTS "${_hal_root}/Inc/${YI_ST_HAL_HEADER}")
        message(FATAL_ERROR "Missing STM32 HAL package at ${_hal_root}")
    endif()

    set(_target "${YI_BOOT_NAME}.elf")
    add_executable(
        "${_target}"
        ${YI_BOOT_SOURCES}
        "${_device_root}/Source/Templates/${YI_ST_SYSTEM_SOURCE}"
        "${_device_root}/Source/Templates/gcc/${YI_ST_STARTUP_SOURCE}"
        "${_hal_root}/Src/${YI_ST_HAL_PREFIX}_hal.c"
        "${_hal_root}/Src/${YI_ST_HAL_PREFIX}_hal_cortex.c"
        "${_hal_root}/Src/${YI_ST_HAL_PREFIX}_hal_flash.c"
        "${_hal_root}/Src/${YI_ST_HAL_PREFIX}_hal_flash_ex.c"
        "${_hal_root}/Src/${YI_ST_HAL_PREFIX}_hal_gpio.c"
        "${_hal_root}/Src/${YI_ST_HAL_PREFIX}_hal_pwr.c"
        "${_hal_root}/Src/${YI_ST_HAL_PREFIX}_hal_pwr_ex.c"
        "${_hal_root}/Src/${YI_ST_HAL_PREFIX}_hal_rcc.c"
        "${_hal_root}/Src/${YI_ST_HAL_PREFIX}_hal_rcc_ex.c"
        "${YICORE_ROOT}/core/yi_device.c"
        "${YICORE_ROOT}/core/yi_build_info.c"
        "${YICORE_ROOT}/drivers/led/yi_led.c"
        "${YICORE_ROOT}/ports/newlib/yi_newlib_syscalls.c"
        "${YICORE_ROOT}/soc/st/stm32/yi_clock_stm32.c"
        "${YICORE_ROOT}/soc/st/stm32/yi_gpio_stm32.c"
        "${YI_ST_SOC_ROOT}/${YI_ST_SYSTEM_IMPL}"
        "${YI_ST_SOC_ROOT}/${YI_ST_RUNTIME_IMPL}"
        "${_generated_dir}/yi_generated.c"
        "${_generated_dir}/yi_build_info.c"
    )
    target_compile_definitions(
        "${_target}" PRIVATE USE_HAL_DRIVER "${YI_ST_DEVICE_DEFINE}"
    )
    target_include_directories(
        "${_target}" PRIVATE
        "${_generated_dir}"
        "${YI_ST_SOC_ROOT}"
        "${_hal_root}/Inc"
        "${_hal_root}/Inc/Legacy"
        "${_device_root}/Include"
        "${_cmsis_root}/Include"
        "${YICORE_ROOT}/core"
        "${YICORE_ROOT}/drivers/clock"
        "${YICORE_ROOT}/drivers/gpio"
        "${YICORE_ROOT}/drivers/led"
    )
    target_compile_options(
        "${_target}" PRIVATE
        ${YI_ST_CPU_OPTIONS}
        -Os -ffunction-sections -fdata-sections -fno-common
        -Wall -Wextra
    )
    set_property(
        TARGET "${_target}" PROPERTY LINK_DEPENDS "${YI_ST_LINKER_SCRIPT}"
    )
    target_link_options(
        "${_target}" PRIVATE
        ${YI_ST_CPU_OPTIONS}
        "-T${YI_ST_LINKER_SCRIPT}"
        -Wl,--gc-sections
        -Wl,--no-warn-rwx-segments
        "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${YI_BOOT_NAME}.map"
        --specs=nosys.specs
    )
    add_custom_command(
        TARGET "${_target}" POST_BUILD
        COMMAND "${CMAKE_OBJCOPY}" -O ihex
                "$<TARGET_FILE:${_target}>"
                "${CMAKE_CURRENT_BINARY_DIR}/${YI_BOOT_NAME}.hex"
        COMMAND "${CMAKE_OBJCOPY}" -O binary
                "$<TARGET_FILE:${_target}>"
                "${CMAKE_CURRENT_BINARY_DIR}/${YI_BOOT_NAME}.bin"
        COMMAND "${CMAKE_SIZE}" "$<TARGET_FILE:${_target}>"
        VERBATIM
    )
endfunction()
