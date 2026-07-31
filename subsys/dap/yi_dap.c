/**
 * @file yi_dap.c
 * @brief Implement the backend-neutral YiDAP lifecycle and diagnostics.
 * @author Don
 * @date 2026-08-01
 * @version 1.0.0
 */

#include "yi_dap.h"

#include <stddef.h>

/** Retained product configuration used by foreground processing. */
static yi_dap_config_t yi_dap_config;

/** Current lifecycle state exposed to product diagnostics. */
static yi_dap_state_t yi_dap_state = YI_DAP_STATE_UNINITIALIZED;

/** Most recent facade or backend initialization result. */
static int yi_dap_last_error = YI_DAP_ERROR_NONE;

/**
 * @brief Initialize the selected backend behind the YiDAP lifecycle boundary.
 * @param config Backend operations and product-owned opaque context.
 * @return Zero on success, a facade error, or the backend initialization error.
 * @note A failed initialization may be retried with a valid configuration.
 */
int yi_dap_init(const yi_dap_config_t *config)
{
    /** Backend-specific initialization result propagated to the product. */
    int result;

    if ((config == NULL) || (config->backend_api == NULL) ||
        (config->backend_api->init == NULL) ||
        (config->backend_api->process == NULL)) {
        yi_dap_state = YI_DAP_STATE_ERROR;
        yi_dap_last_error = YI_DAP_ERROR_INVALID_CONFIG;
        return yi_dap_last_error;
    }
    if ((yi_dap_state == YI_DAP_STATE_INITIALIZING) ||
        (yi_dap_state == YI_DAP_STATE_READY)) {
        yi_dap_last_error = YI_DAP_ERROR_ALREADY_INITIALIZED;
        return yi_dap_last_error;
    }

    yi_dap_state = YI_DAP_STATE_INITIALIZING;
    yi_dap_last_error = YI_DAP_ERROR_NONE;
    result = config->backend_api->init(config->backend_context);
    if (result != 0) {
        yi_dap_state = YI_DAP_STATE_ERROR;
        yi_dap_last_error = result;
        return result;
    }

    yi_dap_config = *config;
    yi_dap_state = YI_DAP_STATE_READY;
    return 0;
}

/**
 * @brief Process pending backend protocol and transport work.
 * @return None.
 * @note Non-blocking thread-context function; safely ignores pre-init calls.
 */
void yi_dap_process(void)
{
    if (yi_dap_state != YI_DAP_STATE_READY) {
        return;
    }

    yi_dap_config.backend_api->process(yi_dap_config.backend_context);
}

/**
 * @brief Return the YiDAP lifecycle initialization state.
 * @return True after successful initialization; otherwise false.
 * @note Does not inspect USB enumeration or target connection state.
 */
bool yi_dap_is_initialized(void)
{
    return yi_dap_state == YI_DAP_STATE_READY;
}

/**
 * @brief Return the current backend-neutral lifecycle state.
 * @return Current YiDAP lifecycle state.
 * @note Safe to call before, during, or after initialization.
 */
yi_dap_state_t yi_dap_get_state(void)
{
    return yi_dap_state;
}

/**
 * @brief Return the most recently recorded initialization error.
 * @return Zero or the most recent facade or backend error code.
 * @note An already-initialized call records its error without leaving READY.
 */
int yi_dap_get_last_error(void)
{
    return yi_dap_last_error;
}
