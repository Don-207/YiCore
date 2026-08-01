/**
 * @file yidap_ch32h417_port.c
 * @brief Implement YiDAP target signaling on CH32H417 PA4 through PA8.
 * @author Don
 * @date 2026-08-01
 * @version 1.0.0
 */
#include "yidap_ch32h417_port.h"
uint32_t yidap_port_cpu_clock_hz(void){return YIDAP_CPU_CLOCK_HZ;}
void yidap_port_jtag_setup(void){GPIO_InitTypeDef out={0},in={0},reset={0};RCC_HB2PeriphClockCmd(RCC_HB2Periph_GPIOA,ENABLE);out.GPIO_Pin=YIDAP_TCK|YIDAP_TMS|YIDAP_TDI;out.GPIO_Speed=GPIO_Speed_Very_High;out.GPIO_Mode=GPIO_Mode_Out_PP;GPIO_Init(YIDAP_PORT,&out);in.GPIO_Pin=YIDAP_TDO;in.GPIO_Mode=GPIO_Mode_IN_FLOATING;GPIO_Init(YIDAP_PORT,&in);reset.GPIO_Pin=YIDAP_RESET;reset.GPIO_Speed=GPIO_Speed_Very_High;reset.GPIO_Mode=GPIO_Mode_Out_OD;GPIO_Init(YIDAP_PORT,&reset);GPIO_SetBits(YIDAP_PORT,YIDAP_TCK|YIDAP_TMS|YIDAP_TDI|YIDAP_RESET);}
void yidap_port_swd_setup(void){GPIO_InitTypeDef out={0},unused={0},reset={0};RCC_HB2PeriphClockCmd(RCC_HB2Periph_GPIOA,ENABLE);out.GPIO_Pin=YIDAP_TCK|YIDAP_TMS;out.GPIO_Speed=GPIO_Speed_Very_High;out.GPIO_Mode=GPIO_Mode_Out_PP;GPIO_Init(YIDAP_PORT,&out);unused.GPIO_Pin=YIDAP_TDO|YIDAP_TDI;unused.GPIO_Mode=GPIO_Mode_IN_FLOATING;GPIO_Init(YIDAP_PORT,&unused);reset.GPIO_Pin=YIDAP_RESET;reset.GPIO_Speed=GPIO_Speed_Very_High;reset.GPIO_Mode=GPIO_Mode_Out_OD;GPIO_Init(YIDAP_PORT,&reset);GPIO_SetBits(YIDAP_PORT,YIDAP_TCK|YIDAP_TMS|YIDAP_RESET);}
void yidap_port_off(void){GPIO_InitTypeDef input={0};input.GPIO_Pin=YIDAP_TDO|YIDAP_TDI|YIDAP_TCK|YIDAP_TMS|YIDAP_RESET;input.GPIO_Mode=GPIO_Mode_IN_FLOATING;GPIO_Init(YIDAP_PORT,&input);}
void yidap_port_init(void){GPIO_InitTypeDef reset={0};RCC_HB2PeriphClockCmd(RCC_HB2Periph_GPIOA,ENABLE);reset.GPIO_Pin=YIDAP_RESET;reset.GPIO_Speed=GPIO_Speed_Very_High;reset.GPIO_Mode=GPIO_Mode_Out_OD;GPIO_Init(YIDAP_PORT,&reset);GPIO_SetBits(YIDAP_PORT,YIDAP_RESET);yidap_port_off();}
void yidap_clock_low(void){YIDAP_PORT->BCR=YIDAP_TCK;}
void yidap_clock_high(void){YIDAP_PORT->BSHR=YIDAP_TCK;}
uint32_t yidap_data_read(void){return GPIO_ReadInputDataBit(YIDAP_PORT,YIDAP_TMS);}
void yidap_data_write(uint32_t bit){GPIO_WriteBit(YIDAP_PORT,YIDAP_TMS,bit?Bit_SET:Bit_RESET);}
uint32_t yidap_tdo_read(void){return GPIO_ReadInputDataBit(YIDAP_PORT,YIDAP_TDO);}
void yidap_tdi_write(uint32_t bit){GPIO_WriteBit(YIDAP_PORT,YIDAP_TDI,bit?Bit_SET:Bit_RESET);}
uint32_t yidap_reset_read(void){return GPIO_ReadInputDataBit(YIDAP_PORT,YIDAP_RESET);}
void yidap_reset_write(uint32_t bit){GPIO_WriteBit(YIDAP_PORT,YIDAP_RESET,bit?Bit_SET:Bit_RESET);}
void yidap_data_output(void){GPIO_InitTypeDef gpio={YIDAP_TMS,GPIO_Speed_Very_High,GPIO_Mode_Out_PP};GPIO_Init(YIDAP_PORT,&gpio);}
void yidap_data_input(void){GPIO_InitTypeDef gpio={YIDAP_TMS,GPIO_Speed_Very_High,GPIO_Mode_IN_FLOATING};GPIO_Init(YIDAP_PORT,&gpio);}
