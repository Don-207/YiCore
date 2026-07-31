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

/** Backend operations implemented by the selected CMSIS-DAP engine adapter. */
typedef struct {
    int (*init)(void *context); /**< Initialize backend protocol and transport state. */
    void (*process)(void *context); /**< Process one non-blocking backend iteration. */
} yi_dap_backend_api_t;

/** YiDAP backend selection and opaque product context. */
typedef struct {
    const yi_dap_backend_api_t *backend_api; /**< Selected backend operations. */
    void *backend_context; /**< Product-owned context passed to backend operations. */
} yi_dap_config_t;

/**
 * @brief Initialize the selected CMSIS-DAP engine and USB device controller.
 * @param config Immutable product configuration retained by value.
 * @return Zero on success, -1 for invalid configuration, -2 if initialized,
 * or the backend initialization error.
 * @note Thread context only; delegates transport and protocol setup to backend.
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
