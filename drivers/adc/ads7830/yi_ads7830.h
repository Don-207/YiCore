#ifndef YI_ADS7830_H
#define YI_ADS7830_H

#include "yi_adc.h"
#include "yi_i2c.h"

#define YI_ADS7830_CHANNEL_COUNT 8U

typedef struct
{
    yi_device_t *self;
    yi_device_t *i2c;
    uint8_t address;
    uint8_t default_channel;
    bool internal_reference;
    uint16_t reference_mv;
    uint32_t transfer_timeout_ms;
} yi_ads7830_config_t;

typedef struct
{
    uint32_t read_count;
    uint32_t error_count;
} yi_ads7830_data_t;

int yi_ads7830_init(const void *config);
extern const yi_adc_api_t yi_ads7830_api;

#define YI_ADS7830_DEFINE_LEVEL(_name, _level, _priority, _config, _data) \
    YI_DEVICE_DEFINE_WITH_API(                                           \
        _name, _level, _priority, yi_ads7830_init,                       \
        &_config, &_data, (const yi_device_api_t *)&yi_ads7830_api       \
    )

#endif
