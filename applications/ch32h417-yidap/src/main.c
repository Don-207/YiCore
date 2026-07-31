/**
 * @file main.c
 * @brief Run YiDAP with a CherryDAP USBFS backend on CH32H417 V3F.
 * @author Don
 * @date 2026-08-01
 * @version 1.0.0
 */

#include "yi_dap.h"
#include "yi_system.h"
#include "yidap_ch32h417_backend.h"
#include "yidap_ch32h417_peripherals.h"

/**
 * @brief Initialize the V3F platform and continuously service CMSIS-DAP work.
 * @return A YiDAP initialization error, or never returns after success.
 */
int main(void)
{
    /** YiDAP initialization result propagated if the backend cannot start. */
    int result;

    (void)yi_system_init();
    yidap_peripherals_init();
    result = yi_dap_init(yidap_ch32h417_backend_get_config());
    if (result != 0) {
        return result;
    }

    for (;;) {
        yi_dap_process();
    }
}
