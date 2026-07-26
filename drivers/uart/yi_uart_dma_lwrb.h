/**
 * @file yi_uart_dma_lwrb.h
 * @brief YiCore uart dma lwrb interface.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#ifndef YI_UART_DMA_LWRB_H
#define YI_UART_DMA_LWRB_H

#include "yi_uart.h"
#include "lwrb/lwrb.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    yi_device_t *uart; /**< Uart value. */
    lwrb_t ring; /**< Ring value. */
    uint8_t *dma_buffer; /**< Dma buffer value. */
    uint32_t dma_size; /**< Dma size value. */
    uint32_t last_pos; /**< Last pos value. */
    uint32_t overrun; /**< Overrun value. */} yi_uart_dma_lwrb_t;

/**
 * @brief Start the module.
 * @param ctx Ctx value.
 * @param uart Uart value.
 * @param dma_buf Dma buf value.
 * @param dma_len Dma len value.
 * @param ring_buf Ring buf value.
 * @param ring_len Ring len value.
 */
int yi_uart_dma_lwrb_start(yi_uart_dma_lwrb_t *ctx,
                           yi_device_t *uart,
                           uint8_t *dma_buf, uint32_t dma_len,
                           uint8_t *ring_buf, uint32_t ring_len);
/**
 * @brief Perform the yi uart dma lwrb detach operation.
 * @param ctx Ctx value.
 */
void yi_uart_dma_lwrb_detach(yi_uart_dma_lwrb_t *ctx);
/**
 * @brief Perform the yi uart dma lwrb poll operation.
 * @param ctx Ctx value.
 */
uint32_t yi_uart_dma_lwrb_poll(yi_uart_dma_lwrb_t *ctx);
/**
 * @brief Perform the yi uart dma lwrb available operation.
 * @param ctx Ctx value.
 */
uint32_t yi_uart_dma_lwrb_available(yi_uart_dma_lwrb_t *ctx);
/**
 * @brief Read the module.
 * @param ctx Ctx value.
 * @param buf Buf value.
 * @param len Len value.
 */
uint32_t yi_uart_dma_lwrb_read(yi_uart_dma_lwrb_t *ctx,
                               uint8_t *buf, uint32_t len);
/**
 * @brief Perform the yi uart dma lwrb overrun operation.
 * @param ctx Ctx value.
 * @param clear Clear value.
 */
uint32_t yi_uart_dma_lwrb_overrun(yi_uart_dma_lwrb_t *ctx, bool clear);
/**
 * @brief Perform the yi uart dma lwrb idle operation.
 * @param ctx Ctx value.
 * @param clear Clear value.
 */
bool yi_uart_dma_lwrb_idle(yi_uart_dma_lwrb_t *ctx, bool clear);

#ifdef __cplusplus
}
#endif

#endif
