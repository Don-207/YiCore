/**
 * @file yi_uart_dma_lwrb.c
 * @brief YiCore uart dma lwrb implementation.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#include "yi_uart_dma_lwrb.h"
#include "yi_system.h"

/**
 * @brief Write the module.
 * @param ctx Ctx value.
 * @param buf Buf value.
 * @param len Len value.
 */
static uint32_t yi_uart_dma_lwrb_write(yi_uart_dma_lwrb_t *ctx,
                                       const uint8_t *buf,
                                       uint32_t len)
{
    uint32_t written;

    if(len == 0U)
    {
        return 0U;
    }

    written = (uint32_t)lwrb_write(&ctx->ring, buf, (lwrb_sz_t)len);
    if(written < len)
    {
        ctx->overrun += len - written;
    }
    return written;
}

/**
 * @brief Perform the yi uart dma lwrb sync to operation.
 * @param ctx Ctx value.
 * @param raw_pos Raw pos value.
 */
static uint32_t yi_uart_dma_lwrb_sync_to(yi_uart_dma_lwrb_t *ctx,
                                         uint32_t raw_pos)
{
    uint32_t pos;
    uint32_t last_pos;
    uint32_t written = 0U;

    if((ctx == NULL) || (ctx->dma_buffer == NULL) ||
       (ctx->dma_size == 0U) || !lwrb_is_ready(&ctx->ring))
    {
        return 0U;
    }

    if(raw_pos > ctx->dma_size)
    {
        raw_pos = 0U;
    }
    pos = (raw_pos == ctx->dma_size) ? 0U : raw_pos;
    last_pos = (ctx->last_pos >= ctx->dma_size) ? 0U : ctx->last_pos;

    if((raw_pos == last_pos) && (raw_pos != ctx->dma_size))
    {
        return 0U;
    }
    if((raw_pos == ctx->dma_size) && (last_pos == 0U))
    {
        return 0U;
    }

    if(raw_pos > last_pos)
    {
        written += yi_uart_dma_lwrb_write(ctx,
                                          &ctx->dma_buffer[last_pos],
                                          raw_pos - last_pos);
    }
    else
    {
        written += yi_uart_dma_lwrb_write(ctx,
                                          &ctx->dma_buffer[last_pos],
                                          ctx->dma_size - last_pos);
        if(pos > 0U)
        {
            written += yi_uart_dma_lwrb_write(ctx, ctx->dma_buffer, pos);
        }
    }

    ctx->last_pos = pos;
    return written;
}

/**
 * @brief Perform the yi uart dma lwrb sync operation.
 * @param ctx Ctx value.
 * @param event Event value.
 */
static uint32_t yi_uart_dma_lwrb_sync(yi_uart_dma_lwrb_t *ctx,
                                      yi_uart_rx_event_t event)
{
    uint32_t raw_pos;

    if((ctx == NULL) || (ctx->uart == NULL))
    {
        return 0U;
    }

    raw_pos = (event == YI_UART_RX_EVENT_DMA_COMPLETE) ?
        ctx->dma_size : yi_uart_rx_dma_pos(ctx->uart);
    /**
     * @brief Perform the yi uart dma lwrb sync to operation.
     * @param ctx Ctx value.
     * @param raw_pos Raw pos value.
     */
    return yi_uart_dma_lwrb_sync_to(ctx, raw_pos);
}

/**
 * @brief Perform the yi uart dma lwrb callback operation.
 * @param dev Device instance.
 * @param event Event value.
 * @param user_data User data value.
 */
static void yi_uart_dma_lwrb_callback(yi_device_t *dev,
                                      yi_uart_rx_event_t event,
                                      void *user_data)
{
    yi_uart_dma_lwrb_t *ctx = (yi_uart_dma_lwrb_t *)user_data;

    (void)dev;
    (void)yi_uart_dma_lwrb_sync(ctx, event);
}

/**
 * @brief Perform the yi uart dma lwrb sync locked operation.
 * @param ctx Ctx value.
 */
static uint32_t yi_uart_dma_lwrb_sync_locked(yi_uart_dma_lwrb_t *ctx)
{
    uint32_t key;
    uint32_t written;

    key = yi_system_irq_save();
    written = yi_uart_dma_lwrb_sync(ctx, YI_UART_RX_EVENT_IDLE);
    yi_system_irq_restore(key);
    return written;
}

/**
 * @brief Read locked.
 * @param ctx Ctx value.
 * @param buf Buf value.
 * @param len Len value.
 */
static uint32_t yi_uart_dma_lwrb_read_locked(yi_uart_dma_lwrb_t *ctx,
                                             uint8_t *buf,
                                             uint32_t len)
{
    uint32_t key;
    uint32_t read;

    key = yi_system_irq_save();
    read = (uint32_t)lwrb_read(&ctx->ring, buf, (lwrb_sz_t)len);
    yi_system_irq_restore(key);
    return read;
}

/**
 * @brief Perform the yi uart dma lwrb full locked operation.
 * @param ctx Ctx value.
 */
static uint32_t yi_uart_dma_lwrb_full_locked(yi_uart_dma_lwrb_t *ctx)
{
    uint32_t key;
    uint32_t full;

    key = yi_system_irq_save();
    full = (uint32_t)lwrb_get_full(&ctx->ring);
    yi_system_irq_restore(key);
    return full;
}

