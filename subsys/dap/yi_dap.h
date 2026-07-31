/**
 * @file yi_dap.h
 * @brief Expose a stable YiCore lifecycle facade for CMSIS-DAP engines.
 * @author Don
 * @date 2026-08-01
 * @version 1.0.0
 */

#ifndef YI_DAP_H
#define YI_DAP_H

#include <stdbool.h>
#include <stdint.h>

/** YiDAP initialization and foreground-processing configuration. */
typedef struct {
    uint8_t usb_bus_id; /**< CherryUSB device-controller bus identifier. */
    uintptr_t usb_register_base; /**< Device-controller register base address. */
    bool enable_cdc_uart; /**< Process the optional USB CDC-to-UART bridge. */
} yi_dap_config_t;

/**
 * @brief Initialize the selected CMSIS-DAP engine and USB device controller.
 * @param config Immutable product configuration retained by value.
 * @return Zero on success, -1 for invalid configuration, or -2 if initialized.
 * @note Thread context only; initializes USB endpoints and DAP protocol state.
 */
int yi_dap_init(const yi_dap_config_t *config);

/**
 * @brief Process one non-blocking iteration of pending DAP and CDC work.
 * @return None.
 * @note Call continuously from thread context after yi_dap_init().
 */
void yi_dap_process(void);

/**
 * @brief Report whether the YiDAP lifecycle has completed initialization.
 * @return True after successful initialization; otherwise false.
 * @note This is lifecycle state, not USB host configuration state.
 */
bool yi_dap_is_initialized(void);

#endif
