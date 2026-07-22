#ifndef __RTT_GET_H__
#define __RTT_GET_H__

#include "main.h"
#include "SEGGER_RTT.h"

enum RTT_GET_ERR
{
	CMD_BITS_ERR     = 1001,
	CMD_TYPE_ILLEGAL = 1002,
	CMD_SPACE_ERR    = 1003,
	CMD_LENGTH_ERR   = 1004,
	CMD_NUM_ILLEGAL  = 1005,
	CMD_NUM_BEYOND   = 1006,
	CMD_NUM_NULL     = 1007,
};

int16_t rtt_get(uint8_t *array);
int16_t get_value(uint8_t *array);
#endif