/**
 * @brief Perform the yi uart dma lwrb overrun locked operation.
 * @param ctx Ctx value.
 * @param clear Clear value.
 */
static uint32_t yi_uart_dma_lwrb_overrun_locked(yi_uart_dma_lwrb_t *ctx,
                                                bool clear)
{
    uint32_t key;
    uint32_t overrun;

    key = yi_system_irq_save();
    overrun = ctx->overrun;
    if(clear)
    {
        ctx->overrun = 0U;
    }
    yi_system_irq_restore(key);
    return overrun;
}

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
                           uint8_t *ring_buf, uint32_t ring_len)
{
    if((ctx == NULL) || !yi_device_is_ready(uart) ||
       (dma_buf == NULL) || (dma_len == 0U) || (dma_len > UINT16_MAX) ||
       (ring_buf == NULL) || (ring_len < 2U))
    {
        return -1;
    }

    if(lwrb_init(&ctx->ring, ring_buf, (lwrb_sz_t)ring_len) == 0U)
    {
        return -1;
    }

    ctx->uart = uart;
    ctx->dma_buffer = dma_buf;
    ctx->dma_size = dma_len;
    ctx->last_pos = 0U;
    ctx->overrun = 0U;

    if(yi_uart_rx_set_callback(uart, yi_uart_dma_lwrb_callback, ctx) != 0)
    {
        lwrb_free(&ctx->ring);
        ctx->uart = NULL;
        return -1;
    }
    if(yi_uart_rx_start_dma(uart, dma_buf, dma_len) != 0)
    {
        (void)yi_uart_rx_set_callback(uart, NULL, NULL);
        lwrb_free(&ctx->ring);
        ctx->uart = NULL;
        return -1;
    }
    return 0;
}

/**
 * @brief Perform the yi uart dma lwrb detach operation.
 * @param ctx Ctx value.
 */
void yi_uart_dma_lwrb_detach(yi_uart_dma_lwrb_t *ctx)
{
    if(ctx == NULL)
    {
        return;
    }
    if(ctx->uart != NULL)
    {
        (void)yi_uart_rx_set_callback(ctx->uart, NULL, NULL);
    }
    lwrb_free(&ctx->ring);
    ctx->uart = NULL;
    ctx->dma_buffer = NULL;
    ctx->dma_size = 0U;
    ctx->last_pos = 0U;
    ctx->overrun = 0U;
}

/**
 * @brief Perform the yi uart dma lwrb poll operation.
 * @param ctx Ctx value.
 */
uint32_t yi_uart_dma_lwrb_poll(yi_uart_dma_lwrb_t *ctx)
{
    if((ctx == NULL) || (ctx->uart == NULL))
    {
        return 0U;
    }

    /**
     * @brief Perform the yi uart dma lwrb sync locked operation.
     * @param ctx Ctx value.
     */
    return yi_uart_dma_lwrb_sync_locked(ctx);
}

/**
 * @brief Perform the yi uart dma lwrb available operation.
 * @param ctx Ctx value.
 */
uint32_t yi_uart_dma_lwrb_available(yi_uart_dma_lwrb_t *ctx)
{
    if(ctx == NULL)
    {
        return 0U;
    }
    (void)yi_uart_dma_lwrb_poll(ctx);
    /**
     * @brief Perform the yi uart dma lwrb full locked operation.
     * @param ctx Ctx value.
     */
    return yi_uart_dma_lwrb_full_locked(ctx);
}

/**
 * @brief Read the module.
 * @param ctx Ctx value.
 * @param buf Buf value.
 * @param len Len value.
 */
uint32_t yi_uart_dma_lwrb_read(yi_uart_dma_lwrb_t *ctx,
                               uint8_t *buf, uint32_t len)
{
    if((ctx == NULL) || (buf == NULL) || (len == 0U))
    {
        return 0U;
    }
    (void)yi_uart_dma_lwrb_poll(ctx);
    /**
     * @brief Read locked.
     * @param ctx Ctx value.
     * @param buf Buf value.
     * @param len Len value.
     */
    return yi_uart_dma_lwrb_read_locked(ctx, buf, len);
}

/**
 * @brief Perform the yi uart dma lwrb overrun operation.
 * @param ctx Ctx value.
 * @param clear Clear value.
 */
uint32_t yi_uart_dma_lwrb_overrun(yi_uart_dma_lwrb_t *ctx, bool clear)
{
    if(ctx == NULL)
    {
        return 0U;
    }
    /**
     * @brief Perform the yi uart dma lwrb overrun locked operation.
     * @param ctx Ctx value.
     * @param clear Clear value.
     */
    return yi_uart_dma_lwrb_overrun_locked(ctx, clear);
}

/**
 * @brief Perform the yi uart dma lwrb idle operation.
 * @param ctx Ctx value.
 * @param clear Clear value.
 */
bool yi_uart_dma_lwrb_idle(yi_uart_dma_lwrb_t *ctx, bool clear)
{
    if((ctx == NULL) || (ctx->uart == NULL))
    {
        return false;
    }
    /**
     * @brief Perform the yi uart rx idle operation.
     * @param uart Uart value.
     * @param clear Clear value.
     */
    return yi_uart_rx_idle(ctx->uart, clear);
}
