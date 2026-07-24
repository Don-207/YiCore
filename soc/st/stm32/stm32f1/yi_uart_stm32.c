#include "yi_uart.h"
#include "yi_uart_stm32.h"
#include "yi_pinmux.h"

#define YI_UART_TIMEOUT_MS 1000U

static bool yi_uart_has_dma(const yi_uart_stm32_config_t *cfg)
{
    return (cfg->tx_dma_channel != NULL) || (cfg->rx_dma_channel != NULL);
}

static int yi_uart_stm32_dma_init(const yi_uart_stm32_config_t *cfg,
                                  yi_uart_stm32_data_t *data)
{
    if(!yi_uart_has_dma(cfg))
    {
        return 0;
    }

    __HAL_RCC_DMA1_CLK_ENABLE();

    if(cfg->tx_dma_channel != NULL)
    {
        data->hdma_tx.Instance = cfg->tx_dma_channel;
        data->hdma_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        data->hdma_tx.Init.PeriphInc = DMA_PINC_DISABLE;
        data->hdma_tx.Init.MemInc = DMA_MINC_ENABLE;
        data->hdma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        data->hdma_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        data->hdma_tx.Init.Mode = DMA_NORMAL;
        data->hdma_tx.Init.Priority = DMA_PRIORITY_LOW;
        if(HAL_DMA_Init(&data->hdma_tx) != HAL_OK)
        {
            data->huart.hdmatx = NULL;
            return -1;
        }
        __HAL_LINKDMA(&data->huart, hdmatx, data->hdma_tx);
        HAL_NVIC_SetPriority(cfg->tx_dma_irqn, cfg->dma_irq_priority, 0U);
        HAL_NVIC_EnableIRQ(cfg->tx_dma_irqn);
    }

    if(cfg->rx_dma_channel != NULL)
    {
        data->hdma_rx.Instance = cfg->rx_dma_channel;
        data->hdma_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
        data->hdma_rx.Init.PeriphInc = DMA_PINC_DISABLE;
        data->hdma_rx.Init.MemInc = DMA_MINC_ENABLE;
        data->hdma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        data->hdma_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        data->hdma_rx.Init.Mode = DMA_CIRCULAR;
        data->hdma_rx.Init.Priority = DMA_PRIORITY_HIGH;
        if(HAL_DMA_Init(&data->hdma_rx) != HAL_OK)
        {
            if(cfg->tx_dma_channel != NULL)
            {
                HAL_NVIC_DisableIRQ(cfg->tx_dma_irqn);
                (void)HAL_DMA_DeInit(&data->hdma_tx);
                data->huart.hdmatx = NULL;
            }
            data->huart.hdmarx = NULL;
            return -1;
        }
        __HAL_LINKDMA(&data->huart, hdmarx, data->hdma_rx);
        HAL_NVIC_SetPriority(cfg->rx_dma_irqn, cfg->dma_irq_priority, 0U);
        HAL_NVIC_EnableIRQ(cfg->rx_dma_irqn);
    }

    return 0;
}

static void yi_uart_stm32_dma_deinit(const yi_uart_stm32_config_t *cfg,
                                     yi_uart_stm32_data_t *data)
{
    if(cfg->tx_dma_channel != NULL)
    {
        HAL_NVIC_DisableIRQ(cfg->tx_dma_irqn);
        (void)HAL_DMA_DeInit(&data->hdma_tx);
    }
    if(cfg->rx_dma_channel != NULL)
    {
        HAL_NVIC_DisableIRQ(cfg->rx_dma_irqn);
        (void)HAL_DMA_DeInit(&data->hdma_rx);
    }
}

