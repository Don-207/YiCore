# File: wch-riscv-gcc.cmake
# Function: Select the WCH GNU RISC-V cross toolchain for CH32 targets.
# Author: Don
# Date: 2026-07-31
# Version: 1.0.0

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv)

if(NOT DEFINED ENV{WCH_RISCV_TOOLCHAIN_PATH})
    message(FATAL_ERROR "Set WCH_RISCV_TOOLCHAIN_PATH to the WCH GCC root")
endif()

file(TO_CMAKE_PATH "$ENV{WCH_RISCV_TOOLCHAIN_PATH}" _wch_toolchain_root)
if(EXISTS "${_wch_toolchain_root}/riscv32-wch-elf-gcc.exe" OR
   EXISTS "${_wch_toolchain_root}/riscv-none-elf-gcc.exe")
    set(_wch_toolchain_bin "${_wch_toolchain_root}")
else()
    set(_wch_toolchain_bin "${_wch_toolchain_root}/bin")
endif()
if(EXISTS "${_wch_toolchain_bin}/riscv32-wch-elf-gcc.exe")
    set(_wch_toolchain_prefix "riscv32-wch-elf")
else()
    set(_wch_toolchain_prefix "riscv-none-elf")
endif()
set(CMAKE_C_COMPILER
    "${_wch_toolchain_bin}/${_wch_toolchain_prefix}-gcc.exe")
set(CMAKE_ASM_COMPILER "${CMAKE_C_COMPILER}")
set(CMAKE_OBJCOPY
    "${_wch_toolchain_bin}/${_wch_toolchain_prefix}-objcopy.exe")
set(CMAKE_SIZE "${_wch_toolchain_bin}/${_wch_toolchain_prefix}-size.exe")
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
