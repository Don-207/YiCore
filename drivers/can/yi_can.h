#ifndef YI_CAN_H
#define YI_CAN_H

#include "yi_device.h"

typedef struct
{
    uint32_t id;
    uint8_t length;
    uint8_t data[8];
    bool extended;
    bool remote;
} yi_can_frame_t;

int yi_can_send(yi_device_t *dev, const yi_can_frame_t *frame, uint32_t *mailbox);
int yi_can_receive(yi_device_t *dev, uint32_t fifo, yi_can_frame_t *frame);
void yi_can_irq_handler(yi_device_t *dev);

#endif
