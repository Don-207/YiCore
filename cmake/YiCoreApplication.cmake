# File: YiCoreApplication.cmake
# Function: Define the common contract for thin YiCore applications.
# Author: Don
# Date: 2026-07-28
# Version: 1.0.0

include_guard(GLOBAL)
include(CMakeParseArguments)

# Define a thin YiCore application and select its board at configure time.
function(yi_application)
    set(_options)
    set(_one_value NAME DTS CONF VERSION_FILE)
    set(_multi_value SOURCES)
    cmake_parse_arguments(
        YI_APP
        "${_options}"
        "${_one_value}"
        "${_multi_value}"
        ${ARGN}
    )

    if(NOT YI_APP_NAME OR NOT YI_APP_SOURCES)
        message(FATAL_ERROR "yi_application requires NAME and SOURCES")
    endif()
    if(NOT DEFINED BOARD OR BOARD STREQUAL "")
        message(FATAL_ERROR "Select a board with -DBOARD=<board-id>")
    endif()

    set(_board_dir "${YICORE_ROOT}/boards/${BOARD}")
    set(_board_manifest "${_board_dir}/board.json")
    if(NOT EXISTS "${_board_manifest}")
        message(FATAL_ERROR "Unknown YiCore board: ${BOARD}")
    endif()

    file(READ "${_board_manifest}" _board_json)
    string(JSON _board_vendor GET "${_board_json}" vendor)
    string(JSON _board_series GET "${_board_json}" series)
    set(
        _platform_adapter
        "${YICORE_ROOT}/cmake/platforms/${_board_vendor}-${_board_series}.cmake"
    )
    if(NOT EXISTS "${_platform_adapter}")
        message(
            FATAL_ERROR
            "Board ${BOARD} has no build adapter: "
            "${_board_vendor}/${_board_series}"
        )
    endif()

    set(YI_APP_BOARD "${BOARD}")
    set(YI_APP_BOARD_DIR "${_board_dir}")
    set(YI_APP_OVERLAY "${YI_APP_DTS}")
    set(YI_APP_CONF "${YI_APP_CONF}")
    set(YI_APP_VERSION_FILE "${YI_APP_VERSION_FILE}")
    include("${_platform_adapter}")
    yi_platform_application(
        NAME "${YI_APP_NAME}"
        SOURCES ${YI_APP_SOURCES}
    )
endfunction()
