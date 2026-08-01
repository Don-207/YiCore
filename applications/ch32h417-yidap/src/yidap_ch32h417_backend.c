/**
 * @file yidap_ch32h417_backend.c
 * @brief Adapt the native YiDAP USBFS transport to the YiDAP lifecycle.
 * @author Don
 * @date 2026-08-01
 * @version 1.0.0
 */

#include "yidap_ch32h417_backend.h"

#include <stddef.h>

#include "yidap_ch32h417_usb.h"

/**
 * @brief Initialize native YiDAP on the CH32H417 USBFS device controller.
 * @param context Unused product context.
 * @return Zero after descriptors, endpoints, and target GPIO are initialized.
 * @note Thread context only; USB interrupts become active before return.
 */
static int yidap_ch32h417_backend_init(void *context)
{
    (void)context;
    return yidap_ch32h417_usb_init();
}

/**
 * @brief Process one non-blocking native YiDAP USB iteration.
 * @param context Unused product context.
 * @note Call continuously from the V3F foreground loop.
 */
static void yidap_ch32h417_backend_process(void *context)
{
    (void)context;
    yidap_ch32h417_usb_process();
}

/** Native YiDAP operations selected by the CH32H417 product. */
static const yi_dap_backend_api_t yidap_ch32h417_backend_api = {
    .init = yidap_ch32h417_backend_init,
    .process = yidap_ch32h417_backend_process,
};

/** Immutable native backend binding with no additional product context. */
static const yi_dap_config_t yidap_ch32h417_config = {
    .backend_api = &yidap_ch32h417_backend_api,
    .backend_context = NULL,
};

/**
 * @brief Return the CH32H417 native YiDAP backend binding.
 * @return Address of the immutable backend configuration.
 */
const yi_dap_config_t *yidap_ch32h417_backend_get_config(void)
{
    return &yidap_ch32h417_config;
}
