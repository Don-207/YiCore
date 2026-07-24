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
} yi_uart_stm32_config_t;

typedef struct
{
    UART_HandleTypeDef huart;
} yi_uart_stm32_data_t;

int yi_uart_stm32_init(const void *config);
void yi_uart_stm32_irq_handler(yi_device_t *dev);

#define YI_UART_STM32_DEFINE_LEVEL(_name, _level, _priority, _config, _data) \
    YI_DEVICE_DEFINE_WITH_API(                                             \
        _name, _level, _priority, yi_uart_stm32_init,                      \
        &_config, &_data, &yi_uart_driver_api                              \
    )

#endif
