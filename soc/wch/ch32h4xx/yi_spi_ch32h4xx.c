/**
 * @file yi_spi_ch32h4xx.c
 * @brief Implement polling hardware SPI for CH32H417 through YiCore.
 * @author Don
 * @date 2026-08-03
 * @version 1.0.0
 */
#include "yi_spi_ch32h4xx.h"
#include "system_ch32h417.h"

#include <stddef.h>

/** Translate a GPIO port into its HB2 clock-enable mask. */
static uint32_t yi_spi_gpio_clock(const GPIO_TypeDef *port)
{
    if(port == GPIOA) { return RCC_HB2Periph_GPIOA; }
    if(port == GPIOB) { return RCC_HB2Periph_GPIOB; }
    if(port == GPIOC) { return RCC_HB2Periph_GPIOC; }
    if(port == GPIOD) { return RCC_HB2Periph_GPIOD; }
    if(port == GPIOE) { return RCC_HB2Periph_GPIOE; }
    return 0U;
}

/** Return the peripheral clock mask for a supported SPI instance. */
static uint32_t yi_spi_clock(const SPI_TypeDef *instance)
{
    if(instance == SPI2) { return RCC_HB1Periph_SPI2; }
    if(instance == SPI3) { return RCC_HB1Periph_SPI3; }
    if(instance == SPI4) { return RCC_HB1Periph_SPI4; }
    return 0U;
}

/** Select a power-of-two divider that does not exceed the requested rate. */
static int yi_spi_prescaler(uint32_t requested_hz, uint16_t *value,
                            uint32_t *active_hz)
{
    /** Candidate hardware divider beginning with the minimum divider. */
    uint16_t divider = 2U;
    /** Baud-rate field encoding associated with the candidate divider. */
    uint16_t mode = 0U;

    while((mode < 7U) && ((HCLKClock / divider) > requested_hz)) {
        divider <<= 1U;
        ++mode;
    }
    *value = (uint16_t)(mode << 3U);
    *active_hz = HCLKClock / divider;
    return 0;
}

/** Apply one YiCore transaction configuration to the WCH peripheral. */
static int yi_spi_ch32h4xx_configure(yi_device_t *dev,
                                     const yi_spi_transfer_config_t *config)
{
    const yi_spi_ch32h4xx_config_t *cfg;
    yi_spi_ch32h4xx_data_t *data;
    SPI_InitTypeDef vendor = {0};
    uint16_t prescaler;
    uint32_t active_hz;

    if((dev == NULL) || (dev->config == NULL) || (dev->data == NULL) ||
       (config == NULL)) { return -1; }
    cfg = (const yi_spi_ch32h4xx_config_t *)dev->config;
    data = (yi_spi_ch32h4xx_data_t *)dev->data;
    if((config->frequency > cfg->max_frequency) ||
       (yi_spi_prescaler(config->frequency, &prescaler, &active_hz) != 0)) {
        return -1;
    }
    SPI_Cmd(cfg->instance, DISABLE);
    vendor.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    vendor.SPI_Mode = SPI_Mode_Master;
    vendor.SPI_DataSize = SPI_DataSize_8b;
    vendor.SPI_CPOL = ((config->mode & 2U) != 0U) ? SPI_CPOL_High : SPI_CPOL_Low;
    vendor.SPI_CPHA = ((config->mode & 1U) != 0U) ? SPI_CPHA_2Edge : SPI_CPHA_1Edge;
    vendor.SPI_NSS = SPI_NSS_Soft;
    vendor.SPI_BaudRatePrescaler = prescaler;
    vendor.SPI_FirstBit = config->lsb_first ? SPI_FirstBit_LSB : SPI_FirstBit_MSB;
    vendor.SPI_CRCPolynomial = 7U;
    SPI_Init(cfg->instance, &vendor);
    SPI_NSSInternalSoftwareConfig(cfg->instance, SPI_NSSInternalSoft_Set);
    SPI_Cmd(cfg->instance, ENABLE);
    data->frequency = active_hz;
    data->mode = config->mode;
    data->lsb_first = config->lsb_first;
    return 0;
}

