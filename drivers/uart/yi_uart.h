#ifndef YI_UART_H
#define YI_UART_H


#include "yi_device.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    yi_device_api_t base;
    int (*write_dma)(yi_device_t *dev, const uint8_t *buf, uint32_t len);
    int (*read_dma)(yi_device_t *dev, uint8_t *buf, uint32_t len);
    int (*rx_start_dma)(yi_device_t *dev, uint8_t *buf, uint32_t len);
    uint32_t (*rx_dma_pos)(yi_device_t *dev);
    bool (*rx_idle)(yi_device_t *dev, bool clear);
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

#ifdef __cplusplus
}
#endif


#endif
