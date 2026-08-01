/**
 * @file yi_dap_protocol.c
 * @brief Implement CMSIS-DAP v2 SWD, JTAG, pin, and control commands.
 * @author Don
 * @date 2026-08-01
 * @version 1.0.0
 */
#include "yi_dap_protocol.h"
#include "yi_dap_protocol_port.h"
#include <string.h>

/** CMSIS-DAP command identifiers implemented by this engine. */
enum { CMD_INFO=0x00U, CMD_HOST_STATUS=0x01U, CMD_CONNECT=0x02U,
       CMD_DISCONNECT=0x03U, CMD_TRANSFER_CONFIGURE=0x04U,
       CMD_TRANSFER=0x05U, CMD_TRANSFER_BLOCK=0x06U,
       CMD_TRANSFER_ABORT=0x07U, CMD_WRITE_ABORT=0x08U,
       CMD_DELAY=0x09U, CMD_RESET_TARGET=0x0AU, CMD_SWJ_PINS=0x10U,
       CMD_SWJ_CLOCK=0x11U, CMD_SWJ_SEQUENCE=0x12U,
       CMD_SWD_CONFIGURE=0x13U, CMD_JTAG_SEQUENCE=0x14U,
       CMD_JTAG_CONFIGURE=0x15U, CMD_JTAG_IDCODE=0x16U,
       CMD_SWD_SEQUENCE=0x1DU };
/** CMSIS-DAP transfer response bits. */
enum { TRANSFER_OK=1U, TRANSFER_WAIT=2U, TRANSFER_FAULT=4U,
       TRANSFER_ERROR=8U };
/** Active target port: zero disabled, one SWD, two JTAG. */
static uint8_t g_port;
/** Busy-loop iterations inserted at each clock half-period. */
static uint32_t g_clock_delay=20U;
/** Maximum WAIT retries requested by DAP_TransferConfigure. */
static uint16_t g_wait_retry=100U;
/** Maximum value-match retries requested by the host. */
static uint16_t g_match_retry;
/** AP read pipeline value retained for compatibility diagnostics. */
static uint32_t g_match_mask=0xFFFFFFFFUL;

