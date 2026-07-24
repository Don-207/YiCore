#include "yi_i2c_stm32.h"
#include "yi_pinmux.h"

int yi_i2c_stm32_init(const void *config)
{
    const yi_i2c_stm32_config_t *cfg = config;
    yi_i2c_stm32_data_t *data;
    if((cfg == NULL) || (cfg->self == NULL) || (cfg->self->data == NULL) ||
       !yi_device_is_ready(cfg->scl_pin) || !yi_device_is_ready(cfg->sda_pin) ||
       (cfg->clock_frequency == 0U) || (cfg->clock_frequency > 400000U) ||
       (cfg->irq_priority > 15U)) { return -1; }
    if((yi_stm32_periph_clock_enable(&cfg->clock) != 0) ||
       (yi_pinmux_apply(cfg->scl_pin) != 0) ||
       (yi_pinmux_apply(cfg->sda_pin) != 0)) { return -1; }
    data = cfg->self->data;
    data->hi2c.Instance = cfg->instance;
    data->hi2c.Init.ClockSpeed = cfg->clock_frequency;
    data->hi2c.Init.DutyCycle = I2C_DUTYCYCLE_2;
    data->hi2c.Init.OwnAddress1 = 0U;
    data->hi2c.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    data->hi2c.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    data->hi2c.Init.OwnAddress2 = 0U;
    data->hi2c.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    data->hi2c.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if(HAL_I2C_Init(&data->hi2c) != HAL_OK) { return -1; }
    HAL_NVIC_SetPriority(cfg->event_irqn, cfg->irq_priority, 0U);
    HAL_NVIC_SetPriority(cfg->error_irqn, cfg->irq_priority, 0U);
    HAL_NVIC_EnableIRQ(cfg->event_irqn);
    HAL_NVIC_EnableIRQ(cfg->error_irqn);
    return 0;
}

static int yi_i2c_stm32_transfer(yi_device_t *dev, uint8_t address,
                                 yi_i2c_msg_t *messages, uint8_t count,
                                 uint32_t timeout_ms)
{
    yi_i2c_stm32_data_t *data = dev->data;
    for(uint8_t index = 0U; index < count; index++)
    {
        HAL_StatusTypeDef status;
        if((messages[index].flags & YI_I2C_MSG_READ) != 0U)
        {
            status = HAL_I2C_Master_Receive(&data->hi2c, (uint16_t)address << 1,
                                            messages[index].buffer,
                                            messages[index].length, timeout_ms);
        }
        else
        {
            status = HAL_I2C_Master_Transmit(&data->hi2c, (uint16_t)address << 1,
                                             messages[index].buffer,
                                             messages[index].length, timeout_ms);
        }
        if(status != HAL_OK) { return -1; }
    }
    return 0;
}

void yi_i2c_stm32_event_irq_handler(yi_device_t *dev)
{ if((dev != NULL) && (dev->data != NULL)) HAL_I2C_EV_IRQHandler(&((yi_i2c_stm32_data_t *)dev->data)->hi2c); }
void yi_i2c_stm32_error_irq_handler(yi_device_t *dev)
{ if((dev != NULL) && (dev->data != NULL)) HAL_I2C_ER_IRQHandler(&((yi_i2c_stm32_data_t *)dev->data)->hi2c); }

const yi_i2c_api_t yi_i2c_stm32_api = { .transfer = yi_i2c_stm32_transfer };
