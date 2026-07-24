#ifndef YI_UART_H
#define YI_UART_H


#include "yi_device.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    YI_UART_RX_EVENT_IDLE = 0,
    YI_UART_RX_EVENT_DMA_HALF,
    YI_UART_RX_EVENT_DMA_COMPLETE
} yi_uart_rx_event_t;

typedef void (*yi_uart_rx_callback_t)(yi_device_t *dev,
                                      yi_uart_rx_event_t event,
                                      void *user_data);

typedef struct
{
    yi_device_api_t base;
    int (*write_dma)(yi_device_t *dev, const uint8_t *buf, uint32_t len);
    int (*read_dma)(yi_device_t *dev, uint8_t *buf, uint32_t len);
    int (*rx_start_dma)(yi_device_t *dev, uint8_t *buf, uint32_t len);
    uint32_t (*rx_dma_pos)(yi_device_t *dev);
    bool (*rx_idle)(yi_device_t *dev, bool clear);
    int (*rx_set_callback)(yi_device_t *dev,
                           yi_uart_rx_callback_t callback,
                           void *user_data);
} yi_uart_api_t;

/*
 * 阻塞发送/接收。
 * 成功返回0，参数错误、超时或HAL错误返回-1。
 */
extern const yi_device_api_t yi_uart_driver_api;

int yi_uart_write(yi_device_t *dev, const uint8_t *buf, uint32_t len);
int yi_uart_read(yi_device_t *dev, uint8_t *buf, uint32_t len);
int yi_uart_write_dma(yi_device_t *dev, const uint8_t *buf, uint32_t len);
int yi_uart_read_dma(yi_device_t *dev, uint8_t *buf, uint32_t len);
int yi_uart_rx_start_dma(yi_device_t *dev, uint8_t *buf, uint32_t len);
uint32_t yi_uart_rx_dma_pos(yi_device_t *dev);
bool yi_uart_rx_idle(yi_device_t *dev, bool clear);
int yi_uart_rx_set_callback(yi_device_t *dev,
                            yi_uart_rx_callback_t callback,
                            void *user_data);

#ifdef __cplusplus
}
#endif


#endif
