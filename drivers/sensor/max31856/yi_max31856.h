#ifndef YI_MAX31856_H
#define YI_MAX31856_H

#include "yi_spi.h"

typedef enum { YI_MAX31856_TC_B = 0, YI_MAX31856_TC_E, YI_MAX31856_TC_J,
    YI_MAX31856_TC_K, YI_MAX31856_TC_N, YI_MAX31856_TC_R,
    YI_MAX31856_TC_S, YI_MAX31856_TC_T } yi_max31856_thermocouple_t;

typedef struct {
    yi_device_t *self;
    yi_device_t *spi;
    yi_spi_transfer_config_t spi_config;
    yi_max31856_thermocouple_t thermocouple_type;
    uint32_t transfer_timeout_ms;
    uint8_t average_samples;
    uint8_t filter_hz;
    uint8_t open_circuit_ms;
} yi_max31856_config_t;

typedef struct { uint8_t initialized; } yi_max31856_data_t;

int yi_max31856_init(const void *config);
int yi_max31856_read(yi_device_t *dev, int32_t *temperature_mc,
                     uint8_t *fault_status);

#define YI_MAX31856_DEFINE_LEVEL(_name, _level, _priority, _config, _data) \
    YI_DEVICE_DEFINE_WITH_API(_name, _level, _priority, yi_max31856_init,  \
                              &_config, &_data, NULL)
#endif
