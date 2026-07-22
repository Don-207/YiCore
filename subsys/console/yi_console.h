#ifndef YI_CONSOLE_H
#define YI_CONSOLE_H

#include "yi_device.h"

typedef struct
{
    yi_device_t *self;
    yi_device_t *backend;
    bool default_console;
} yi_console_config_t;

int yi_console_init(const void *config);
int yi_console_write(yi_device_t *console, const uint8_t *buf, uint32_t len);
yi_device_t *yi_console_get_default(void);
int yi_console_set_default(yi_device_t *console);

extern const yi_device_api_t yi_console_driver_api;

#define YI_CONSOLE_DEFINE_LEVEL(_name, _level, _priority, _config) \
    YI_DEVICE_DEFINE_WITH_API(                                  \
        _name, _level, _priority, yi_console_init,               \
        &_config, NULL, &yi_console_driver_api                   \
    )

#endif
