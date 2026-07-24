#ifndef YI_UART_DMA_LWRB_H
#define YI_UART_DMA_LWRB_H

#include "yi_uart.h"
#include "lwrb/lwrb.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    yi_device_t *uart;
    lwrb_t ring;
    uint8_t *dma_buffer;
    uint32_t dma_size;
    uint32_t last_pos;
    uint32_t overrun;
} yi_uart_dma_lwrb_t;

int yi_uart_dma_lwrb_start(yi_uart_dma_lwrb_t *ctx,
                           yi_device_t *uart,
                           uint8_t *dma_buf, uint32_t dma_len,
                           uint8_t *ring_buf, uint32_t ring_len);
void yi_uart_dma_lwrb_detach(yi_uart_dma_lwrb_t *ctx);
uint32_t yi_uart_dma_lwrb_poll(yi_uart_dma_lwrb_t *ctx);
uint32_t yi_uart_dma_lwrb_available(yi_uart_dma_lwrb_t *ctx);
uint32_t yi_uart_dma_lwrb_read(yi_uart_dma_lwrb_t *ctx,
                               uint8_t *buf, uint32_t len);
uint32_t yi_uart_dma_lwrb_overrun(yi_uart_dma_lwrb_t *ctx, bool clear);
bool yi_uart_dma_lwrb_idle(yi_uart_dma_lwrb_t *ctx, bool clear);

#ifdef __cplusplus
}
#endif

#endif
