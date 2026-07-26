/** @file yi_ads1258.h @brief YiCore ADS1258 ADC interface. */
#ifndef YI_ADS1258_H
#define YI_ADS1258_H
#include "yi_gpio.h"
#include "yi_spi.h"

#define YI_ADS1258_ANALOG_CHANNEL_COUNT 16U
typedef enum { YI_ADS1258_REG_CONFIG0 = 0x00U, YI_ADS1258_REG_CONFIG1,
    YI_ADS1258_REG_MUXSCH, YI_ADS1258_REG_MUXDIF, YI_ADS1258_REG_MUXSG0,
    YI_ADS1258_REG_MUXSG1, YI_ADS1258_REG_SYSRED, YI_ADS1258_REG_GPIOC,
    YI_ADS1258_REG_GPIOD, YI_ADS1258_REG_ID } yi_ads1258_register_t;

typedef struct {
    int32_t value; /**< Sign-extended 24-bit conversion code. */
    uint8_t channel; /**< Status channel identifier. */
    bool new_data; /**< NEW status flag. */
    bool overflow; /**< Overflow status flag. */
    bool supply; /**< Supply-monitor status flag. */
} yi_ads1258_sample_t;

typedef struct {
    yi_device_t *self; /**< ADC device instance. */
    yi_device_t *spi; /**< SPI bus. */
    yi_spi_transfer_config_t spi_config; /**< SPI configuration. */
    yi_device_t *reset_gpio; /**< Optional active-low reset GPIO. */
    yi_device_t *start_gpio; /**< START output GPIO. */
    yi_device_t *drdy_gpio; /**< Optional active-low DRDY GPIO. */
    uint32_t transfer_timeout_ms; /**< SPI timeout. */
    uint8_t config0; /**< CONFIG0 value. */
    uint8_t config1; /**< CONFIG1 value. */
    uint8_t muxsch; /**< Fixed-channel mux value. */
    uint8_t muxdif; /**< Differential scan mask. */
    uint16_t single_ended_mask; /**< Single-ended scan mask. */
    uint8_t system_readings; /**< System-reading scan mask. */
} yi_ads1258_config_t;

typedef struct {
    uint32_t sample_count; /**< Successful sample count. */
    uint32_t error_count; /**< Transfer error count. */
    uint8_t device_id; /**< Device ID register. */
    bool initialized; /**< Initialization state. */
    bool running; /**< START state. */
} yi_ads1258_data_t;

/** @brief Initialize the ADS1258. */
int yi_ads1258_init(const void *config);
/** @brief Reset the converter. */
int yi_ads1258_reset(yi_device_t *dev);
/** @brief Start conversions. */
int yi_ads1258_start(yi_device_t *dev);
/** @brief Stop conversions. */
int yi_ads1258_stop(yi_device_t *dev);
/** @brief Trigger a pulse conversion. */
int yi_ads1258_pulse_convert(yi_device_t *dev);
/** @brief Query DRDY. */
int yi_ads1258_data_ready(yi_device_t *dev, bool *ready);
/** @brief Read one tagged sample. */
int yi_ads1258_read_sample(yi_device_t *dev, yi_ads1258_sample_t *sample);
/** @brief Read a register. */
int yi_ads1258_read_register(yi_device_t *dev, uint8_t address, uint8_t *value);
/** @brief Write a register. */
int yi_ads1258_write_register(yi_device_t *dev, uint8_t address, uint8_t value);

#define YI_ADS1258_DEFINE_LEVEL(_name, _level, _priority, _config, _data) \
    YI_DEVICE_DEFINE_WITH_API(_name, _level, _priority, yi_ads1258_init,  \
                              &_config, &_data, NULL)
#endif