/** Delay one target clock half-period in foreground context. */
static void clock_delay(void){volatile uint32_t count=g_clock_delay;while(count--!=0U){__asm volatile("nop");}}
/** Generate one clock and sample SWDIO/TDO while the clock is high. */
static uint32_t clock_sample(int jtag){uint32_t bit;yidap_clock_low();clock_delay();yidap_clock_high();clock_delay();bit=jtag?yidap_tdo_read():yidap_data_read();return bit&1U;}
/** Shift bits least-significant-bit first on SWDIO. */
static void swd_write_bits(uint32_t value,uint32_t count){uint32_t i;yidap_data_output();for(i=0U;i<count;++i){yidap_data_write(value&1U);(void)clock_sample(0);value>>=1U;}}
/** Receive bits least-significant-bit first from SWDIO. */
static uint32_t swd_read_bits(uint32_t count){uint32_t i,value=0U;yidap_data_input();for(i=0U;i<count;++i)value|=clock_sample(0)<<i;return value;}
/** Calculate odd/even XOR parity over a 32-bit word. */
static uint32_t parity32(uint32_t value){value^=value>>16U;value^=value>>8U;value^=value>>4U;value^=value>>2U;value^=value>>1U;return value&1U;}
/** Execute one SWD DP/AP register request and return CMSIS-DAP ACK bits. */
static uint8_t swd_transfer(uint8_t request,uint32_t *data)
{
    uint32_t header=1U|((uint32_t)(request&0x0FU)<<1U);uint32_t ack,value=0U,retry=0U;
    header|=parity32((request&0x0FU))<<5U;header|=1U<<7U;
    do{swd_write_bits(header,8U);yidap_data_input();(void)clock_sample(0);ack=swd_read_bits(3U);if(ack!=TRANSFER_WAIT)break;yidap_data_output();(void)clock_sample(0);}while(++retry<g_wait_retry);
    if(ack==TRANSFER_OK){if((request&2U)!=0U){value=swd_read_bits(32U);if(swd_read_bits(1U)!=parity32(value))ack=TRANSFER_ERROR;yidap_data_output();(void)clock_sample(0);*data=value;}else{yidap_data_output();(void)clock_sample(0);swd_write_bits(*data,32U);swd_write_bits(parity32(*data),1U);}}
    else{if((request&2U)!=0U)(void)swd_read_bits(33U);else{swd_write_bits(0U,33U);}yidap_data_output();(void)clock_sample(0);}
    yidap_data_write(1U);return (uint8_t)ack;
}
/** Encode a little-endian 32-bit response value. */
static void put_u32(uint8_t *p,uint32_t value){p[0]=(uint8_t)value;p[1]=(uint8_t)(value>>8U);p[2]=(uint8_t)(value>>16U);p[3]=(uint8_t)(value>>24U);}
/** Decode a little-endian 32-bit request value. */
static uint32_t get_u32(const uint8_t *p){return(uint32_t)p[0]|((uint32_t)p[1]<<8U)|((uint32_t)p[2]<<16U)|((uint32_t)p[3]<<24U);}
/** Return an identification or capability item. */
static uint16_t process_info(const uint8_t *request,uint8_t *response)
{
    const char *text=0;uint8_t length=0U;response[0]=CMD_INFO;
    if(request[1]==1U)text="YiLink";else if(request[1]==2U)text="YiDAP CH32H417";else if(request[1]==4U)text="1.0.0";else if(request[1]==9U)text="1.0.0";
    if(text!=0){length=(uint8_t)(strlen(text)+1U);response[1]=length;memcpy(&response[2],text,length);return(uint16_t)(2U+length);}
    response[1]=1U;if(request[1]==0xF0U)response[2]=0x03U;else if(request[1]==0xFEU)response[2]=4U;else if(request[1]==0xFFU){response[1]=2U;response[2]=64U;response[3]=0U;return 4U;}else response[1]=0U;return(uint16_t)(2U+response[1]);
}
/** Process DAP_Transfer requests using the active SWD port. */
static uint16_t process_transfer(const uint8_t *request,uint8_t *response)
{
    uint8_t count=request[2],done=0U,ack=TRANSFER_ERROR;uint16_t input=3U,output=3U;uint32_t data;
    response[0]=CMD_TRANSFER;
    while(done<count){uint8_t operation=request[input++];if((operation&0x30U)!=0U){ack=TRANSFER_ERROR;break;}if((operation&2U)==0U){data=get_u32(&request[input]);input+=4U;}ack=swd_transfer(operation,&data);if(ack!=TRANSFER_OK)break;if((operation&2U)!=0U){if(output+4U>64U){ack=TRANSFER_ERROR;break;}put_u32(&response[output],data);output+=4U;}done++;}
    response[1]=done;response[2]=ack;return output;
}
/** Process repeated reads or writes of one DP/AP address. */
static uint16_t process_transfer_block(const uint8_t *request,uint8_t *response)
{
    uint16_t total=(uint16_t)request[2]|((uint16_t)request[3]<<8U),done=0U,output=4U;uint8_t operation=request[4],ack=TRANSFER_ERROR;uint32_t data=0U;response[0]=CMD_TRANSFER_BLOCK;
    if((operation&2U)==0U)data=get_u32(&request[5]);
    while(done<total){ack=swd_transfer(operation,&data);if(ack!=TRANSFER_OK)break;if((operation&2U)!=0U){if(output+4U>64U)break;put_u32(&response[output],data);output+=4U;}done++;}
    response[1]=(uint8_t)done;response[2]=(uint8_t)(done>>8U);response[3]=ack;return output;
}
/** Execute raw SWD input/output sequences used for line reset and diagnostics. */
static uint16_t process_swd_sequence(const uint8_t *request,uint8_t *response)
{
    uint8_t sequences=request[1],sequence;uint16_t input=2U,output=2U;response[0]=CMD_SWD_SEQUENCE;response[1]=0U;
    for(sequence=0U;sequence<sequences;++sequence){uint8_t info=request[input++],cycles=info&0x3FU;uint8_t bytes,i;if(cycles==0U)cycles=64U;bytes=(uint8_t)((cycles+7U)/8U);if((info&0x80U)!=0U){for(i=0U;i<bytes;++i)response[output+i]=0U;for(i=0U;i<cycles;++i)response[output+i/8U]|=(uint8_t)(swd_read_bits(1U)<<(i&7U));output+=bytes;}else{yidap_data_output();for(i=0U;i<cycles;++i){yidap_data_write((request[input+i/8U]>>(i&7U))&1U);(void)clock_sample(0);}input+=bytes;}}
    return output;
}
/** Execute one or more raw JTAG sequences. */
static uint16_t process_jtag_sequence(const uint8_t *request,uint8_t *response)
{
    uint8_t sequences=request[1],sequence;uint16_t input=2U,output=1U;response[0]=CMD_JTAG_SEQUENCE;
    for(sequence=0U;sequence<sequences;++sequence){uint8_t info=request[input++],cycles=info&0x3FU;uint8_t bytes,i;if(cycles==0U)cycles=64U;bytes=(uint8_t)((cycles+7U)/8U);for(i=0U;i<cycles;++i){uint8_t tdi=(uint8_t)((request[input+i/8U]>>(i&7U))&1U);yidap_data_write((info>>6U)&1U);yidap_tdi_write(tdi);if((info&0x80U)!=0U){if((i&7U)==0U)response[output+i/8U]=0U;response[output+i/8U]|=(uint8_t)(clock_sample(1)<<(i&7U));}else(void)clock_sample(1);}input+=bytes;if((info&0x80U)!=0U)output+=bytes;}
    return output;
}
/** Initialize disconnected protocol state and safe pins. */
void yi_dap_protocol_init(void){g_port=0U;g_match_retry=0U;g_match_mask=0xFFFFFFFFUL;yidap_port_init();}
/** Dispatch one CMSIS-DAP request without depending on an external DAP engine. */
uint16_t yi_dap_protocol_process(const uint8_t *request,uint8_t *response)
{
    uint8_t command=request[0];uint32_t value;uint16_t i,length=2U;response[0]=command;response[1]=0xFFU;
    if(command==CMD_INFO)return process_info(request,response);
    if(command==CMD_HOST_STATUS){response[1]=0U;return 2U;}
    if(command==CMD_CONNECT){uint8_t port=request[1];if(port==0U)port=1U;if(port==1U){yidap_port_swd_setup();g_port=1U;}else if(port==2U){yidap_port_jtag_setup();g_port=2U;}else port=0U;response[1]=port;return 2U;}
    if(command==CMD_DISCONNECT){yidap_port_off();g_port=0U;response[1]=0U;return 2U;}
    if(command==CMD_TRANSFER_CONFIGURE){g_wait_retry=(uint16_t)request[2]|((uint16_t)request[3]<<8U);g_match_retry=(uint16_t)request[4]|((uint16_t)request[5]<<8U);response[1]=0U;return 2U;}
    if(command==CMD_TRANSFER&&g_port==1U)return process_transfer(request,response);
    if(command==CMD_TRANSFER_BLOCK&&g_port==1U)return process_transfer_block(request,response);
    if(command==CMD_TRANSFER_ABORT){response[1]=0U;return 2U;}
    if(command==CMD_WRITE_ABORT&&g_port==1U){value=get_u32(&request[2]);response[1]=(swd_transfer(0U,&value)==TRANSFER_OK)?0U:0xFFU;return 2U;}
    if(command==CMD_DELAY){value=(uint32_t)request[1]|((uint32_t)request[2]<<8U);while(value--!=0U){for(i=0U;i<30U;++i)__asm volatile("nop");}response[1]=0U;return 2U;}
    if(command==CMD_RESET_TARGET){yidap_reset_write(0U);for(i=0U;i<60000U;++i)__asm volatile("nop");yidap_reset_write(1U);response[1]=0U;response[2]=1U;return 3U;}
    if(command==CMD_SWJ_PINS){uint8_t output=request[1],select=request[2],pins=0U;if((select&1U)!=0U){if((output&1U)!=0U)yidap_clock_high();else yidap_clock_low();}if((select&2U)!=0U)yidap_data_write((output>>1U)&1U);if((select&0x80U)!=0U)yidap_reset_write((output>>7U)&1U);pins|=(uint8_t)(yidap_data_read()<<1U);pins|=(uint8_t)(yidap_tdo_read()<<3U);pins|=(uint8_t)(yidap_reset_read()<<7U);response[1]=pins;return 2U;}
    if(command==CMD_SWJ_CLOCK){value=get_u32(&request[1]);if(value>=1000U&&value<=10000000U){g_clock_delay=yidap_port_cpu_clock_hz()/(value*6U);if(g_clock_delay==0U)g_clock_delay=1U;response[1]=0U;}return 2U;}
    if(command==CMD_SWJ_SEQUENCE){uint16_t bits=request[1];if(bits==0U)bits=256U;for(i=0U;i<bits;++i){yidap_data_write((request[2U+i/8U]>>(i&7U))&1U);(void)clock_sample(0);}response[1]=0U;return 2U;}
    if(command==CMD_SWD_CONFIGURE){response[1]=0U;return 2U;}
    if(command==CMD_SWD_SEQUENCE&&g_port==1U)return process_swd_sequence(request,response);
    if(command==CMD_JTAG_SEQUENCE&&g_port==2U)return process_jtag_sequence(request,response);
    if(command==CMD_JTAG_CONFIGURE){response[1]=0U;return 2U;}
    if(command==CMD_JTAG_IDCODE){response[1]=0U;memset(&response[2],0,4U);return 6U;}
    if(command>=0x80U&&command<=0x87U)return yidap_vendor_process(request,response);
    (void)g_match_retry;(void)g_match_mask;(void)length;return 2U;
}
