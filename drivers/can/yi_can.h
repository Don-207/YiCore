/**
 * @file yi_can.h
 * @brief YiCore can interface.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#ifndef YI_CAN_H
#define YI_CAN_H

#include "yi_device.h"

typedef struct
{
    uint32_t id; /**< Id value. */
    uint8_t length; /**< Length value. */
    uint8_t data[8]; /**< Data value. */
    bool extended; /**< Extended value. */
    bool remote; /**< Remote value. */} yi_can_frame_t;

/**
 * @brief Perform the yi can send operation.
 * @param dev Device instance.
 * @param frame Frame value.
 * @param mailbox Mailbox value.
 */
int yi_can_send(yi_device_t *dev, const yi_can_frame_t *frame, uint32_t *mailbox);
/**
 * @brief Perform the yi can receive operation.
 * @param dev Device instance.
 * @param fifo Fifo value.
 * @param frame Frame value.
 */
int yi_can_receive(yi_device_t *dev, uint32_t fifo, yi_can_frame_t *frame);
/**
 * @brief Perform the yi can irq handler operation.
 * @param dev Device instance.
 */
void yi_can_irq_handler(yi_device_t *dev);

#endif