int yi_uart_stm32_init(const void *config)
{
    const yi_uart_stm32_config_t *cfg = (const yi_uart_stm32_config_t *)config;
    yi_uart_stm32_data_t *data;

    if((cfg == NULL) || (cfg->self == NULL) || (cfg->instance == NULL) ||
       !yi_device_is_ready(cfg->tx_pin) || !yi_device_is_ready(cfg->rx_pin) ||
       (cfg->baudrate == 0U) || (cfg->irq_priority > 15U) ||
       (cfg->self->data == NULL))
    {
        return -1;
    }
    if((yi_stm32_periph_clock_enable(&cfg->clock) != 0) ||
       (yi_pinmux_apply(cfg->tx_pin) != 0) ||
       (yi_pinmux_apply(cfg->rx_pin) != 0))
    {
        return -1;
    }

    data = (yi_uart_stm32_data_t *)cfg->self->data;
    data->huart.Instance = cfg->instance;
    data->huart.Init.BaudRate = cfg->baudrate;
    data->huart.Init.WordLength = UART_WORDLENGTH_8B;
    data->huart.Init.StopBits = UART_STOPBITS_1;
    data->huart.Init.Parity = UART_PARITY_NONE;
    data->huart.Init.Mode = UART_MODE_TX_RX;
    data->huart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    data->huart.Init.OverSampling = UART_OVERSAMPLING_16;
    data->rx_dma_buffer = NULL;
    data->rx_dma_size = 0U;
    data->rx_idle_seen = false;

    if(yi_uart_stm32_dma_init(cfg, data) != 0)
    {
        (void)yi_stm32_periph_clock_disable(&cfg->clock);
        return -1;
    }
    if(HAL_UART_Init(&data->huart) != HAL_OK)
    {
        yi_uart_stm32_dma_deinit(cfg, data);
        (void)yi_stm32_periph_clock_disable(&cfg->clock);
        return -1;
    }
    HAL_NVIC_SetPriority(cfg->irqn, cfg->irq_priority, 0U);
    HAL_NVIC_EnableIRQ(cfg->irqn);
    return 0;
}

static int yi_uart_open(yi_device_t *dev)
{
    const yi_uart_stm32_config_t *cfg;
    yi_uart_stm32_data_t *data;

    if((dev == NULL) || (dev->config == NULL) || (dev->data == NULL))
    {
        return -1;
    }
    cfg = (const yi_uart_stm32_config_t *)dev->config;
    data = (yi_uart_stm32_data_t *)dev->data;
    if(yi_stm32_periph_clock_enable(&cfg->clock) != 0)
    {
        return -1;
    }
    data->rx_idle_seen = false;
    if(yi_uart_stm32_dma_init(cfg, data) != 0)
    {
        (void)yi_stm32_periph_clock_disable(&cfg->clock);
        return -1;
    }
    if(HAL_UART_Init(&data->huart) != HAL_OK)
    {
        yi_uart_stm32_dma_deinit(cfg, data);
        (void)yi_stm32_periph_clock_disable(&cfg->clock);
        return -1;
    }
    HAL_NVIC_SetPriority(cfg->irqn, cfg->irq_priority, 0U);
    HAL_NVIC_EnableIRQ(cfg->irqn);
    return 0;
}

static int yi_uart_close(yi_device_t *dev)
{
    const yi_uart_stm32_config_t *cfg;
    yi_uart_stm32_data_t *data;

    if((dev == NULL) || (dev->config == NULL) || (dev->data == NULL))
    {
        return -1;
    }
    cfg = (const yi_uart_stm32_config_t *)dev->config;
    data = (yi_uart_stm32_data_t *)dev->data;
    HAL_NVIC_DisableIRQ(cfg->irqn);
    if(HAL_UART_DeInit(&data->huart) != HAL_OK)
    {
        return -1;
    }
    yi_uart_stm32_dma_deinit(cfg, data);
    return yi_stm32_periph_clock_disable(&cfg->clock);
}

static int yi_uart_write_blocking(yi_device_t *dev, const uint8_t *buf, uint32_t len)
{
    yi_uart_stm32_data_t *data;
    if(!yi_device_is_ready(dev) || (buf == NULL) ||
       (len == 0U) || (len > UINT16_MAX) || (dev->data == NULL))
    {
        return -1;
    }
    data = (yi_uart_stm32_data_t *)dev->data;
    return (HAL_UART_Transmit(&data->huart, (uint8_t *)buf, (uint16_t)len,
                              YI_UART_TIMEOUT_MS) == HAL_OK) ? 0 : -1;
}

