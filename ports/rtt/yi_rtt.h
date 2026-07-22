#ifndef YI_RTT_H
#define YI_RTT_H

#include "yi_device.h"

typedef enum
{
    YI_RTT_MODE_NO_BLOCK_SKIP = 0,
    YI_RTT_MODE_NO_BLOCK_TRIM,
    YI_RTT_MODE_BLOCK
} yi_rtt_mode_t;

typedef struct
{
    uint32_t up_buffer;
    yi_rtt_mode_t mode;
} yi_rtt_config_t;

int yi_rtt_init(const void *config);
extern const yi_device_api_t yi_rtt_driver_api;

#define YI_RTT_DEFINE_LEVEL(_name, _level, _priority, _config) \
    YI_DEVICE_DEFINE_WITH_API(                              \
        _name, _level, _priority, yi_rtt_init,              \
        &_config, NULL, &yi_rtt_driver_api                  \
    )

#endif
