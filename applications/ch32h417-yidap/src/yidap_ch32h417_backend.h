/**
 * @file yidap_ch32h417_backend.h
 * @brief Expose the CH32H417 CherryDAP adapter to the YiDAP lifecycle.
 * @author Don
 * @date 2026-08-01
 * @version 1.0.0
 */

#ifndef YIDAP_CH32H417_BACKEND_H
#define YIDAP_CH32H417_BACKEND_H

#include "yi_dap.h"

/**
 * @brief Return the immutable CH32H417 YiDAP backend configuration.
 * @return Configuration retained for the lifetime of the firmware.
 */
const yi_dap_config_t *yidap_ch32h417_backend_get_config(void);

#endif