static int yi_uart_read_blocking(yi_device_t *dev, uint8_t *buf, uint32_t len)
{
    yi_uart_stm32_data_t *data;
    if(!yi_device_is_ready(dev) || (buf == NULL) ||
       (len == 0U) || (len > UINT16_MAX) || (dev->data == NULL))
    {
        return -1;
    }
    data = (yi_uart_stm32_data_t *)dev->data;
    return (HAL_UART_Receive(&data->huart, buf, (uint16_t)len,
                             YI_UART_TIMEOUT_MS) == HAL_OK) ? 0 : -1;
}

void yi_uart_stm32_irq_handler(yi_device_t *dev)
{
    if((dev != NULL) && (dev->data != NULL))
    {
        yi_uart_stm32_data_t *data = (yi_uart_stm32_data_t *)dev->data;
        if(__HAL_UART_GET_FLAG(&data->huart, UART_FLAG_IDLE) != RESET)
        {
            __HAL_UART_CLEAR_IDLEFLAG(&data->huart);
            data->rx_idle_seen = true;
        }
        HAL_UART_IRQHandler(&data->huart);
    }
}

void yi_uart_stm32_dma_tx_irq_handler(yi_device_t *dev)
{
    if((dev != NULL) && (dev->data != NULL))
    {
        yi_uart_stm32_data_t *data = (yi_uart_stm32_data_t *)dev->data;
        HAL_DMA_IRQHandler(&data->hdma_tx);
    }
}

void yi_uart_stm32_dma_rx_irq_handler(yi_device_t *dev)
{
    if((dev != NULL) && (dev->data != NULL))
    {
        yi_uart_stm32_data_t *data = (yi_uart_stm32_data_t *)dev->data;
        HAL_DMA_IRQHandler(&data->hdma_rx);
    }
}

static int yi_uart_write_dma_impl(yi_device_t *dev, const uint8_t *buf, uint32_t len)
{
    yi_uart_stm32_data_t *data;
    if(!yi_device_is_ready(dev) || (buf == NULL) ||
       (len == 0U) || (len > UINT16_MAX) || (dev->data == NULL))
    {
        return -1;
    }
    data = (yi_uart_stm32_data_t *)dev->data;
    if(data->huart.hdmatx == NULL)
    {
        return -1;
    }
    return (HAL_UART_Transmit_DMA(&data->huart, (uint8_t *)buf,
                                  (uint16_t)len) == HAL_OK) ? 0 : -1;
}

static int yi_uart_read_dma_impl(yi_device_t *dev, uint8_t *buf, uint32_t len)
{
    yi_uart_stm32_data_t *data;
    if(!yi_device_is_ready(dev) || (buf == NULL) ||
       (len == 0U) || (len > UINT16_MAX) || (dev->data == NULL))
    {
        return -1;
    }
    data = (yi_uart_stm32_data_t *)dev->data;
    if(data->huart.hdmarx == NULL)
    {
        return -1;
    }
    return (HAL_UART_Receive_DMA(&data->huart, buf,
                                 (uint16_t)len) == HAL_OK) ? 0 : -1;
}

static int yi_uart_rx_start_dma_impl(yi_device_t *dev, uint8_t *buf, uint32_t len)
{
    yi_uart_stm32_data_t *data;
    if(!yi_device_is_ready(dev) || (buf == NULL) ||
       (len == 0U) || (len > UINT16_MAX) || (dev->data == NULL))
    {
        return -1;
    }
    data = (yi_uart_stm32_data_t *)dev->data;
    if(data->huart.hdmarx == NULL)
    {
        return -1;
    }
    data->rx_dma_buffer = buf;
    data->rx_dma_size = len;
    data->rx_idle_seen = false;
    if(HAL_UART_Receive_DMA(&data->huart, buf, (uint16_t)len) != HAL_OK)
    {
        data->rx_dma_buffer = NULL;
        data->rx_dma_size = 0U;
        return -1;
    }
    __HAL_UART_ENABLE_IT(&data->huart, UART_IT_IDLE);
    return 0;
}

