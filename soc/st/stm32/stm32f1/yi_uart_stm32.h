/**
 * @file yi_uart_stm32.h
 * @brief YiCore uart stm32 interface.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#ifndef YI_UART_STM32_H
#define YI_UART_STM32_H

#include "yi_uart.h"
#include "yi_stm32_periph.h"
#include "stm32f1xx_hal.h"

typedef struct
{
    yi_device_t *self; /**< Self value. */
    USART_TypeDef *instance; /**< Instance value. */
    yi_stm32_periph_clock_t clock; /**< Clock value. */
    uint32_t baudrate; /**< Baudrate value. */
    yi_device_t *tx_pin; /**< Tx pin value. */
    yi_device_t *rx_pin; /**< Rx pin value. */
    IRQn_Type irqn; /**< Irqn value. */
    uint8_t irq_priority; /**< Irq priority value. */
    DMA_Channel_TypeDef *tx_dma_channel; /**< Tx dma channel value. */
    DMA_Channel_TypeDef *rx_dma_channel; /**< Rx dma channel value. */
    IRQn_Type tx_dma_irqn; /**< Tx dma irqn value. */
    IRQn_Type rx_dma_irqn; /**< Rx dma irqn value. */
    uint8_t dma_irq_priority; /**< Dma irq priority value. */} yi_uart_stm32_config_t;

typedef struct
{
    UART_HandleTypeDef huart; /**< Huart value. */
    DMA_HandleTypeDef hdma_tx; /**< Hdma tx value. */
    DMA_HandleTypeDef hdma_rx; /**< Hdma rx value. */
    uint8_t *rx_dma_buffer; /**< Rx dma buffer value. */
    uint32_t rx_dma_size; /**< Rx dma size value. */
    volatile bool rx_idle_seen; /**< Rx idle seen value. */
    yi_uart_rx_callback_t rx_callback; /**< Rx callback value. */
    void *rx_callback_user_data; /**< Rx callback user data value. */} yi_uart_stm32_data_t;

/**
 * @brief Initialize the module.
 * @param config Device configuration.
 */
int yi_uart_stm32_init(const void *config);
/**
 * @brief Perform the yi uart stm32 irq handler operation.
 * @param dev Device instance.
 */
void yi_uart_stm32_irq_handler(yi_device_t *dev);
/**
 * @brief Perform the yi uart stm32 dma tx irq handler operation.
 * @param dev Device instance.
 */
void yi_uart_stm32_dma_tx_irq_handler(yi_device_t *dev);
/**
 * @brief Perform the yi uart stm32 dma rx irq handler operation.
 * @param dev Device instance.
 */
void yi_uart_stm32_dma_rx_irq_handler(yi_device_t *dev);
extern const yi_uart_api_t yi_uart_stm32_api;

#define YI_UART_STM32_DEFINE_LEVEL(_name, _level, _priority, _config, _data) \
    YI_DEVICE_DEFINE_WITH_API(                                             \
        _name, _level, _priority, yi_uart_stm32_init,                      \
        &_config, &_data, &yi_uart_stm32_api.base                          \
    )

#endif
