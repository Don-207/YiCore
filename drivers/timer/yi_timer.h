#ifndef YI_TIMER_H
#define YI_TIMER_H

#include "yi_device.h"

int yi_timer_start(yi_device_t *dev);
int yi_timer_stop(yi_device_t *dev);
uint32_t yi_timer_get_period_count(const yi_device_t *dev);
void yi_timer_irq_handler(yi_device_t *dev);

#endif