static uint32_t yi_uart_rx_dma_pos_impl(yi_device_t *dev)
{
    yi_uart_stm32_data_t *data;
    if(!yi_device_is_ready(dev) || (dev->data == NULL))
    {
        return 0U;
    }
    data = (yi_uart_stm32_data_t *)dev->data;
    if((data->huart.hdmarx == NULL) || (data->rx_dma_size == 0U))
    {
        return 0U;
    }
    return data->rx_dma_size - __HAL_DMA_GET_COUNTER(data->huart.hdmarx);
}

static bool yi_uart_rx_idle_impl(yi_device_t *dev, bool clear)
{
    yi_uart_stm32_data_t *data;
    bool seen;
    if(!yi_device_is_ready(dev) || (dev->data == NULL))
    {
        return false;
    }
    data = (yi_uart_stm32_data_t *)dev->data;
    seen = data->rx_idle_seen;
    if(clear)
    {
        data->rx_idle_seen = false;
    }
    return seen;
}

const yi_uart_api_t yi_uart_stm32_api =
{
    .base =
    {
        .open = yi_uart_open,
        .close = yi_uart_close,
        .write = yi_uart_write_blocking,
        .read = yi_uart_read_blocking
    },
    .write_dma = yi_uart_write_dma_impl,
    .read_dma = yi_uart_read_dma_impl,
    .rx_start_dma = yi_uart_rx_start_dma_impl,
    .rx_dma_pos = yi_uart_rx_dma_pos_impl,
    .rx_idle = yi_uart_rx_idle_impl
};

const yi_device_api_t yi_uart_driver_api =
{
    .open = yi_uart_open,
    .close = yi_uart_close,
    .write = yi_uart_write_blocking,
    .read = yi_uart_read_blocking
};

static const yi_uart_api_t *yi_uart_api_get(yi_device_t *dev)
{
    if((dev == NULL) || (dev->api != &yi_uart_stm32_api.base))
    {
        return NULL;
    }
    return (const yi_uart_api_t *)dev->api;
}

int yi_uart_write(yi_device_t *dev, const uint8_t *buf, uint32_t len)
{
    return ((dev != NULL) && (dev->api != NULL) && (dev->api->write != NULL)) ?
        dev->api->write(dev, buf, len) : -1;
}

int yi_uart_read(yi_device_t *dev, uint8_t *buf, uint32_t len)
{
    return ((dev != NULL) && (dev->api != NULL) && (dev->api->read != NULL)) ?
        dev->api->read(dev, buf, len) : -1;
}

int yi_uart_write_dma(yi_device_t *dev, const uint8_t *buf, uint32_t len)
{
    const yi_uart_api_t *api = yi_uart_api_get(dev);
    return ((api != NULL) && (api->write_dma != NULL)) ?
        api->write_dma(dev, buf, len) : -1;
}

int yi_uart_read_dma(yi_device_t *dev, uint8_t *buf, uint32_t len)
{
    const yi_uart_api_t *api = yi_uart_api_get(dev);
    return ((api != NULL) && (api->read_dma != NULL)) ?
        api->read_dma(dev, buf, len) : -1;
}

int yi_uart_rx_start_dma(yi_device_t *dev, uint8_t *buf, uint32_t len)
{
    const yi_uart_api_t *api = yi_uart_api_get(dev);
    return ((api != NULL) && (api->rx_start_dma != NULL)) ?
        api->rx_start_dma(dev, buf, len) : -1;
}

uint32_t yi_uart_rx_dma_pos(yi_device_t *dev)
{
    const yi_uart_api_t *api = yi_uart_api_get(dev);
    return ((api != NULL) && (api->rx_dma_pos != NULL)) ?
        api->rx_dma_pos(dev) : 0U;
}

bool yi_uart_rx_idle(yi_device_t *dev, bool clear)
{
    const yi_uart_api_t *api = yi_uart_api_get(dev);
    return ((api != NULL) && (api->rx_idle != NULL)) ?
        api->rx_idle(dev, clear) : false;
}
