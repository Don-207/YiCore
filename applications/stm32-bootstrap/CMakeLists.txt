# File: CMakeLists.txt
# Function: Build the minimal STM32F407 or STM32H743 YiCore bootstrap image.
# Author: Don
# Date: 2026-07-31
# Version: 1.0.0

cmake_minimum_required(VERSION 3.20)
project(yicore_stm32_bootstrap LANGUAGES C ASM)

get_filename_component(YICORE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
include("${YICORE_ROOT}/cmake/YiCoreApplication.cmake")

yi_application(
    NAME yicore-stm32-bootstrap
    SOURCES "${CMAKE_CURRENT_LIST_DIR}/src/main.c"
    DTS "${CMAKE_CURRENT_LIST_DIR}/app.dts"
    VERSION_FILE "${CMAKE_CURRENT_LIST_DIR}/VERSION"
)
