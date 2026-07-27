/**
 * @file yi_ads1298.h
 * @brief YiCore ads1298 interface.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#ifndef YI_ADS1298_H
#define YI_ADS1298_H

#include <stdbool.h>
#include <stdint.h>

#include "yi_gpio.h"
#include "yi_spi.h"

#define YI_ADS1298_CHANNEL_COUNT 8U
#define YI_ADS1298_FRAME_SIZE    27U
#define YI_ADS1298_REGISTER_COUNT 0x1AU

typedef enum
{
    YI_ADS1298_GAIN_6 = 0U,
    YI_ADS1298_GAIN_1 = 1U,
    YI_ADS1298_GAIN_2 = 2U,
    YI_ADS1298_GAIN_3 = 3U,
    YI_ADS1298_GAIN_4 = 4U,
    YI_ADS1298_GAIN_8 = 5U,
    YI_ADS1298_GAIN_12 = 6U
} yi_ads1298_gain_t;

typedef enum
{
    YI_ADS1298_MUX_NORMAL = 0U,
    YI_ADS1298_MUX_INPUT_SHORT = 1U,
    YI_ADS1298_MUX_RLD_MEASURE = 2U,
    YI_ADS1298_MUX_MVDD = 3U,
    YI_ADS1298_MUX_TEMPERATURE = 4U,
    YI_ADS1298_MUX_TEST_SIGNAL = 5U,
    YI_ADS1298_MUX_RLD_DRP = 6U,
    YI_ADS1298_MUX_RLD_DRN = 7U
} yi_ads1298_mux_t;

/* CONFIG1 DR[2:0]. Sample rates depend on HR mode and fCLK; at 2.048 MHz
 * HR mode these values represent 16k, 8k, 4k, 2k, 1k, 500, and 250 SPS. */
typedef enum
{
    YI_ADS1298_DATA_RATE_0 = 0U,
    YI_ADS1298_DATA_RATE_1 = 1U,
    YI_ADS1298_DATA_RATE_2 = 2U,
    YI_ADS1298_DATA_RATE_3 = 3U,
    YI_ADS1298_DATA_RATE_4 = 4U,
    YI_ADS1298_DATA_RATE_5 = 5U,
    YI_ADS1298_DATA_RATE_6 = 6U
} yi_ads1298_data_rate_t;

typedef enum
{
    YI_ADS1298_REG_ID = 0x00U,
    YI_ADS1298_REG_CONFIG1 = 0x01U,
    YI_ADS1298_REG_CONFIG2 = 0x02U,
    YI_ADS1298_REG_CONFIG3 = 0x03U,
    YI_ADS1298_REG_LOFF = 0x04U,
    YI_ADS1298_REG_CH1SET = 0x05U,
    YI_ADS1298_REG_CH8SET = 0x0CU,
    YI_ADS1298_REG_RLD_SENSP = 0x0DU,
    YI_ADS1298_REG_RLD_SENSN = 0x0EU,
    YI_ADS1298_REG_LOFF_SENSP = 0x0FU,
    YI_ADS1298_REG_LOFF_SENSN = 0x10U,
    YI_ADS1298_REG_LOFF_FLIP = 0x11U,
    YI_ADS1298_REG_LOFF_STATP = 0x12U,
    YI_ADS1298_REG_LOFF_STATN = 0x13U,
    YI_ADS1298_REG_GPIO = 0x14U,
    YI_ADS1298_REG_PACE = 0x15U,
    YI_ADS1298_REG_RESP = 0x16U,
    YI_ADS1298_REG_CONFIG4 = 0x17U,
    YI_ADS1298_REG_WCT1 = 0x18U,
    YI_ADS1298_REG_WCT2 = 0x19U
} yi_ads1298_register_t;

typedef enum
{
    YI_ADS1298_CMD_WAKEUP = 0x02U,
    YI_ADS1298_CMD_STANDBY = 0x04U,
    YI_ADS1298_CMD_RESET = 0x06U,
    YI_ADS1298_CMD_START = 0x08U,
    YI_ADS1298_CMD_STOP = 0x0AU,
    YI_ADS1298_CMD_RDATAC = 0x10U,
    YI_ADS1298_CMD_SDATAC = 0x11U,
    YI_ADS1298_CMD_RDATA = 0x12U
} yi_ads1298_command_t;

typedef struct
{
    bool power_down; /**< Power down value. */
    yi_ads1298_gain_t gain; /**< Gain value. */
    yi_ads1298_mux_t mux; /**< Mux value. */} yi_ads1298_channel_config_t;

