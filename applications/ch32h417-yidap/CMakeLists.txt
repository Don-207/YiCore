# File: CMakeLists.txt
# Function: Build the YiDAP CMSIS-DAP image for the CH32H417 V3F core.
# Author: Don
# Date: 2026-08-01
# Version: 1.0.0

cmake_minimum_required(VERSION 3.20)

get_filename_component(YICORE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(CMAKE_TOOLCHAIN_FILE "${YICORE_ROOT}/cmake/toolchains/wch-riscv-gcc.cmake"
    CACHE FILEPATH "CH32H417 cross compilation toolchain")
project(yicore_ch32h417_yidap LANGUAGES C ASM)

if(NOT DEFINED BOARD OR BOARD STREQUAL "")
    set(BOARD "ch32h417-evt" CACHE STRING "Selected CH32H417 board")
endif()

set(CHERRYDAP_ROOT "${YICORE_ROOT}/../YiLink/modules/lib/cherrydap"
    CACHE PATH "CherryDAP source tree")
if(NOT EXISTS "${CHERRYDAP_ROOT}/dap_main.c")
    message(FATAL_ERROR "CherryDAP is missing; set CHERRYDAP_ROOT")
endif()

include("${YICORE_ROOT}/cmake/YiCoreApplication.cmake")
set_source_files_properties(
    "${CHERRYDAP_ROOT}/CherryUSB/port/ch32/usb_dc_usbfs.c"
    PROPERTIES COMPILE_DEFINITIONS
        "YIDAP_CH32H417_USBFS_PORT=1;USBFS_BASE=0x40023400UL"
)
yi_application(
    NAME yicore-ch32h417-yidap
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/src/main.c"
        "${CMAKE_CURRENT_LIST_DIR}/src/yidap_ch32h417_backend.c"
        "${CMAKE_CURRENT_LIST_DIR}/src/yidap_ch32h417_usb.c"
        "${YICORE_ROOT}/subsys/dap/yi_dap.c"
        "${CHERRYDAP_ROOT}/dap_main.c"
        "${CHERRYDAP_ROOT}/DAP/Source/DAP.c"
        "${CHERRYDAP_ROOT}/DAP/Source/SW_DP.c"
        "${CHERRYDAP_ROOT}/DAP/Source/JTAG_DP.c"
        "${CHERRYDAP_ROOT}/CherryRB/chry_ringbuffer.c"
        "${CHERRYDAP_ROOT}/CherryUSB/core/usbd_core.c"
        "${CHERRYDAP_ROOT}/CherryUSB/class/cdc/usbd_cdc_acm.c"
        "${CHERRYDAP_ROOT}/CherryUSB/port/ch32/usb_dc_usbfs.c"
    INCLUDE_DIRS
        "${CMAKE_CURRENT_LIST_DIR}/src"
        "${YICORE_ROOT}/subsys/dap"
        "${CHERRYDAP_ROOT}"
        "${CHERRYDAP_ROOT}/CherryRB"
        "${CHERRYDAP_ROOT}/DAP/Include"
        "${CHERRYDAP_ROOT}/CherryUSB/common"
        "${CHERRYDAP_ROOT}/CherryUSB/core"
        "${CHERRYDAP_ROOT}/CherryUSB/class/cdc"
        "${CHERRYDAP_ROOT}/CherryUSB/class/hid"
        "${CHERRYDAP_ROOT}/CherryUSB/class/msc"
)
