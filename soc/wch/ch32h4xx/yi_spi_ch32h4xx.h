/**
 * @file yi_spi_ch32h4xx.h
 * @brief Define the CH32H4xx backend for the YiCore SPI interface.
 * @author Don
 * @date 2026-08-03
 * @version 1.0.0
 */
#ifndef YI_SPI_CH32H4XX_H
#define YI_SPI_CH32H4XX_H

#include "ch32h417.h"
#include "yi_spi.h"

/** Immutable description of one CH32H4xx SPI controller. */
typedef struct {
    SPI_TypeDef *instance; /**< Vendor register block hidden below YiCore. */
    GPIO_TypeDef *sck_port; /**< Clock output GPIO port. */
    GPIO_TypeDef *miso_port; /**< Input GPIO port. */
    GPIO_TypeDef *mosi_port; /**< Output GPIO port. */
    uint16_t sck_pin; /**< Clock output pin mask. */
    uint16_t miso_pin; /**< Input pin mask. */
    uint16_t mosi_pin; /**< Output pin mask. */
    uint32_t max_frequency; /**< Maximum permitted SCLK in hertz. */
} yi_spi_ch32h4xx_config_t;

/** Mutable configuration cached for one CH32H4xx SPI controller. */
typedef struct {
    uint32_t frequency; /**< Actual divided SCLK in hertz. */
    uint8_t mode; /**< Active SPI mode in the range zero through three. */
    bool lsb_first; /**< Active wire bit order. */
} yi_spi_ch32h4xx_data_t;

/** Initialize one board-described CH32H4xx SPI controller. */
int yi_spi_ch32h4xx_init(const void *config);
/** YiCore dispatch table implemented by the CH32H4xx backend. */
extern const yi_spi_api_t yi_spi_ch32h4xx_api;

#endif
