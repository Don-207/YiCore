# File: CMakeLists.txt
# Function: Register the Yi HPM5301 custom board implementation.
# Author: Don
# Date: 2026-07-29
# Version: 1.0.0

set(YILINK_HPM_REFERENCE_BOARD "${HPM_SDK_BASE}/boards/hpm5301evklite")
set(YI_HPM5301_SOC_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../soc/hpmicro/hpm5300")

sdk_inc(.)
sdk_inc("${HPM_SDK_BASE}/boards")
sdk_inc("${YILINK_HPM_REFERENCE_BOARD}")

# YiLink uses the same HPM5301 power and 24 MHz clock baseline as EVKLite.
sdk_src("${YILINK_HPM_REFERENCE_BOARD}/board.c")
sdk_src("${YILINK_HPM_REFERENCE_BOARD}/clock.c")
sdk_src("${YILINK_HPM_REFERENCE_BOARD}/pinmux.c")
sdk_src("${YI_HPM5301_SOC_ROOT}/yi_hpm5301_board.c")
