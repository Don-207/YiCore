/**
 * @file yi_ad9834.h
 * @brief YiCore AD9834 DDS interface.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#ifndef YI_AD9834_H
#define YI_AD9834_H

#include "yi_spi.h"

#define YI_AD9834_FREQUENCY_REGISTERS 2U
#define YI_AD9834_PHASE_REGISTERS     2U

typedef enum {
    YI_AD9834_WAVE_SINE = 0, /**< Sine output. */
    YI_AD9834_WAVE_TRIANGLE, /**< Triangle output. */
    YI_AD9834_WAVE_SQUARE, /**< MSB square-wave output. */
    YI_AD9834_WAVE_SQUARE_DIV2 /**< Divided square-wave output. */
} yi_ad9834_waveform_t;

typedef struct {
    yi_device_t *self; /**< Self value. */
    yi_device_t *spi; /**< SPI bus. */
    yi_spi_transfer_config_t spi_config; /**< SPI transfer configuration. */
    uint32_t mclk_frequency; /**< Master clock frequency in hertz. */
    uint32_t transfer_timeout_ms; /**< Transfer timeout in milliseconds. */
} yi_ad9834_config_t;

typedef struct {
    uint16_t control; /**< Cached control register. */
    uint8_t initialized; /**< Initialization state. */
} yi_ad9834_data_t;

/** @brief Initialize the AD9834. */
int yi_ad9834_init(const void *config);
/** @brief Program a frequency register. */
int yi_ad9834_set_frequency(yi_device_t *dev, uint8_t reg, uint32_t frequency_hz);
/** @brief Program a phase register. */
int yi_ad9834_set_phase(yi_device_t *dev, uint8_t reg, uint16_t phase_tenths);
/** @brief Select active frequency and phase registers. */
int yi_ad9834_select(yi_device_t *dev, uint8_t frequency_reg, uint8_t phase_reg);
/** @brief Select the output waveform. */
int yi_ad9834_set_waveform(yi_device_t *dev, yi_ad9834_waveform_t waveform);
/** @brief Assert or release reset. */
int yi_ad9834_set_reset(yi_device_t *dev, bool reset);
/** @brief Control clock and DAC sleep states. */
int yi_ad9834_set_sleep(yi_device_t *dev, bool disable_mclk, bool disable_dac);

#define YI_AD9834_DEFINE_LEVEL(_name, _level, _priority, _config, _data) \
    YI_DEVICE_DEFINE_WITH_API(_name, _level, _priority, yi_ad9834_init,  \
                              &_config, &_data, NULL)

#endif
