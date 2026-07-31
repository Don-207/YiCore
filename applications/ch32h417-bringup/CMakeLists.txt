# File: CMakeLists.txt
# Function: Build the minimum YiCore CH32H417 V3F GPIO image.
# Author: Don
# Date: 2026-07-31
# Version: 1.0.0

cmake_minimum_required(VERSION 3.20)

get_filename_component(YICORE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(
    CMAKE_TOOLCHAIN_FILE
    "${YICORE_ROOT}/cmake/toolchains/wch-riscv-gcc.cmake"
    CACHE FILEPATH "CH32H417 cross compilation toolchain"
)
project(yicore_ch32h417_bringup LANGUAGES C ASM)

if(NOT DEFINED BOARD OR BOARD STREQUAL "")
    set(BOARD "ch32h417-evt" CACHE STRING "Selected CH32H417 board")
endif()

include("${YICORE_ROOT}/cmake/YiCoreApplication.cmake")
yi_application(
    NAME yicore-ch32h417-bringup
    SOURCES "${CMAKE_CURRENT_LIST_DIR}/src/main.c"
)
