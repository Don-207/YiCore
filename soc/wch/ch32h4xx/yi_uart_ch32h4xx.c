/**
 * @file yi_uart_ch32h4xx.c
 * @brief Implement polling YiCore UART operation for CH32H417.
 * @author Don
 * @date 2026-08-02
 * @version 1.0.0
 */

#include "yi_uart_wch.h"

/** Return the HB2 clock mask for a supported GPIO port. */
static uint32_t yi_uart_wch_gpio_clock(const GPIO_TypeDef *port)
{
    if(port == GPIOA) { return RCC_HB2Periph_GPIOA; }
    if(port == GPIOB) { return RCC_HB2Periph_GPIOB; }
    if(port == GPIOC) { return RCC_HB2Periph_GPIOC; }
    if(port == GPIOD) { return RCC_HB2Periph_GPIOD; }
    if(port == GPIOE) { return RCC_HB2Periph_GPIOE; }
    return 0U;
}

/** Initialize USART1 at 8 data bits, no parity and one stop bit. */
int yi_uart_wch_init(const void *config)
{
    const yi_uart_wch_config_t *uart_config = config;
    GPIO_InitTypeDef gpio_config = {0};
    USART_InitTypeDef usart_config = {0};
    uint32_t gpio_clocks;
    yi_uart_wch_data_t *uart_data;

    if((uart_config == NULL) || (uart_config->self == NULL) ||
       (uart_config->self->data == NULL) ||
       (uart_config->instance != USART1) ||
       (uart_config->tx_port == NULL) || (uart_config->rx_port == NULL) ||
       (uart_config->tx_pin == 0U) || (uart_config->rx_pin == 0U) ||
       (uart_config->baudrate == 0U))
    {
        return -1;
    }
    gpio_clocks = yi_uart_wch_gpio_clock(uart_config->tx_port) |
                  yi_uart_wch_gpio_clock(uart_config->rx_port);
    if(gpio_clocks == 0U)
    {
        return -1;
    }
    RCC_HB2PeriphClockCmd(RCC_HB2Periph_AFIO | RCC_HB2Periph_USART1 |
                          gpio_clocks, ENABLE);
    GPIO_PinAFConfig(uart_config->tx_port, uart_config->tx_pin_source,
                     uart_config->alternate);
    GPIO_PinAFConfig(uart_config->rx_port, uart_config->rx_pin_source,
                     uart_config->alternate);

    gpio_config.GPIO_Pin = uart_config->tx_pin;
    gpio_config.GPIO_Speed = GPIO_Speed_Very_High;
    gpio_config.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(uart_config->tx_port, &gpio_config);
    gpio_config.GPIO_Pin = uart_config->rx_pin;
    gpio_config.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(uart_config->rx_port, &gpio_config);

    usart_config.USART_BaudRate = uart_config->baudrate;
    usart_config.USART_WordLength = USART_WordLength_8b;
    usart_config.USART_StopBits = USART_StopBits_1;
    usart_config.USART_Parity = USART_Parity_No;
    usart_config.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart_config.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(uart_config->instance, &usart_config);
    uart_data = uart_config->self->data;
    uart_data->rx_head = 0U;
    uart_data->rx_tail = 0U;
    uart_data->rx_dropped = 0U;
    NVIC_SetPriority(uart_config->irqn, uart_config->irq_priority);
    USART_ITConfig(uart_config->instance, USART_IT_RXNE, ENABLE);
    NVIC_EnableIRQ(uart_config->irqn);
    USART_Cmd(uart_config->instance, ENABLE);
    return 0;
}

/** Re-enable an initialized UART; safe in main context only. */
static int yi_uart_wch_open(yi_device_t *device)
{
    const yi_uart_wch_config_t *config;

    if((device == NULL) || (device->config == NULL))
    {
        return -1;
    }
    config = device->config;
    USART_ITConfig(config->instance, USART_IT_RXNE, ENABLE);
    NVIC_EnableIRQ(config->irqn);
    USART_Cmd(config->instance, ENABLE);
    return 0;
}

/** Disable an initialized UART; safe in main context only. */
static int yi_uart_wch_close(yi_device_t *device)
{
    const yi_uart_wch_config_t *config;

    if((device == NULL) || (device->config == NULL))
    {
        return -1;
    }
    config = device->config;
    USART_ITConfig(config->instance, USART_IT_RXNE, DISABLE);
    NVIC_DisableIRQ(config->irqn);
    USART_Cmd(config->instance, DISABLE);
    return 0;
}

