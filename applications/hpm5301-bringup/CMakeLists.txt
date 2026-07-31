# File: CMakeLists.txt
# Function: Build the minimum YiCore HPM5301EVKLite architecture image.
# Author: Don
# Date: 2026-07-29
# Version: 1.0.0

cmake_minimum_required(VERSION 3.20)

get_filename_component(YICORE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

if(NOT DEFINED ENV{HPM_SDK_BASE})
    message(
        FATAL_ERROR
        "Set HPM_SDK_BASE to YiHAL-HPMicro/hpm_sdk before configuring"
    )
endif()

if(NOT DEFINED BOARD OR BOARD STREQUAL "")
    set(BOARD "hpm5301evklite" CACHE STRING "Selected HPM5301 board")
endif()
if(BOARD STREQUAL "yi-hpm5301")
    set(
        BOARD_SEARCH_PATH
        "${YICORE_ROOT}/boards"
        CACHE PATH
        "YiCore custom HPM board root"
    )
endif()

set(HPM_BUILD_TYPE "flash_xip" CACHE STRING
    "Official HPM SDK memory layout used by the bring-up image")
set(APP_NAME "yicore-hpm5301-bringup" CACHE STRING
    "Output image base name")
set(STACK_SIZE "0x1000" CACHE STRING
    "Bring-up stack size in bytes")
set(HEAP_SIZE "0x1000" CACHE STRING
    "Bring-up heap size in bytes")
set(
    RV_ARCH
    "rv32imac_zicsr_zifencei"
    CACHE STRING
    "Portable HPM5301 ISA for modern generic GNU RISC-V toolchains"
)
set(RV_ABI "ilp32" CACHE STRING "HPM5301 integer ABI")

find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})

project(yicore_hpm5301_bringup LANGUAGES C ASM)

sdk_app_src(
    src/main.c
    "${YICORE_ROOT}/arch/riscv/yi_riscv_irq.c"
)
sdk_app_inc("${YICORE_ROOT}/arch/riscv")
