#ifndef YI_SPI_H
#define YI_SPI_H

#include "yi_device.h"
#include "yi_stm32_periph.h"

typedef struct
{
    yi_device_t *self;
    SPI_TypeDef *instance;
    yi_stm32_periph_clock_t clock;
    yi_device_t *sck_pin;
    yi_device_t *miso_pin;
    yi_device_t *mosi_pin;
    uint32_t max_frequency;
    uint8_t mode;
    IRQn_Type irqn;
    uint8_t irq_priority;
} yi_spi_config_t;

typedef struct { SPI_HandleTypeDef hspi; } yi_spi_data_t;

typedef struct
{
    int (*transceive)(yi_device_t *dev, const uint8_t *tx, uint8_t *rx,
                      uint16_t length, uint32_t timeout_ms);
} yi_spi_api_t;

int yi_spi_init(const void *config);
int yi_spi_transceive(yi_device_t *dev, const uint8_t *tx, uint8_t *rx,
                      uint16_t length, uint32_t timeout_ms);
void yi_spi_irq_handler(yi_device_t *dev);
extern const yi_spi_api_t yi_spi_api;

#define YI_SPI_DEFINE_LEVEL(_name, _level, _priority, _config, _data) \
    YI_DEVICE_DEFINE_WITH_API(_name, _level, _priority, yi_spi_init,  \
                              &_config, &_data,                       \
                              (const yi_device_api_t *)&yi_spi_api)

#endif
