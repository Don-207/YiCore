/**
 * @file yi_dap.c
 * @brief Adapt the CherryDAP lifecycle to the stable YiDAP interface.
 * @author Don
 * @date 2026-08-01
 * @version 1.0.0
 */

#include "yi_dap.h"

#include <limits.h>

#include "dap_main.h"

/** Retained product configuration used by foreground processing. */
static yi_dap_config_t yi_dap_config;

/** True after CherryDAP has initialized its protocol and USB state. */
static bool yi_dap_initialized;

/**
 * @brief Initialize CherryDAP behind the YiDAP lifecycle boundary.
 * @param config USB controller and optional CDC processing configuration.
 * @return Zero on success, -1 for invalid configuration, or -2 if initialized.
 * @note Thread context only; CherryDAP currently provides no deinit operation.
 */
int yi_dap_init(const yi_dap_config_t *config)
{
    if ((config == NULL) ||
        (config->usb_register_base == (uintptr_t)0U) ||
        (config->usb_register_base > (uintptr_t)UINT32_MAX)) {
        return -1;
    }
    if (yi_dap_initialized) {
        return -2;
    }

    yi_dap_config = *config;
    chry_dap_init(
        yi_dap_config.usb_bus_id,
        (uint32_t)yi_dap_config.usb_register_base
    );
    yi_dap_initialized = true;
    return 0;
}

/**
 * @brief Process pending CherryDAP protocol and optional CDC bridge work.
 * @return None.
 * @note Non-blocking thread-context function; safely ignores pre-init calls.
 */
void yi_dap_process(void)
{
    if (!yi_dap_initialized) {
        return;
    }

    chry_dap_handle();
    if (yi_dap_config.enable_cdc_uart) {
        chry_dap_usb2uart_handle();
    }
}

/**
 * @brief Return the YiDAP lifecycle initialization state.
 * @return True after successful initialization; otherwise false.
 * @note Does not inspect USB enumeration or target connection state.
 */
bool yi_dap_is_initialized(void)
{
    return yi_dap_initialized;
}
