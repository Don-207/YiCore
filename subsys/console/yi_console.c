/**
 * @file yi_console.c
 * @brief YiCore console implementation.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#include "yi_console.h"
#include <stddef.h>

static yi_device_t *default_console;

/**
 * @brief Set default.
 * @param console Console value.
 */
int yi_console_set_default(yi_device_t *console)
{
    if((console == NULL) ||
       ((default_console != NULL) && (default_console != console)))
    {
        return -1;
    }
    default_console = console;
    return 0;
}

/**
 * @brief Get default.
 */
yi_device_t *yi_console_get_default(void)
{
    return default_console;
}

/**
 * @brief Initialize the module.
 * @param config Device configuration.
 */
int yi_console_init(const void *config)
{
    const yi_console_config_t *cfg = (const yi_console_config_t *)config;

    if((cfg == NULL) || (cfg->self == NULL) ||
       !yi_device_is_ready(cfg->backend) ||
       (cfg->backend->api == NULL) || (cfg->backend->api->write == NULL))
    {
        return -1;
    }
    if(cfg->default_console && (yi_console_set_default(cfg->self) != 0))
    {
        return -1;
    }
    return 0;
}

/**
 * @brief Perform the yi console open operation.
 * @param dev Device instance.
 */
static int yi_console_open(yi_device_t *dev)
{
    const yi_console_config_t *cfg;

    if((dev == NULL) || (dev->config == NULL))
    {
        return -1;
    }
    cfg = (const yi_console_config_t *)dev->config;
    if((cfg->backend->api == NULL) || (cfg->backend->api->open == NULL))
    {
        return 0;
    }
    return cfg->backend->api->open(cfg->backend);
}

/**
 * @brief Perform the yi console close operation.
 * @param dev Device instance.
 */
static int yi_console_close(yi_device_t *dev)
{
    const yi_console_config_t *cfg;

    if((dev == NULL) || (dev->config == NULL))
    {
        return -1;
    }
    cfg = (const yi_console_config_t *)dev->config;
    if((cfg->backend->api == NULL) || (cfg->backend->api->close == NULL))
    {
        return 0;
    }
    return cfg->backend->api->close(cfg->backend);
}

/**
 * @brief Write the module.
 * @param console Console value.
 * @param buf Buf value.
 * @param len Len value.
 */
int yi_console_write(yi_device_t *console, const uint8_t *buf, uint32_t len)
{
    const yi_console_config_t *cfg;

    if(!yi_device_is_ready(console) || (console->config == NULL) ||
       (buf == NULL) || (len == 0U))
    {
        return -1;
    }
    cfg = (const yi_console_config_t *)console->config;
    if(!yi_device_is_ready(cfg->backend) || (cfg->backend->api == NULL) ||
       (cfg->backend->api->write == NULL))
    {
        return -1;
    }
    return cfg->backend->api->write(cfg->backend, buf, len);
}

/**
 * @brief Read the module.
 * @param dev Device instance.
 * @param buf Buf value.
 * @param len Len value.
 */
static int yi_console_read(yi_device_t *dev, uint8_t *buf, uint32_t len)
{
    const yi_console_config_t *cfg;

    if(!yi_device_is_ready(dev) || (dev->config == NULL) ||
       (buf == NULL) || (len == 0U))
    {
        return -1;
    }
    cfg = (const yi_console_config_t *)dev->config;
    if(!yi_device_is_ready(cfg->backend) || (cfg->backend->api == NULL) ||
       (cfg->backend->api->read == NULL))
    {
        return -1;
    }
    return cfg->backend->api->read(cfg->backend, buf, len);
}

const yi_device_api_t yi_console_driver_api =
{
    .open = yi_console_open,
    .close = yi_console_close,
    .write = yi_console_write,
    .read = yi_console_read
};
