#include "RTT_get.h"

/*接收RTT指令格式
*第一位 命令类别
*  s 设置运动速度
*  a 设置运动角度
*  d 设置运动距离
*第二位 无
*  空格
*第三位 命令数值
*  速度范围 -999 - 999
*  角度范围 -360 - 360
*  距离范围 -100 - 100
*/

/*将RTT收到的值转换为pwm值
*原理
*前提：设置接收buff长度为8 每次接收完成后将buff清零 rtt发送时不能添加任何结束符
*每次接收后判断buff中0的个数，根据0的个数判断收的的是几位数
*如果是负值，buff[0]代表符号，需要从buff[1]开始处理获取具体数值
*/
int16_t rtt_get(uint8_t *array)
{
	uint8_t i = 0;
	uint8_t zero_count = 0;
	uint16_t ret = 0;
	
	for(i = 0; i < 8; i++)
	{
		if(array[i] == 0)
		{
			zero_count++;
		}
	}
	
	if(zero_count > 5)
	{
		if(zero_count == 8)
		{
			return 1008;
		}
		else
		{
			return 1001;/*命令位数出错*/
		}
	}
	else
	{
		if((array[0] != 0x73)/*s*/
			&& (array[0] != 0x53)/*S*/ 
			&& (array[0] != 0x61)/*a*/
			&& (array[0] != 0x41)/*A*/
			&& (array[0] != 0x64)/*d*/
			&& (array[0] != 0x44)/*D*/
		)
		{
			return 1002;/*命令类型非法*/
		}
		else
		{
			if(array[1] != 0x20)/*空格*/
			{
				return 1003;/*命令空格处出错*/
			}
			else
			{
				switch(zero_count)
				{
					case 0x00:
					case 0x01:
						ret = 1004;/*命令超长*/
						break;
					case 0x02:
						if(array[2] == 0x2D)/*判断负号*/
						{
							/*判断是否为数字*/
							if((array[3] >= 0x30) && (array[3] <= 0x39)
								&& (array[4] >= 0x30) && (array[4] <= 0x39)
							  && (array[5] >= 0x30) && (array[5] <= 0x39))
							{
								ret = array[3] * 100 + array[4] * 10 + array[5];
							}
							else
							{
								ret = 1005;/*命令数值部分包含非法内容*/
							}
						}
						else
						{
							ret = 1006;/*命令数值超限*/
						}
						break;
					case 0x03:
						if(array[2] == 0x2D)/*判断负号*/
						{
							/*判断是否为数字*/
							if((array[3] >= 0x30) && (array[3] <= 0x39)
								&& (array[4] >= 0x30) && (array[4] <= 0x39))
							{
								ret = array[3] * 10 + array[4];
							}
							else
							{
								ret = 1005;/*命令数值部分包含非法内容*/
							}
						}
						else
						{
							/*判断是否为数字*/
							if((array[2] >= 0x30) && (array[2] <= 0x39)
								&& (array[3] >= 0x30) && (array[3] <= 0x39)
							  && (array[4] >= 0x30) && (array[4] <= 0x39))
							{
								ret = array[2] * 100 + array[3] * 10 + array[4];
							}
							else
							{
								ret = 1005;/*命令数值部分包含非法内容*/
							}
						}
						break;
					case 0x04:
						if(array[2] == 0x2D)/*判断负号*/
						{
							/*判断是否为数字*/
							if((array[3] >= 0x30) && (array[3] <= 0x39))
							{
								ret = array[3];
							}
							else
							{
								ret = 1005;/*命令数值部分包含非法内容*/
							}
						}
						else
						{
							/*判断是否为数字*/
							if((array[2] >= 0x30) && (array[2] <= 0x39)
								&& (array[3] >= 0x30) && (array[3] <= 0x39))
							{
								ret = array[2] * 10 + array[3];
							}
							else
							{
								ret = 1005;/*命令数值部分包含非法内容*/
							}
						}
						break;
					case 0x05:
						if(array[2] == 0x2D)/*判断负号*/
						{
							ret = 1007;/*命令数值部分为空*/
						}
						else
						{
							/*判断是否为数字*/
							if((array[2] >= 0x30) && (array[2] <= 0x39))
							{
								ret = array[2];
							}
							else
							{
								ret = 1005;/*命令数值部分包含非法内容*/
							}
						}
						break;
				}
				return ret;
			}
		}
	}
}

int16_t get_value(uint8_t *array)
{
	int16_t num = rtt_get(array);
	switch(num)
	{
		case CMD_BITS_ERR:
			SEGGER_RTT_printf(0, "命令长度出错，不能少于3位\r\n");
			break;
	}	
	
	return num;
}
/*将RTT收到的值转换为pwm值
*原理
*前提：设置接收buff长度为8 每次接收完成后将buff清零 rtt发送时不能添加任何结束符
*每次接收后判断buff中0的个数，根据0的个数判断收的的是几位数
*如果是负值，buff[0]代表符号，需要从buff[1]开始处理获取具体数值
*/
uint16_t rtt_speed(uint8_t *array, uint8_t sign)
{
	uint8_t i = 0;
	uint8_t zero_count = 0;
	uint16_t ret = 0;

	for(i = 0; i < 8; i++)
	{
		if(array[i] == 0)
		{
			zero_count++;
		}
	}
	switch(zero_count)
	{
		case 0x01:
			break;
		case 0x02:
			break;
		case 0x03:
			break;
		case 0x04:
			if(sign == 0)
				ret = (array[1] - 48) * 100 + (array[2] - 48) * 10 + (array[3] - 48);
			else
				ret = 0;
			break;
		case 0x05:
			if(sign == 0)
				ret = (array[1] - 48) * 10 + (array[2] - 48);
			else
				ret = (array[0] - 48) * 100 + (array[1] - 48) * 10 + (array[2] - 48);
			break;
		case 0x06:
			if(sign == 0)
				ret = (array[1] - 48);
			else
				ret = (array[0] - 48) * 10 + (array[1] - 48);
			break;
		case 0x07:
			if(sign == 0)
				ret = 0;
			else
				ret = (array[0] - 48);
			break;
	}
	return ret;
}
