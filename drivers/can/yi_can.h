#ifndef YI_CAN_H
#define YI_CAN_H

#include "yi_device.h"
#include "yi_stm32_periph.h"

typedef struct
{
    yi_device_t *self;
    CAN_TypeDef *instance;
    yi_stm32_periph_clock_t clock;
    yi_device_t *tx_pin;
    yi_device_t *rx_pin;
    uint32_t bitrate;
    uint16_t sample_point;
    IRQn_Type tx_irqn;
    IRQn_Type rx0_irqn;
    IRQn_Type rx1_irqn;
    IRQn_Type sce_irqn;
    uint8_t irq_priority;
} yi_can_config_t;

typedef struct { CAN_HandleTypeDef hcan; } yi_can_data_t;

typedef struct
{
    uint32_t id;
    uint8_t length;
    uint8_t data[8];
    bool extended;
    bool remote;
} yi_can_frame_t;

int yi_can_init(const void *config);
int yi_can_send(yi_device_t *dev, const yi_can_frame_t *frame, uint32_t *mailbox);
int yi_can_receive(yi_device_t *dev, uint32_t fifo, yi_can_frame_t *frame);
void yi_can_irq_handler(yi_device_t *dev);

#define YI_CAN_DEFINE_LEVEL(_name, _level, _priority, _config, _data) \
    YI_DEVICE_DEFINE_WITH_API(_name, _level, _priority, yi_can_init,  \
                              &_config, &_data, NULL)

#endif
