#include "yi_uart.h"
#include "yi_uart_stm32.h"
#include "yi_pinmux.h"

#define YI_UART_TIMEOUT_MS 1000U

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
    if(HAL_UART_Init(&data->huart) != HAL_OK)
    {
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
    return (HAL_UART_Init(&data->huart) == HAL_OK) ? 0 : -1;
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
    return yi_stm32_periph_clock_disable(&cfg->clock);
}

static int yi_uart_write(yi_device_t *dev, const uint8_t *buf, uint32_t len)
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

static int yi_uart_read(yi_device_t *dev, uint8_t *buf, uint32_t len)
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
        HAL_UART_IRQHandler(&data->huart);
    }
}

const yi_device_api_t yi_uart_driver_api =
{
    .open = yi_uart_open,
    .close = yi_uart_close,
    .write = yi_uart_write,
    .read = yi_uart_read
};
