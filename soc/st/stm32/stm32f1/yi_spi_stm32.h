/**
 * @file yi_spi_stm32.h
 * @brief YiCore spi stm32 interface.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#ifndef YI_SPI_STM32_H
#define YI_SPI_STM32_H

#include "yi_spi.h"
#include "yi_stm32_periph.h"
#include "stm32f1xx_hal.h"

typedef struct
{
    yi_device_t *self; /**< Self value. */
    SPI_TypeDef *instance; /**< Instance value. */
    yi_stm32_periph_clock_t clock; /**< Clock value. */
    yi_device_t *sck_pin; /**< Sck pin value. */
    yi_device_t *miso_pin; /**< Miso pin value. */
    yi_device_t *mosi_pin; /**< Mosi pin value. */
    uint32_t max_frequency; /**< Max frequency value. */
    IRQn_Type irqn; /**< Irqn value. */
    uint8_t irq_priority; /**< Irq priority value. */} yi_spi_stm32_config_t;

typedef struct
{
    SPI_HandleTypeDef hspi; /**< Hspi value. */
    uint32_t frequency; /**< Frequency value. */
    uint8_t mode; /**< Mode value. */} yi_spi_stm32_data_t;

/**
 * @brief Initialize the module.
 * @param config Device configuration.
 */
int yi_spi_stm32_init(const void *config);
/**
 * @brief Perform the yi spi stm32 irq handler operation.
 * @param dev Device instance.
 */
void yi_spi_stm32_irq_handler(yi_device_t *dev);
extern const yi_spi_api_t yi_spi_stm32_api;

#define YI_SPI_STM32_DEFINE_LEVEL(_name, _level, _priority, _config, _data) \
    YI_DEVICE_DEFINE_WITH_API(                                             \
        _name, _level, _priority, yi_spi_stm32_init,                       \
        &_config, &_data, (const yi_device_api_t *)&yi_spi_stm32_api       \
    )

#endif
