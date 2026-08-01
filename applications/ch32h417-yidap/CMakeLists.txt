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

set(CHERRYUSB_ROOT "${YICORE_ROOT}/../YiLink/modules/lib/cherrydap/CherryUSB"
    CACHE PATH "CherryUSB source tree")
if(NOT EXISTS "${CHERRYUSB_ROOT}/core/usbd_core.c")
    message(FATAL_ERROR "CherryUSB is missing; set CHERRYUSB_ROOT")
endif()

include("${YICORE_ROOT}/cmake/YiCoreApplication.cmake")
set_source_files_properties(
    "${CHERRYUSB_ROOT}/port/ch32/usb_dc_usbfs.c"
    PROPERTIES COMPILE_DEFINITIONS
        "YIDAP_CH32H417_USBFS_PORT=1;USBFS_BASE=0x40023400UL"
)
yi_application(
    NAME yicore-ch32h417-yidap
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/src/main.c"
        "${CMAKE_CURRENT_LIST_DIR}/src/yidap_ch32h417_backend.c"
        "${CMAKE_CURRENT_LIST_DIR}/src/yidap_ch32h417_usb.c"
        "${CMAKE_CURRENT_LIST_DIR}/src/yidap_ch32h417_usb_device.c"
        "${CMAKE_CURRENT_LIST_DIR}/src/yidap_ch32h417_port.c"
        "${CMAKE_CURRENT_LIST_DIR}/src/yidap_ch32h417_peripherals.c"
        "${CMAKE_CURRENT_LIST_DIR}/src/yidap_ch32h417_vendor.c"
        "${CMAKE_CURRENT_LIST_DIR}/src/yidap_ch32h417_fpga.c"
        "${YICORE_ROOT}/subsys/dap/yi_dap.c"
        "${YICORE_ROOT}/subsys/dap/yi_dap_protocol.c"
        "${CHERRYUSB_ROOT}/core/usbd_core.c"
        "${CHERRYUSB_ROOT}/port/ch32/usb_dc_usbfs.c"
        "${YICORE_ROOT}/../modules/hal/wch/Peripheral/src/ch32h417_i2c.c"
        "${YICORE_ROOT}/../modules/hal/wch/Peripheral/src/ch32h417_spi.c"
    INCLUDE_DIRS
        "${CMAKE_CURRENT_LIST_DIR}/src"
        "${YICORE_ROOT}/subsys/dap"
        "${CHERRYUSB_ROOT}/common"
        "${CHERRYUSB_ROOT}/core"
)
