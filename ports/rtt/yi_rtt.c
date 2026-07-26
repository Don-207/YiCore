/**
 * @file yi_rtt.c
 * @brief YiCore rtt implementation.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#include "yi_rtt.h"
#include "SEGGER_RTT.h"
#include <stddef.h>

/**
 * @brief Initialize the module.
 * @param config Device configuration.
 */
int yi_rtt_init(const void *config)
{
    const yi_rtt_config_t *cfg = (const yi_rtt_config_t *)config;

    if((cfg == NULL) ||
       (cfg->up_buffer >= SEGGER_RTT_MAX_NUM_UP_BUFFERS) ||
       (cfg->mode > YI_RTT_MODE_BLOCK))
    {
        return -1;
    }
    SEGGER_RTT_Init();
    return (SEGGER_RTT_SetFlagsUpBuffer(cfg->up_buffer, (unsigned)cfg->mode) == 0) ? 0 : -1;
}

/**
 * @brief Perform the yi rtt open operation.
 * @param dev Device instance.
 */
static int yi_rtt_open(yi_device_t *dev)
{
    return (dev != NULL) ? 0 : -1;
}

/**
 * @brief Perform the yi rtt close operation.
 * @param dev Device instance.
 */
static int yi_rtt_close(yi_device_t *dev)
{
    return (dev != NULL) ? 0 : -1;
}

/**
 * @brief Write the module.
 * @param dev Device instance.
 * @param buf Buf value.
 * @param len Len value.
 */
static int yi_rtt_write(yi_device_t *dev, const uint8_t *buf, uint32_t len)
{
    const yi_rtt_config_t *cfg;
    unsigned written;

    if(!yi_device_is_ready(dev) || (dev->config == NULL) ||
       (buf == NULL) || (len == 0U))
    {
        return -1;
    }
    cfg = (const yi_rtt_config_t *)dev->config;
    written = SEGGER_RTT_Write(cfg->up_buffer, buf, len);
    return (written == len) ? 0 : -1;
}

const yi_device_api_t yi_rtt_driver_api =
{
    .open = yi_rtt_open,
    .close = yi_rtt_close,
    .write = yi_rtt_write,
    .read = NULL
};
