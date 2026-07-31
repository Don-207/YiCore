/**
 * @file yi_dap.c
 * @brief Adapt the CherryDAP lifecycle to the stable YiDAP interface.
 * @author Don
 * @date 2026-08-01
 * @version 1.0.0
 */

#include "yi_dap.h"

#include <stddef.h>

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
    /** Backend-specific initialization result propagated to the product. */
    int result;

    if ((config == NULL) || (config->backend_api == NULL) ||
        (config->backend_api->init == NULL) ||
        (config->backend_api->process == NULL)) {
        return -1;
    }
    if (yi_dap_initialized) {
        return -2;
    }

    result = config->backend_api->init(config->backend_context);
    if (result != 0) {
        return result;
    }

    yi_dap_config = *config;
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

    yi_dap_config.backend_api->process(yi_dap_config.backend_context);
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