/** Execute one full-duplex polling transfer with a millisecond timeout. */
static int yi_spi_ch32h4xx_transceive(yi_device_t *dev,
                                      const yi_spi_transfer_config_t *config,
                                      const uint8_t *tx, uint8_t *rx,
                                      uint16_t length, uint32_t timeout_ms)
{
    const yi_spi_ch32h4xx_config_t *cfg = dev->config;
    yi_spi_ch32h4xx_data_t *data = dev->data;
    uint16_t index;
    uint32_t timeout;

    if((data->mode != config->mode) ||
       (data->lsb_first != config->lsb_first) ||
       (data->frequency > config->frequency) ||
       (data->frequency <= (config->frequency / 2U))) {
        if(yi_spi_ch32h4xx_configure(dev, config) != 0) { return -1; }
    }
    timeout = ((HCLKClock / 1000U) * timeout_ms);
    for(index = 0U; index < length; ++index) {
        while(SPI_I2S_GetFlagStatus(cfg->instance, SPI_I2S_FLAG_TXE) == RESET) {
            if(timeout-- == 0U) { return -1; }
        }
        SPI_I2S_SendData(cfg->instance, (tx != NULL) ? tx[index] : 0xFFU);
        while(SPI_I2S_GetFlagStatus(cfg->instance, SPI_I2S_FLAG_RXNE) == RESET) {
            if(timeout-- == 0U) { return -1; }
        }
        /** Byte received for storage or intentional discard. */
        uint8_t value = (uint8_t)SPI_I2S_ReceiveData(cfg->instance);
        if(rx != NULL) { rx[index] = value; }
    }
    while(SPI_I2S_GetFlagStatus(cfg->instance, SPI_I2S_FLAG_BSY) != RESET) {
        if(timeout-- == 0U) { return -1; }
    }
    return 0;
}

/** Return the active divided clock cached by the CH32H4xx backend. */
static uint32_t yi_spi_ch32h4xx_get_frequency(yi_device_t *dev)
{
    const yi_spi_ch32h4xx_data_t *data = dev->data;
    return (data != NULL) ? data->frequency : 0U;
}

/** Initialize clocks, pin modes, and a mode-zero SPI controller. */
int yi_spi_ch32h4xx_init(const void *config)
{
    const yi_spi_ch32h4xx_config_t *cfg = config;
    GPIO_InitTypeDef gpio = {0};
    uint32_t gpio_clocks;
    uint32_t spi_clock;

    if((cfg == NULL) || (cfg->instance == NULL) ||
       (cfg->sck_port == NULL) || (cfg->miso_port == NULL) ||
       (cfg->mosi_port == NULL) || (cfg->max_frequency == 0U)) { return -1; }
    gpio_clocks = yi_spi_gpio_clock(cfg->sck_port) |
                  yi_spi_gpio_clock(cfg->miso_port) |
                  yi_spi_gpio_clock(cfg->mosi_port);
    spi_clock = yi_spi_clock(cfg->instance);
    if((gpio_clocks == 0U) || (spi_clock == 0U)) { return -1; }
    RCC_HB2PeriphClockCmd(gpio_clocks, ENABLE);
    RCC_HB1PeriphClockCmd(spi_clock, ENABLE);
    gpio.GPIO_Speed = GPIO_Speed_Very_High;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Pin = cfg->sck_pin;
    GPIO_Init(cfg->sck_port, &gpio);
    gpio.GPIO_Pin = cfg->mosi_pin;
    GPIO_Init(cfg->mosi_port, &gpio);
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    gpio.GPIO_Pin = cfg->miso_pin;
    GPIO_Init(cfg->miso_port, &gpio);
    return 0;
}

/** YiCore SPI dispatch table backed by the WCH standard peripheral library. */
const yi_spi_api_t yi_spi_ch32h4xx_api = {
    .configure = yi_spi_ch32h4xx_configure,
    .transceive = yi_spi_ch32h4xx_transceive,
    .get_frequency = yi_spi_ch32h4xx_get_frequency
};
