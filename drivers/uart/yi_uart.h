#ifndef YI_UART_H
#define YI_UART_H


#include "yi_device.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef yi_device_api_t yi_uart_api_t;

/*
 * 阻塞发送/接收。
 * 成功返回0，参数错误、超时或HAL错误返回-1。
 */
extern const yi_device_api_t yi_uart_driver_api;

#ifdef __cplusplus
}
#endif


#endif
