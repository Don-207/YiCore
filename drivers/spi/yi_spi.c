#include "yi_spi.h"
#include "yi_pinmux.h"

static uint32_t yi_spi_prescaler(uint32_t source, uint32_t maximum)
{
    static const uint16_t divisors[] = {2U, 4U, 8U, 16U, 32U, 64U, 128U, 256U};
    static const uint32_t values[] = {
        SPI_BAUDRATEPRESCALER_2, SPI_BAUDRATEPRESCALER_4,
        SPI_BAUDRATEPRESCALER_8, SPI_BAUDRATEPRESCALER_16,
        SPI_BAUDRATEPRESCALER_32, SPI_BAUDRATEPRESCALER_64,
        SPI_BAUDRATEPRESCALER_128, SPI_BAUDRATEPRESCALER_256
    };
    for(uint32_t i = 0U; i < 8U; i++)
    {
        if((source / divisors[i]) <= maximum) { return values[i]; }
    }
    return 0xFFFFFFFFU;
}

int yi_spi_init(const void *config)
{
    const yi_spi_config_t *cfg = (const yi_spi_config_t *)config;
    yi_spi_data_t *data;
    uint32_t prescaler;

    if((cfg == NULL) || (cfg->self == NULL) || (cfg->self->data == NULL) ||
       !yi_device_is_ready(cfg->sck_pin) || !yi_device_is_ready(cfg->miso_pin) ||
       !yi_device_is_ready(cfg->mosi_pin) || (cfg->max_frequency == 0U) ||
       (cfg->mode > 3U) || (cfg->irq_priority > 15U)) { return -1; }
    prescaler = yi_spi_prescaler(yi_stm32_periph_clock_rate(&cfg->clock),
                                 cfg->max_frequency);
    if((prescaler == 0xFFFFFFFFU) ||
       (yi_stm32_periph_clock_enable(&cfg->clock) != 0) ||
       (yi_pinmux_apply(cfg->sck_pin) != 0) ||
       (yi_pinmux_apply(cfg->miso_pin) != 0) ||
       (yi_pinmux_apply(cfg->mosi_pin) != 0)) { return -1; }

    data = (yi_spi_data_t *)cfg->self->data;
    data->hspi.Instance = cfg->instance;
    data->hspi.Init.Mode = SPI_MODE_MASTER;
    data->hspi.Init.Direction = SPI_DIRECTION_2LINES;
    data->hspi.Init.DataSize = SPI_DATASIZE_8BIT;
    data->hspi.Init.CLKPolarity = (cfg->mode & 2U) ? SPI_POLARITY_HIGH : SPI_POLARITY_LOW;
    data->hspi.Init.CLKPhase = (cfg->mode & 1U) ? SPI_PHASE_2EDGE : SPI_PHASE_1EDGE;
    data->hspi.Init.NSS = SPI_NSS_SOFT;
    data->hspi.Init.BaudRatePrescaler = prescaler;
    data->hspi.Init.FirstBit = SPI_FIRSTBIT_MSB;
    data->hspi.Init.TIMode = SPI_TIMODE_DISABLE;
    data->hspi.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    data->hspi.Init.CRCPolynomial = 7U;
    if(HAL_SPI_Init(&data->hspi) != HAL_OK) { return -1; }
    HAL_NVIC_SetPriority(cfg->irqn, cfg->irq_priority, 0U);
    HAL_NVIC_EnableIRQ(cfg->irqn);
    return 0;
}

int yi_spi_transceive(yi_device_t *dev, const uint8_t *tx, uint8_t *rx,
                      uint16_t length, uint32_t timeout_ms)
{
    yi_spi_data_t *data;
    HAL_StatusTypeDef status;
    if(!yi_device_is_ready(dev) || (dev->data == NULL) || (length == 0U) ||
       ((tx == NULL) && (rx == NULL))) { return -1; }
    data = (yi_spi_data_t *)dev->data;
    if((tx != NULL) && (rx != NULL))
        status = HAL_SPI_TransmitReceive(&data->hspi, (uint8_t *)tx, rx, length, timeout_ms);
    else if(tx != NULL)
        status = HAL_SPI_Transmit(&data->hspi, (uint8_t *)tx, length, timeout_ms);
    else
        status = HAL_SPI_Receive(&data->hspi, rx, length, timeout_ms);
    return (status == HAL_OK) ? 0 : -1;
}

void yi_spi_irq_handler(yi_device_t *dev)
{
    if((dev != NULL) && (dev->data != NULL))
        HAL_SPI_IRQHandler(&((yi_spi_data_t *)dev->data)->hspi);
}
