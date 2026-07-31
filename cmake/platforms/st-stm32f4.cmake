# File: st-stm32f4.cmake
# Function: Configure the minimal STM32F407 YiCore GCC platform.
# Author: Don
# Date: 2026-07-31
# Version: 1.0.0

set(YI_ST_DEVICE_DIRECTORY STM32F4xx)
set(YI_ST_DEVICE_HEADER stm32f407xx.h)
set(YI_ST_DEVICE_DEFINE STM32F407xx)
set(YI_ST_SOC_HEADER stm32f4xx.h)
set(YI_ST_HAL_DIRECTORY stm32f4xx_hal_driver)
set(YI_ST_HAL_HEADER stm32f4xx_hal.h)
set(YI_ST_HAL_PREFIX stm32f4xx)
set(YI_ST_SYSTEM_SOURCE system_stm32f4xx.c)
set(YI_ST_STARTUP_SOURCE startup_stm32f407xx.s)
set(YI_ST_SOC_ROOT "${YICORE_ROOT}/soc/st/stm32/stm32f4")
set(YI_ST_SYSTEM_IMPL yi_stm32f4_system.c)
set(YI_ST_RUNTIME_IMPL yi_stm32f4_runtime.c)
set(YI_ST_LINKER_SCRIPT "${YICORE_ROOT}/linker/gcc/stm32f407zg.ld")
set(
    YI_ST_CPU_OPTIONS
    -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
)
include("${CMAKE_CURRENT_LIST_DIR}/st-stm32-bootstrap.cmake")

# Build one minimal STM32F407 application.
function(yi_platform_application)
    yi_stm32_bootstrap_application(${ARGN})
endfunction()
