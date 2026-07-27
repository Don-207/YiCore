#ifndef ECG_PROTOCOL_H
#define ECG_PROTOCOL_H

#include "yi_ads1298.h"
#include "yi_device.h"

int ecg_protocol_send(yi_device_t *uart,
                      const yi_ads1298_frame_t *sample);

#endif
