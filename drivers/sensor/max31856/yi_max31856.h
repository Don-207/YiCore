/**
 * @file yi_max31856.h
 * @brief YiCore max31856 interface.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#ifndef YI_MAX31856_H
#define YI_MAX31856_H

#include "yi_spi.h"

typedef enum { YI_MAX31856_TC_B = 0, YI_MAX31856_TC_E, YI_MAX31856_TC_J,
    YI_MAX31856_TC_K, YI_MAX31856_TC_N, YI_MAX31856_TC_R,
    YI_MAX31856_TC_S, YI_MAX31856_TC_T } yi_max31856_thermocouple_t;

typedef struct {
    yi_device_t *self; /**< Self value. */
    yi_device_t *spi; /**< Spi value. */
    yi_spi_transfer_config_t spi_config; /**< Spi config value. */
    yi_max31856_thermocouple_t thermocouple_type; /**< Thermocouple type value. */
    uint32_t transfer_timeout_ms; /**< Transfer timeout ms value. */
    uint8_t average_samples; /**< Average samples value. */
    uint8_t filter_hz; /**< Filter hz value. */
    uint8_t open_circuit_ms; /**< Open circuit ms value. */} yi_max31856_config_t;

typedef struct { uint8_t initialized; } yi_max31856_data_t;

/**
 * @brief Initialize the module.
 * @param config Device configuration.
 */
int yi_max31856_init(const void *config);
/**
 * @brief Read the module.
 * @param dev Device instance.
 * @param temperature_mc Temperature mc value.
 * @param fault_status Fault status value.
 */
int yi_max31856_read(yi_device_t *dev, int32_t *temperature_mc,
                     uint8_t *fault_status);

#define YI_MAX31856_DEFINE_LEVEL(_name, _level, _priority, _config, _data) \
    YI_DEVICE_DEFINE_WITH_API(_name, _level, _priority, yi_max31856_init,  \
                              &_config, &_data, NULL)
#endif