typedef struct
{
    uint32_t status; /**< Status value. */
    int32_t channel[YI_ADS1298_CHANNEL_COUNT]; /**< Channel value. */} yi_ads1298_frame_t;

typedef struct
{
    yi_device_t *self; /**< Self value. */
    yi_device_t *spi; /**< Spi value. */
    yi_spi_transfer_config_t spi_config; /**< Spi config value. */
    yi_device_t *enable_gpio; /* Optional board-level power enable. */
    bool enable_active_low; /**< Enable active low value. */
    yi_device_t *reset_gpio; /* Optional, active low. */
    yi_device_t *start_gpio; /* Optional; command control is used if NULL. */
    yi_device_t *drdy_gpio;  /* Optional, active low. */
    uint32_t master_clock_hz; /**< Master clock hz value. */
    uint32_t transfer_timeout_ms; /**< Transfer timeout ms value. */
    yi_ads1298_data_rate_t data_rate; /**< Data rate value. */
    bool high_resolution; /**< High resolution value. */
    bool internal_reference; /**< Internal reference value. */
    bool reference_4v; /**< Reference 4v value. */
    uint8_t config2; /**< CONFIG2 register value. */
    uint8_t config3_extra; /**< Additional CONFIG3 low-bit value. */
    uint8_t loff; /**< LOFF register value. */
    uint8_t rld_sensp; /**< RLD_SENSP register value. */
    uint8_t rld_sensn; /**< RLD_SENSN register value. */
    uint8_t loff_sensp; /**< LOFF_SENSP register value. */
    uint8_t loff_sensn; /**< LOFF_SENSN register value. */
    uint8_t loff_flip; /**< LOFF_FLIP register value. */
    uint8_t gpio; /**< GPIO register value. */
    uint8_t pace; /**< PACE register value. */
    uint8_t resp; /**< RESP register value. */
    uint8_t config4; /**< CONFIG4 register value. */
    uint8_t wct1; /**< WCT1 register value. */
    uint8_t wct2; /**< WCT2 register value. */
    yi_ads1298_channel_config_t channels[YI_ADS1298_CHANNEL_COUNT]; /**< Channels value. */} yi_ads1298_config_t;

typedef struct
{
    uint32_t frame_count; /**< Frame count value. */
    uint32_t error_count; /**< Error count value. */
    uint8_t device_id; /**< Device id value. */
    bool initialized; /**< Initialized value. */
    bool continuous; /**< Continuous value. */
    bool running; /**< Running value. */
    volatile bool data_ready; /**< Latched DRDY falling edge. */
    yi_gpio_callback_t drdy_callback; /**< DRDY GPIO callback. */
} yi_ads1298_data_t;

/**
 * @brief Initialize the module.
 * @param config Device configuration.
 */
int yi_ads1298_init(const void *config);
/**
 * @brief Perform the yi ads1298 command operation.
 * @param dev Device instance.
 * @param command Command value.
 */
int yi_ads1298_command(yi_device_t *dev, yi_ads1298_command_t command);
/**
 * @brief Read registers.
 * @param dev Device instance.
 * @param address Address value.
 * @param values Values value.
 * @param count Count value.
 */
int yi_ads1298_read_registers(yi_device_t *dev, uint8_t address,
                              uint8_t *values, uint8_t count);
/**
 * @brief Write registers.
 * @param dev Device instance.
 * @param address Address value.
 * @param values Values value.
 * @param count Count value.
 */
int yi_ads1298_write_registers(yi_device_t *dev, uint8_t address,
                               const uint8_t *values, uint8_t count);
/**
 * @brief Start the module.
 * @param dev Device instance.
 */
int yi_ads1298_start(yi_device_t *dev);
/**
 * @brief Stop the module.
 * @param dev Device instance.
 */
int yi_ads1298_stop(yi_device_t *dev);
/**
 * @brief Set continuous.
 * @param dev Device instance.
 * @param enable Enable value.
 */
int yi_ads1298_set_continuous(yi_device_t *dev, bool enable);
/**
 * @brief Perform the yi ads1298 data ready operation.
 * @param dev Device instance.
 * @param ready Ready value.
 */
int yi_ads1298_data_ready(yi_device_t *dev, bool *ready);
/**
 * @brief Read frame.
 * @param dev Device instance.
 * @param frame Frame value.
 */
int yi_ads1298_read_frame(yi_device_t *dev, yi_ads1298_frame_t *frame);

#define YI_ADS1298_DEFINE_LEVEL(_name, _level, _priority, _config, _data) \
    YI_DEVICE_DEFINE_WITH_API(_name, _level, _priority, yi_ads1298_init,  \
                              &_config, &_data, NULL)

#endif
