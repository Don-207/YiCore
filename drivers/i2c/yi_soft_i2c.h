#ifndef YI_SOFT_I2C_H
#define YI_SOFT_I2C_H

#include "yi_i2c.h"

typedef struct
{
    yi_device_t *scl_gpio;
    yi_device_t *sda_gpio;
    uint32_t clock_frequency;
    uint32_t half_period_us;
    uint32_t stretch_timeout_us;
    uint8_t recovery_clocks;
} yi_soft_i2c_config_t;

typedef struct
{
    uint32_t transfer_count;
    uint32_t error_count;
} yi_soft_i2c_data_t;

int yi_soft_i2c_init(const void *config);
extern const yi_i2c_api_t yi_soft_i2c_api;

#define YI_SOFT_I2C_DEFINE_LEVEL(_name, _level, _priority, _config, _data) \
    YI_DEVICE_DEFINE_WITH_API(_name, _level, _priority,                  \
                              yi_soft_i2c_init, &_config, &_data,        \
                              (const yi_device_api_t *)&yi_soft_i2c_api)

#endif
