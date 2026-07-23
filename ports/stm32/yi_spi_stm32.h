#ifndef YI_SPI_STM32_H
#define YI_SPI_STM32_H

#include "yi_spi.h"
#include "yi_stm32_periph.h"
#include "stm32f1xx_hal.h"

typedef struct
{
    yi_device_t *self;
    SPI_TypeDef *instance;
    yi_stm32_periph_clock_t clock;
    yi_device_t *sck_pin;
    yi_device_t *miso_pin;
    yi_device_t *mosi_pin;
    uint32_t max_frequency;
    IRQn_Type irqn;
    uint8_t irq_priority;
} yi_spi_stm32_config_t;

typedef struct
{
    SPI_HandleTypeDef hspi;
    uint32_t frequency;
    uint8_t mode;
} yi_spi_stm32_data_t;

int yi_spi_stm32_init(const void *config);
void yi_spi_stm32_irq_handler(yi_device_t *dev);
extern const yi_spi_api_t yi_spi_stm32_api;

#define YI_SPI_STM32_DEFINE_LEVEL(_name, _level, _priority, _config, _data) \
    YI_DEVICE_DEFINE_WITH_API(                                             \
        _name, _level, _priority, yi_spi_stm32_init,                       \
        &_config, &_data, (const yi_device_api_t *)&yi_spi_stm32_api       \
    )

#endif
