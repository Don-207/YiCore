/** @file yi_ads8688.h @brief YiCore ADS8688 ADC interface. */
#ifndef YI_ADS8688_H
#define YI_ADS8688_H
#include "yi_adc.h"
#include "yi_spi.h"
#define YI_ADS8688_CHANNEL_COUNT 8U
typedef enum { YI_ADS8688_RANGE_PM_10V24 = 0U, YI_ADS8688_RANGE_PM_5V12 = 1U,
    YI_ADS8688_RANGE_PM_2V56 = 2U, YI_ADS8688_RANGE_0_10V24 = 5U,
    YI_ADS8688_RANGE_0_5V12 = 6U } yi_ads8688_range_t;
typedef struct {
    yi_device_t *self; /**< ADC device instance. */
    yi_device_t *spi; /**< SPI bus. */
    yi_spi_transfer_config_t spi_config; /**< SPI configuration. */
    uint32_t transfer_timeout_ms; /**< SPI timeout. */
    uint8_t default_channel; /**< Generic ADC default channel. */
    uint8_t auto_sequence_mask; /**< Automatic scan mask. */
    uint8_t power_down_mask; /**< Channel power-down mask. */
    yi_ads8688_range_t range[YI_ADS8688_CHANNEL_COUNT]; /**< Channel ranges. */
} yi_ads8688_config_t;
typedef struct { uint32_t sample_count; /**< Successful sample count. */
    uint32_t error_count; /**< Transfer error count. */
    uint8_t active_channel; /**< Last manual channel. */
    bool initialized; /**< Initialization state. */
    bool auto_running; /**< Automatic-sequence state. */
} yi_ads8688_data_t;
/** @brief Initialize the ADS8688. */
int yi_ads8688_init(const void *config);
/** @brief Reset the converter. */
int yi_ads8688_reset(yi_device_t *dev);
/** @brief Enter standby. */
int yi_ads8688_standby(yi_device_t *dev);
/** @brief Enter power-down. */
int yi_ads8688_power_down(yi_device_t *dev);
/** @brief Start automatic sequencing. */
int yi_ads8688_start_auto(yi_device_t *dev);
/** @brief Stop automatic sequencing. */
int yi_ads8688_stop_auto(yi_device_t *dev);
/** @brief Read the next automatic result. */
int yi_ads8688_read_auto(yi_device_t *dev, uint16_t *value);
/** @brief Read a program register. */
int yi_ads8688_read_register(yi_device_t *dev, uint8_t address, uint8_t *value);
/** @brief Write a program register. */
int yi_ads8688_write_register(yi_device_t *dev, uint8_t address, uint8_t value);
/** @brief Set one channel range. */
int yi_ads8688_set_range(yi_device_t *dev, uint8_t channel, yi_ads8688_range_t range);
extern const yi_adc_api_t yi_ads8688_api;
#define YI_ADS8688_DEFINE_LEVEL(_name, _level, _priority, _config, _data) \
 YI_DEVICE_DEFINE_WITH_API(_name, _level, _priority, yi_ads8688_init, &_config, \
 &_data, (const yi_device_api_t *)&yi_ads8688_api)
#endif
