#ifndef ECG_SERVICE_H
#define ECG_SERVICE_H

#include <stdint.h>

typedef struct
{
    uint32_t acquired_frames;
    uint32_t uploaded_frames;
    uint32_t read_errors;
    uint32_t uart_errors;
} ecg_service_stats_t;

int ecg_service_init(void);
void ecg_service_process(void);
const ecg_service_stats_t *ecg_service_stats(void);

#endif
