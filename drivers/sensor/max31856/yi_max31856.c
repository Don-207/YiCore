/**
 * @file yi_max31856.c
 * @brief YiCore max31856 implementation.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#include "yi_max31856.h"

#define MAX31856_REG_CR0_WRITE   0x80U
#define MAX31856_REG_LTCBH       0x0CU
#define MAX31856_CR0_AUTOCONVERT (1U << 7)

/**
 * @brief Perform the max31856 average bits operation.
 * @param samples Samples value.
 * @param bits Bits value.
 */
static int max31856_average_bits(uint8_t samples, uint8_t *bits)
{
    switch(samples) {
    case 1U: *bits = 0U; break; case 2U: *bits = 1U; break;
    case 4U: *bits = 2U; break; case 8U: *bits = 3U; break;
    case 16U: *bits = 4U; break; default: return -1;
    }
    return 0;
}

/**
 * @brief Perform the max31856 open bits operation.
 * @param milliseconds Milliseconds value.
 * @param bits Bits value.
 */
static int max31856_open_bits(uint8_t milliseconds, uint8_t *bits)
{
    switch(milliseconds) {
    case 0U: *bits = 0U; break; case 10U: *bits = 1U; break;
    case 32U: *bits = 2U; break; case 100U: *bits = 3U; break;
    default: return -1;
    }
    return 0;
}

/**
 * @brief Initialize the module.
 * @param config Device configuration.
 */
int yi_max31856_init(const void *config)
{
    const yi_max31856_config_t *cfg = config;
    yi_max31856_data_t *data;
    uint8_t average, open_circuit, cr0, cr1;
    uint8_t tx[3];

    if((cfg == NULL) || (cfg->self == NULL) || (cfg->self->data == NULL) ||
       !yi_device_is_ready(cfg->spi) || !yi_device_is_ready(cfg->spi_config.cs_gpio) ||
       (cfg->spi_config.mode != 1U) || (cfg->spi_config.frequency == 0U) ||
       (cfg->transfer_timeout_ms == 0U) ||
       (cfg->thermocouple_type > YI_MAX31856_TC_T) ||
       ((cfg->filter_hz != 50U) && (cfg->filter_hz != 60U)) ||
       (max31856_average_bits(cfg->average_samples, &average) != 0) ||
       (max31856_open_bits(cfg->open_circuit_ms, &open_circuit) != 0))
    { return -1; }

    cr0 = MAX31856_CR0_AUTOCONVERT | (uint8_t)(open_circuit << 4);
    if(cfg->filter_hz == 50U) { cr0 |= 1U; }
    cr1 = (uint8_t)((average << 4) | cfg->thermocouple_type);
    tx[0] = MAX31856_REG_CR0_WRITE; tx[1] = cr0; tx[2] = cr1;
    if(yi_spi_transceive(cfg->spi, &cfg->spi_config, tx, NULL, sizeof(tx),
                         cfg->transfer_timeout_ms) != 0) { return -1; }
    data = (yi_max31856_data_t *)cfg->self->data;
    data->initialized = 1U;
    return 0;
}

/**
 * @brief Read the module.
 * @param dev Device instance.
 * @param temperature_mc Temperature mc value.
 * @param fault_status Fault status value.
 */
int yi_max31856_read(yi_device_t *dev, int32_t *temperature_mc,
                     uint8_t *fault_status)
{
    const yi_max31856_config_t *cfg;
    uint8_t tx[5] = {MAX31856_REG_LTCBH, 0U, 0U, 0U, 0U};
    uint8_t rx[5] = {0U};
    int32_t raw;

    if(!yi_device_is_ready(dev) || (dev->config == NULL) ||
       (dev->data == NULL) || (temperature_mc == NULL) ||
       (((const yi_max31856_data_t *)dev->data)->initialized == 0U))
    { return -1; }
    cfg = (const yi_max31856_config_t *)dev->config;
    if(yi_spi_transceive(cfg->spi, &cfg->spi_config, tx, rx, sizeof(tx),
                         cfg->transfer_timeout_ms) != 0) { return -1; }
    raw = ((int32_t)rx[1] << 16) | ((int32_t)rx[2] << 8) | rx[3];
    if((raw & 0x00800000L) != 0L) { raw |= (int32_t)0xFF000000L; }
    *temperature_mc = (raw * 125L) / 512L;
    if(fault_status != NULL) { *fault_status = rx[4]; }
    return 0;
}