/** Send all requested bytes by polling the transmit-data-empty flag. */
static int yi_uart_wch_write(yi_device_t *device, const uint8_t *buffer,
                             uint32_t length)
{
    const yi_uart_wch_config_t *config;
    uint32_t index;

    if(!yi_device_is_ready(device) || (device->config == NULL) ||
       (buffer == NULL) || (length == 0U))
    {
        return -1;
    }
    config = device->config;
    for(index = 0U; index < length; ++index)
    {
        while(USART_GetFlagStatus(config->instance, USART_FLAG_TXE) == RESET) { }
        USART_SendData(config->instance, buffer[index]);
    }
    while(USART_GetFlagStatus(config->instance, USART_FLAG_TC) == RESET) { }
    return 0;
}

/** Read available bytes without waiting; return the byte count, or -1 on error. */
static int yi_uart_wch_read(yi_device_t *device, uint8_t *buffer,
                            uint32_t length)
{
    yi_uart_wch_data_t *uart_data;
    uint32_t count = 0U;

    if(!yi_device_is_ready(device) || (device->config == NULL) ||
       (buffer == NULL) || (length == 0U))
    {
        return -1;
    }
    uart_data = device->data;
    while((count < length) && (uart_data->rx_tail != uart_data->rx_head))
    {
        buffer[count] = uart_data->rx_buffer[uart_data->rx_tail];
        uart_data->rx_tail = (uint16_t)((uart_data->rx_tail + 1U) &
                                       (YI_UART_WCH_RX_BUFFER_SIZE - 1U));
        ++count;
    }
    return (int)count;
}

/** Drain USART RXNE into the single-producer ring; no application work runs here. */
void yi_uart_wch_irq_handler(yi_device_t *device)
{
    const yi_uart_wch_config_t *config;
    yi_uart_wch_data_t *uart_data;

    if((device == NULL) || (device->config == NULL) || (device->data == NULL))
    {
        return;
    }
    config = device->config;
    uart_data = device->data;
    while(USART_GetITStatus(config->instance, USART_IT_RXNE) != RESET)
    {
        /** Byte removed from DATAR, which also clears RXNE. */
        uint8_t received_byte = (uint8_t)USART_ReceiveData(config->instance);
        /** Candidate producer position used to detect a full ring. */
        uint16_t next_head = (uint16_t)((uart_data->rx_head + 1U) &
                                       (YI_UART_WCH_RX_BUFFER_SIZE - 1U));

        if(next_head != uart_data->rx_tail)
        {
            uart_data->rx_buffer[uart_data->rx_head] = received_byte;
            uart_data->rx_head = next_head;
        }
        else
        {
            ++uart_data->rx_dropped;
        }
    }
}

/** Polling API: DMA and callbacks are deliberately unavailable. */
const yi_uart_api_t yi_uart_wch_api =
{
    .base =
    {
        .open = yi_uart_wch_open,
        .close = yi_uart_wch_close,
        .write = yi_uart_wch_write,
        .read = yi_uart_wch_read
    },
    .write_dma = NULL,
    .read_dma = NULL,
    .rx_start_dma = NULL,
    .rx_dma_pos = NULL,
    .rx_idle = NULL,
    .rx_set_callback = NULL
};

/** Dispatch a device write through its selected UART backend. */
int yi_uart_write(yi_device_t *device, const uint8_t *buffer, uint32_t length)
{
    return ((device != NULL) && (device->api != NULL) &&
            (device->api->write != NULL))
        ? device->api->write(device, buffer, length) : -1;
}

/** Dispatch a non-blocking read through its selected UART backend. */
int yi_uart_read(yi_device_t *device, uint8_t *buffer, uint32_t length)
{
    return ((device != NULL) && (device->api != NULL) &&
            (device->api->read != NULL))
        ? device->api->read(device, buffer, length) : -1;
}

/** Report unsupported DMA transmit on the polling backend. */
int yi_uart_write_dma(yi_device_t *device, const uint8_t *buffer, uint32_t length)
{
    (void)device; (void)buffer; (void)length;
    return -1;
}

/** Report unsupported DMA receive on the polling backend. */
int yi_uart_read_dma(yi_device_t *device, uint8_t *buffer, uint32_t length)
{
    (void)device; (void)buffer; (void)length;
    return -1;
}

/** Report unsupported circular DMA receive on the polling backend. */
int yi_uart_rx_start_dma(yi_device_t *device, uint8_t *buffer, uint32_t length)
{
    (void)device; (void)buffer; (void)length;
    return -1;
}

/** Return zero because the polling backend has no DMA cursor. */
uint32_t yi_uart_rx_dma_pos(yi_device_t *device)
{
    (void)device;
    return 0U;
}

/** Return false because the polling backend has no idle interrupt state. */
bool yi_uart_rx_idle(yi_device_t *device, bool clear)
{
    (void)device; (void)clear;
    return false;
}

/** Report unsupported callbacks on the polling backend. */
int yi_uart_rx_set_callback(yi_device_t *device,
                            yi_uart_rx_callback_t callback,
                            void *user_data)
{
    (void)device; (void)callback; (void)user_data;
    return -1;
}
