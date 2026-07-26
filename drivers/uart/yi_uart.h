/**
 * @file yi_uart.h
 * @brief YiCore uart interface.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

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

/**
 * @brief Perform the void operation.
 * @param dev Device instance.
 * @param event Event value.
 * @param user_data User data value.
 */
typedef void (*yi_uart_rx_callback_t)(yi_device_t *dev,
                                      yi_uart_rx_event_t event,
                                      void *user_data);

typedef struct
{
    yi_device_api_t base; /**< Base value. */
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

/**
 * @brief Write the module.
 * @param dev Device instance.
 * @param buf Buf value.
 * @param len Len value.
 */
int yi_uart_write(yi_device_t *dev, const uint8_t *buf, uint32_t len);
/**
 * @brief Read the module.
 * @param dev Device instance.
 * @param buf Buf value.
 * @param len Len value.
 */
int yi_uart_read(yi_device_t *dev, uint8_t *buf, uint32_t len);
/**
 * @brief Write dma.
 * @param dev Device instance.
 * @param buf Buf value.
 * @param len Len value.
 */
int yi_uart_write_dma(yi_device_t *dev, const uint8_t *buf, uint32_t len);
/**
 * @brief Read dma.
 * @param dev Device instance.
 * @param buf Buf value.
 * @param len Len value.
 */
int yi_uart_read_dma(yi_device_t *dev, uint8_t *buf, uint32_t len);
/**
 * @brief Start dma.
 * @param dev Device instance.
 * @param buf Buf value.
 * @param len Len value.
 */
int yi_uart_rx_start_dma(yi_device_t *dev, uint8_t *buf, uint32_t len);
/**
 * @brief Perform the yi uart rx dma pos operation.
 * @param dev Device instance.
 */
uint32_t yi_uart_rx_dma_pos(yi_device_t *dev);
/**
 * @brief Perform the yi uart rx idle operation.
 * @param dev Device instance.
 * @param clear Clear value.
 */
bool yi_uart_rx_idle(yi_device_t *dev, bool clear);
/**
 * @brief Set callback.
 * @param dev Device instance.
 * @param callback Callback registration object.
 * @param user_data User data value.
 */
int yi_uart_rx_set_callback(yi_device_t *dev,
                            yi_uart_rx_callback_t callback,
                            void *user_data);

#ifdef __cplusplus
}
#endif


#endif
