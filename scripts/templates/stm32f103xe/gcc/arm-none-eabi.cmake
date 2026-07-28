# File: arm-none-eabi.cmake
# Function: Configure CMake for an ARM Cortex-M bare-metal GCC toolchain.
# Author: Don
# Date: 2026-07-27
# Version: 1.0.0

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(ARM_GCC_PREFIX "arm-none-eabi" CACHE STRING
    "Executable prefix, for example arm-none-eabi or arm-xilinx-eabi")
set(ARM_GCC_ROOT "" CACHE PATH "Directory containing the ARM GCC executables")

if(ARM_GCC_ROOT)
    set(_ARM_GCC_DIR "${ARM_GCC_ROOT}/")
else()
    set(_ARM_GCC_DIR "")
endif()

set(CMAKE_C_COMPILER   "${_ARM_GCC_DIR}${ARM_GCC_PREFIX}-gcc.exe")
set(CMAKE_ASM_COMPILER "${_ARM_GCC_DIR}${ARM_GCC_PREFIX}-gcc.exe")
set(CMAKE_OBJCOPY      "${_ARM_GCC_DIR}${ARM_GCC_PREFIX}-objcopy.exe")
set(CMAKE_SIZE         "${_ARM_GCC_DIR}${ARM_GCC_PREFIX}-size.exe")
