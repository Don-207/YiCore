#ifndef YI_UART_STM32_H
#define YI_UART_STM32_H

#include "yi_uart.h"
#include "yi_stm32_periph.h"
#include "stm32f1xx_hal.h"

typedef struct
{
    yi_device_t *self;
    USART_TypeDef *instance;
    yi_stm32_periph_clock_t clock;
    uint32_t baudrate;
    yi_device_t *tx_pin;
    yi_device_t *rx_pin;
    IRQn_Type irqn;
    uint8_t irq_priority;
    DMA_Channel_TypeDef *tx_dma_channel;
    DMA_Channel_TypeDef *rx_dma_channel;
    IRQn_Type tx_dma_irqn;
    IRQn_Type rx_dma_irqn;
    uint8_t dma_irq_priority;
} yi_uart_stm32_config_t;

typedef struct
{
    UART_HandleTypeDef huart;
    DMA_HandleTypeDef hdma_tx;
    DMA_HandleTypeDef hdma_rx;
    uint8_t *rx_dma_buffer;
    uint32_t rx_dma_size;
    volatile bool rx_idle_seen;
    yi_uart_rx_callback_t rx_callback;
    void *rx_callback_user_data;
} yi_uart_stm32_data_t;

int yi_uart_stm32_init(const void *config);
void yi_uart_stm32_irq_handler(yi_device_t *dev);
void yi_uart_stm32_dma_tx_irq_handler(yi_device_t *dev);
void yi_uart_stm32_dma_rx_irq_handler(yi_device_t *dev);
extern const yi_uart_api_t yi_uart_stm32_api;

#define YI_UART_STM32_DEFINE_LEVEL(_name, _level, _priority, _config, _data) \
    YI_DEVICE_DEFINE_WITH_API(                                             \
        _name, _level, _priority, yi_uart_stm32_init,                      \
        &_config, &_data, &yi_uart_stm32_api.base                          \
    )

#endif
