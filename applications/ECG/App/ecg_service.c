#include "ecg_service.h"

#include <stdbool.h>
#include <string.h>

#include "ecg_protocol.h"
#include "yi_ads1298.h"
#include "yi_device.h"

#define ECG_DECIMATION 5U

static yi_device_t *ecg_ads1298;
static yi_device_t *ecg_uart;
static uint8_t ecg_decimation;
static ecg_service_stats_t ecg_stats;

int ecg_service_init(void)
{
    ecg_ads1298 = yi_device_get("ads1298");
    ecg_uart = yi_device_get("usart1");
    ecg_decimation = 0U;
    memset(&ecg_stats, 0, sizeof(ecg_stats));

    if(!yi_device_is_ready(ecg_ads1298) ||
       !yi_device_is_ready(ecg_uart) ||
       (yi_ads1298_start(ecg_ads1298) != 0) ||
       (yi_ads1298_set_continuous(ecg_ads1298, true) != 0))
    {
        return -1;
    }
    return 0;
}

void ecg_service_process(void)
{
    yi_ads1298_frame_t sample;
    bool ready;

    if(yi_ads1298_data_ready(ecg_ads1298, &ready) != 0)
    {
        ecg_stats.read_errors++;
        return;
    }
    if(!ready) { return; }
    if(yi_ads1298_read_frame(ecg_ads1298, &sample) != 0)
    {
        ecg_stats.read_errors++;
        return;
    }
    ecg_stats.acquired_frames++;

    if(++ecg_decimation < ECG_DECIMATION) { return; }
    ecg_decimation = 0U;
    if(ecg_protocol_send(ecg_uart, &sample) != 0)
    {
        ecg_stats.uart_errors++;
        return;
    }
    ecg_stats.uploaded_frames++;
}

const ecg_service_stats_t *ecg_service_stats(void)
{
    return &ecg_stats;
}
