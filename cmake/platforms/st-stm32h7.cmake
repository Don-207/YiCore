# File: st-stm32h7.cmake
# Function: Configure the minimal STM32H743 YiCore GCC platform.
# Author: Don
# Date: 2026-07-31
# Version: 1.0.0

set(YI_ST_DEVICE_DIRECTORY STM32H7xx)
set(YI_ST_DEVICE_HEADER stm32h743xx.h)
set(YI_ST_DEVICE_DEFINE STM32H743xx)
set(YI_ST_SOC_HEADER stm32h7xx.h)
set(YI_ST_HAL_DIRECTORY stm32h7xx_hal_driver)
set(YI_ST_HAL_HEADER stm32h7xx_hal.h)
set(YI_ST_HAL_PREFIX stm32h7xx)
set(YI_ST_SYSTEM_SOURCE system_stm32h7xx.c)
set(YI_ST_STARTUP_SOURCE startup_stm32h743xx.s)
set(YI_ST_SOC_ROOT "${YICORE_ROOT}/soc/st/stm32/stm32h7")
set(YI_ST_SYSTEM_IMPL yi_stm32h7_system.c)
set(YI_ST_RUNTIME_IMPL yi_stm32h7_runtime.c)
set(YI_ST_LINKER_SCRIPT "${YICORE_ROOT}/linker/gcc/stm32h743zi.ld")
set(
    YI_ST_CPU_OPTIONS
    -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard
)
include("${CMAKE_CURRENT_LIST_DIR}/st-stm32-bootstrap.cmake")

# Build one minimal STM32H743 application.
function(yi_platform_application)
    yi_stm32_bootstrap_application(${ARGN})
endfunction()
