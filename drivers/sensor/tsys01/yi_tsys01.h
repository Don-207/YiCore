#ifndef YI_TSYS01_H
#define YI_TSYS01_H

#include <stdbool.h>
#include "yi_i2c.h"

#define YI_TSYS01_PROM_WORD_COUNT 8U
#define YI_TSYS01_DEFAULT_ADDRESS 0x77U

typedef struct
{
    yi_device_t *self;
    yi_device_t *i2c;
    uint8_t address;
    uint32_t transfer_timeout_ms;
    uint32_t conversion_delay_ms;
    uint32_t reset_delay_ms;
    bool validate_prom_checksum;
} yi_tsys01_config_t;

typedef struct
{
    uint16_t prom[YI_TSYS01_PROM_WORD_COUNT];
    uint32_t read_count;
    uint32_t error_count;
    uint8_t initialized;
} yi_tsys01_data_t;

int yi_tsys01_init(const void *config);
int yi_tsys01_read_raw(yi_device_t *dev, uint32_t *adc_raw);
int yi_tsys01_read(yi_device_t *dev, int32_t *temperature_mc);

#define YI_TSYS01_DEFINE_LEVEL(_name, _level, _priority, _config, _data) \
    YI_DEVICE_DEFINE_WITH_API(                                          \
        _name, _level, _priority, yi_tsys01_init,                       \
        &_config, &_data, NULL                                          \
    )

#endif
