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
    bool power_down;
    yi_ads1298_gain_t gain;
    yi_ads1298_mux_t mux;
} yi_ads1298_channel_config_t;

typedef struct
{
    uint32_t status;
    int32_t channel[YI_ADS1298_CHANNEL_COUNT];
} yi_ads1298_frame_t;

typedef struct
{
    yi_device_t *self;
    yi_device_t *spi;
    yi_spi_transfer_config_t spi_config;
    yi_device_t *reset_gpio; /* Optional, active low. */
    yi_device_t *start_gpio; /* Optional; command control is used if NULL. */
    yi_device_t *drdy_gpio;  /* Optional, active low. */
    uint32_t master_clock_hz;
    uint32_t transfer_timeout_ms;
    yi_ads1298_data_rate_t data_rate;
    bool high_resolution;
    bool internal_reference;
    bool reference_4v;
    yi_ads1298_channel_config_t channels[YI_ADS1298_CHANNEL_COUNT];
} yi_ads1298_config_t;

typedef struct
{
    uint32_t frame_count;
    uint32_t error_count;
    uint8_t device_id;
    bool initialized;
    bool continuous;
    bool running;
} yi_ads1298_data_t;

int yi_ads1298_init(const void *config);
int yi_ads1298_command(yi_device_t *dev, yi_ads1298_command_t command);
int yi_ads1298_read_registers(yi_device_t *dev, uint8_t address,
                              uint8_t *values, uint8_t count);
int yi_ads1298_write_registers(yi_device_t *dev, uint8_t address,
                               const uint8_t *values, uint8_t count);
int yi_ads1298_start(yi_device_t *dev);
int yi_ads1298_stop(yi_device_t *dev);
int yi_ads1298_set_continuous(yi_device_t *dev, bool enable);
int yi_ads1298_data_ready(yi_device_t *dev, bool *ready);
int yi_ads1298_read_frame(yi_device_t *dev, yi_ads1298_frame_t *frame);

#define YI_ADS1298_DEFINE_LEVEL(_name, _level, _priority, _config, _data) \
    YI_DEVICE_DEFINE_WITH_API(_name, _level, _priority, yi_ads1298_init,  \
                              &_config, &_data, NULL)

#endif
