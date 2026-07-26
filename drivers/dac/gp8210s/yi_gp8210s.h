/** @file yi_gp8210s.h @brief YiCore GP8210S dual DAC interface. */
#ifndef YI_GP8210S_H
#define YI_GP8210S_H
#include "yi_dac.h"
#include "yi_i2c.h"
#define YI_GP8210S_CHANNEL_COUNT 2U
#define YI_GP8210S_MAX_CODE 32767U
typedef enum { YI_GP8210S_RANGE_5V = 0U, YI_GP8210S_RANGE_10V = 1U
} yi_gp8210s_range_t;
typedef struct {
    yi_device_t *self; /**< DAC device instance. */
    yi_device_t *i2c; /**< I2C bus. */
    uint8_t address; /**< Seven-bit I2C address. */
    uint8_t default_channel; /**< Generic DAC default channel. */
    yi_gp8210s_range_t range; /**< Output voltage range. */
    uint16_t default_value[2]; /**< Initial channel codes. */
    uint32_t transfer_timeout_ms; /**< I2C timeout. */
} yi_gp8210s_config_t;
typedef struct { uint16_t value[2]; /**< Cached channel codes. */
    uint32_t write_count; /**< Successful write count. */
    uint32_t error_count; /**< Transfer error count. */
} yi_gp8210s_data_t;
/** @brief Initialize the GP8210S. */
int yi_gp8210s_init(const void *config);
/** @brief Select output range. */
int yi_gp8210s_set_range(yi_device_t *dev, yi_gp8210s_range_t range);
/** @brief Write one channel code. */
int yi_gp8210s_write_channel(yi_device_t *dev, uint8_t channel, uint16_t value);
/** @brief Write one channel in millivolts. */
int yi_gp8210s_write_channel_mv(yi_device_t *dev, uint8_t channel, uint16_t mv);
/** @brief Update both channels. */
int yi_gp8210s_write_all(yi_device_t *dev, uint16_t channel0, uint16_t channel1);
extern const yi_dac_api_t yi_gp8210s_api;
#define YI_GP8210S_DEFINE_LEVEL(_name, _level, _priority, _config, _data) \
 YI_DEVICE_DEFINE_WITH_API(_name, _level, _priority, yi_gp8210s_init, &_config, \
 &_data, (const yi_device_api_t *)&yi_gp8210s_api)
#endif
