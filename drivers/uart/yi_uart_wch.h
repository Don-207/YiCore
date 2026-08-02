/**
 * @file yi_uart_wch.h
 * @brief CH32H4xx UART device configuration for YiCore.
 * @author Don
 * @date 2026-08-02
 * @version 1.0.0
 */

#ifndef YI_UART_WCH_H
#define YI_UART_WCH_H

#include "ch32h417.h"
#include "yi_gpio.h"
#include "yi_uart.h"

/** Static hardware description for one CH32H4xx USART device. */
typedef struct
{
    yi_device_t *self; /**< Device being initialized. */
    USART_TypeDef *instance; /**< WCH USART register block. */
    GPIO_TypeDef *tx_port; /**< GPIO port carrying target transmit data. */
    GPIO_TypeDef *rx_port; /**< GPIO port receiving target input data. */
    uint16_t tx_pin; /**< Target transmit GPIO mask. */
    uint16_t rx_pin; /**< Target receive GPIO mask. */
    uint8_t tx_pin_source; /**< Target transmit GPIO source index. */
    uint8_t rx_pin_source; /**< Target receive GPIO source index. */
    uint8_t alternate; /**< GPIO alternate-function selector. */
    IRQn_Type irqn; /**< USART interrupt request number. */
    uint8_t irq_priority; /**< Platform interrupt priority. */
    uint32_t baudrate; /**< Line speed in bits per second. */
} yi_uart_wch_config_t;

/** Power-of-two receive queue capacity; one slot is reserved to detect full. */
#define YI_UART_WCH_RX_BUFFER_SIZE (256U)

/** Interrupt-owned receive queue state for one CH32H4xx USART. */
typedef struct
{
    uint8_t rx_buffer[YI_UART_WCH_RX_BUFFER_SIZE]; /**< ISR-to-main byte queue. */
    volatile uint16_t rx_head; /**< Next slot written only by the ISR. */
    volatile uint16_t rx_tail; /**< Next slot consumed only by main context. */
    volatile uint32_t rx_dropped; /**< Bytes discarded while the queue was full. */
} yi_uart_wch_data_t;

/** Initialize a CH32H4xx UART from its generated device configuration. */
int yi_uart_wch_init(const void *config);

/** Move pending hardware RX bytes into the device ring buffer from ISR context. */
void yi_uart_wch_irq_handler(yi_device_t *device);

/** CH32H4xx polling UART API table. */
extern const yi_uart_api_t yi_uart_wch_api;

#define YI_UART_WCH_DEFINE_LEVEL(_name, _level, _priority, _config, _data) \
    YI_DEVICE_DEFINE_WITH_API(                                       \
        _name, _level, _priority, yi_uart_wch_init,                  \
        &_config, &_data, &yi_uart_wch_api.base                      \
    )

#endif
