#include "yi_tsys01.h"
#include "yi_system.h"

#define TSYS01_CMD_ADC_READ      0x00U
#define TSYS01_CMD_RESET         0x1EU
#define TSYS01_CMD_CONVERT_TEMP  0x48U
#define TSYS01_CMD_PROM_READ     0xA0U

#define TSYS01_PROM_K4_INDEX     1U
#define TSYS01_PROM_K3_INDEX     2U
#define TSYS01_PROM_K2_INDEX     3U
#define TSYS01_PROM_K1_INDEX     4U
#define TSYS01_PROM_K0_INDEX     5U

static int yi_tsys01_command(const yi_tsys01_config_t *cfg, uint8_t command)
{
    return yi_i2c_master_write(cfg->i2c, cfg->address, &command, 1U,
                               cfg->transfer_timeout_ms);
}

static int yi_tsys01_read_prom_word(const yi_tsys01_config_t *cfg,
                                    uint8_t index, uint16_t *word)
{
    uint8_t command = (uint8_t)(TSYS01_CMD_PROM_READ + (index * 2U));
    uint8_t buffer[2] = {0U, 0U};

    if(yi_i2c_master_write_read(cfg->i2c, cfg->address, &command, 1U,
                                buffer, sizeof(buffer),
                                cfg->transfer_timeout_ms) != 0)
    {
        return -1;
    }

    *word = (uint16_t)(((uint16_t)buffer[0] << 8U) | buffer[1]);
    return 0;
}

static bool yi_tsys01_prom_checksum_valid(const uint16_t prom[])
{
    uint8_t sum = 0U;

    for(uint8_t index = 0U; index < YI_TSYS01_PROM_WORD_COUNT; index++)
    {
        sum = (uint8_t)(sum + (uint8_t)(prom[index] >> 8U));
        sum = (uint8_t)(sum + (uint8_t)prom[index]);
    }

    return sum == 0U;
}

static bool yi_tsys01_coefficients_present(const uint16_t prom[])
{
    return (prom[TSYS01_PROM_K4_INDEX] != 0U) ||
           (prom[TSYS01_PROM_K3_INDEX] != 0U) ||
           (prom[TSYS01_PROM_K2_INDEX] != 0U) ||
           (prom[TSYS01_PROM_K1_INDEX] != 0U) ||
           (prom[TSYS01_PROM_K0_INDEX] != 0U);
}

static int32_t yi_tsys01_temperature_mc(uint32_t adc_raw,
                                        const uint16_t prom[])
{
    double adc = (double)adc_raw / 256.0;
    double temperature_c =
        ((((-2.0e-21 * (double)prom[TSYS01_PROM_K4_INDEX] * adc) +
           (4.0e-16 * (double)prom[TSYS01_PROM_K3_INDEX])) * adc -
          (2.0e-11 * (double)prom[TSYS01_PROM_K2_INDEX])) * adc +
         (1.0e-6 * (double)prom[TSYS01_PROM_K1_INDEX])) * adc -
        (1.5e-2 * (double)prom[TSYS01_PROM_K0_INDEX]);

    if(temperature_c >= 0.0)
    {
        return (int32_t)((temperature_c * 1000.0) + 0.5);
    }
    return (int32_t)((temperature_c * 1000.0) - 0.5);
}

int yi_tsys01_init(const void *config)
{
    const yi_tsys01_config_t *cfg = config;
    yi_tsys01_data_t *data;

    if((cfg == NULL) || (cfg->self == NULL) || (cfg->self->data == NULL) ||
       !yi_device_is_ready(cfg->i2c) || (cfg->address > 0x7FU) ||
       (cfg->transfer_timeout_ms == 0U) ||
       (cfg->conversion_delay_ms == 0U) ||
       (cfg->reset_delay_ms == 0U))
    {
        return -1;
    }

    data = (yi_tsys01_data_t *)cfg->self->data;
    data->read_count = 0U;
    data->error_count = 0U;
    data->initialized = 0U;

    if(yi_tsys01_command(cfg, TSYS01_CMD_RESET) != 0)
    {
        data->error_count++;
        return -1;
    }
    yi_system_delay_ms(cfg->reset_delay_ms);

    for(uint8_t index = 0U; index < YI_TSYS01_PROM_WORD_COUNT; index++)
    {
        if(yi_tsys01_read_prom_word(cfg, index, &data->prom[index]) != 0)
        {
            data->error_count++;
            return -1;
        }
    }

    if(!yi_tsys01_coefficients_present(data->prom) ||
       (cfg->validate_prom_checksum &&
        !yi_tsys01_prom_checksum_valid(data->prom)))
    {
        data->error_count++;
        return -1;
    }

    data->initialized = 1U;
    return 0;
}

int yi_tsys01_read_raw(yi_device_t *dev, uint32_t *adc_raw)
{
    const yi_tsys01_config_t *cfg;
    yi_tsys01_data_t *data;
    uint8_t command = TSYS01_CMD_ADC_READ;
    uint8_t buffer[3] = {0U, 0U, 0U};

    if(!yi_device_is_ready(dev) || (dev->config == NULL) ||
       (dev->data == NULL) || (adc_raw == NULL) ||
       (((yi_tsys01_data_t *)dev->data)->initialized == 0U))
    {
        return -1;
    }

    cfg = (const yi_tsys01_config_t *)dev->config;
    data = (yi_tsys01_data_t *)dev->data;

    if(yi_tsys01_command(cfg, TSYS01_CMD_CONVERT_TEMP) != 0)
    {
        data->error_count++;
        return -1;
    }
    yi_system_delay_ms(cfg->conversion_delay_ms);

    if(yi_i2c_master_write_read(cfg->i2c, cfg->address, &command, 1U,
                                buffer, sizeof(buffer),
                                cfg->transfer_timeout_ms) != 0)
    {
        data->error_count++;
        return -1;
    }

    *adc_raw = ((uint32_t)buffer[0] << 16U) |
               ((uint32_t)buffer[1] << 8U) |
               buffer[2];
    data->read_count++;
    return 0;
}

int yi_tsys01_read(yi_device_t *dev, int32_t *temperature_mc)
{
    const yi_tsys01_data_t *data;
    uint32_t adc_raw;

    if((temperature_mc == NULL) || (yi_tsys01_read_raw(dev, &adc_raw) != 0))
    {
        return -1;
    }

    data = (const yi_tsys01_data_t *)dev->data;
    *temperature_mc = yi_tsys01_temperature_mc(adc_raw, data->prom);
    return 0;
}
