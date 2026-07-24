#ifndef YI_CLOCK_H
#define YI_CLOCK_H

#include "yi_device.h"

int yi_clock_enable(yi_device_t *dev);
int yi_clock_disable(yi_device_t *dev);
uint32_t yi_clock_get_rate(yi_device_t *dev);

#endif
