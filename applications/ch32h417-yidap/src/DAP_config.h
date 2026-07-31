/**
 * @file DAP_config.h
 * @brief Bind CMSIS-DAP GPIO signaling to the CH32H417 evaluation board.
 * @author Don
 * @date 2026-08-01
 * @version 1.0.0
 */

#ifndef YIDAP_CH32H417_DAP_CONFIG_H
#define YIDAP_CH32H417_DAP_CONFIG_H

#include <stdint.h>
#include <string.h>

#include "ch32h417.h"

#ifndef __STATIC_INLINE
#define __STATIC_INLINE static inline
#endif
#ifndef __STATIC_FORCEINLINE
#define __STATIC_FORCEINLINE __attribute__((always_inline)) static inline
#endif
#ifndef __WEAK
#define __WEAK __attribute__((weak))
#endif

/** V3F core clock used by the CMSIS-DAP delay calculation. */
#define CPU_CLOCK 120000000U
/** Approximate cycles required for one direct GPIO register write. */
#define IO_PORT_WRITE_CYCLES 2U
/** Enable Serial Wire Debug commands. */
#define DAP_SWD 1
/** Enable JTAG commands. */
#define DAP_JTAG 1
/** Maximum number of devices accepted in one JTAG chain. */
#define DAP_JTAG_DEV_CNT 8U
/** Prefer SWD when the host requests the default port. */
#define DAP_DEFAULT_PORT 1U
/** Conservative default debug clock for GPIO signaling. */
#define DAP_DEFAULT_SWJ_CLOCK 1000000U
/** USBFS CMSIS-DAP packet size in bytes. */
#define DAP_PACKET_SIZE 64U
/** Number of queued CMSIS-DAP request and response packets. */
#define DAP_PACKET_COUNT 4U
/** Disable UART SWO capture in the first CH32H417 port. */
#define SWO_UART 0
/** Placeholder SWO UART driver index. */
#define SWO_UART_DRIVER 0
/** Maximum advertised SWO rate when UART SWO is disabled. */
#define SWO_UART_MAX_BAUDRATE 0U
/** Disable Manchester SWO capture. */
#define SWO_MANCHESTER 0
/** Reserved SWO buffer size required by the CMSIS-DAP sources. */
#define SWO_BUFFER_SIZE 256U
/** Disable streaming SWO. */
#define SWO_STREAM 0
/** Disable CMSIS-DAP timestamps. */
#define TIMESTAMP_CLOCK 0U
/** Disable the CMSIS-DAP UART command extension. */
#define DAP_UART 0
/** Placeholder CMSIS-DAP UART driver index. */
#define DAP_UART_DRIVER 0
/** Reserved UART receive buffer size. */
#define DAP_UART_RX_BUFFER_SIZE 64U
/** Reserved UART transmit buffer size. */
#define DAP_UART_TX_BUFFER_SIZE 64U
/** CherryDAP exposes its separate USB CDC interface. */
#define DAP_UART_USB_COM_PORT 1
/** The probe may connect to arbitrary target devices. */
#define TARGET_FIXED 0

/** Target debug GPIO port borrowed from the HPM5301 PA4..PA8 layout. */
#define YIDAP_GPIO_PORT GPIOA
/** JTAG TDO input on PA4. */
#define YIDAP_PIN_TDO GPIO_Pin_4
/** JTAG TDI output on PA5. */
#define YIDAP_PIN_TDI GPIO_Pin_5
/** SWCLK/JTAG TCK output on PA6. */
#define YIDAP_PIN_TCK GPIO_Pin_6
/** SWDIO/JTAG TMS bidirectional signal on PA7. */
#define YIDAP_PIN_TMS GPIO_Pin_7
/** Active-low target reset output on PA8. */
#define YIDAP_PIN_RESET GPIO_Pin_8

/** Return the YiDAP vendor string to the host. */
__STATIC_INLINE uint8_t DAP_GetVendorString(char *str)
{
    memcpy(str, "YiLink", 7U);
    return 7U;
}

/** Return the CH32H417 probe product string to the host. */
__STATIC_INLINE uint8_t DAP_GetProductString(char *str)
{
    memcpy(str, "YiDAP CH32H417", 15U);
    return 15U;
}

/** Return no application-managed serial number. */
__STATIC_INLINE uint8_t DAP_GetSerNumString(char *str)
{
    (void)str;
    return 0U;
}

/** Return no fixed target vendor string. */
__STATIC_INLINE uint8_t DAP_GetTargetDeviceVendorString(char *str) { (void)str; return 0U; }
/** Return no fixed target device string. */
__STATIC_INLINE uint8_t DAP_GetTargetDeviceNameString(char *str) { (void)str; return 0U; }
/** Return no fixed target board vendor string. */
__STATIC_INLINE uint8_t DAP_GetTargetBoardVendorString(char *str) { (void)str; return 0U; }
/** Return no fixed target board string. */
__STATIC_INLINE uint8_t DAP_GetTargetBoardNameString(char *str) { (void)str; return 0U; }
/** Return the initial CH32H417 YiDAP firmware version. */
__STATIC_INLINE uint8_t DAP_GetProductFirmwareVersionString(char *str)
{
    memcpy(str, "1.0.0", 6U);
    return 6U;
}

/** Configure the common debug pins for JTAG signaling. */
__STATIC_INLINE void PORT_JTAG_SETUP(void)
{
    GPIO_InitTypeDef output = {0};
    GPIO_InitTypeDef input = {0};
    GPIO_InitTypeDef reset = {0};

    RCC_HB2PeriphClockCmd(RCC_HB2Periph_GPIOA, ENABLE);
    output.GPIO_Pin = YIDAP_PIN_TCK | YIDAP_PIN_TMS | YIDAP_PIN_TDI;
    output.GPIO_Speed = GPIO_Speed_Very_High;
    output.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(YIDAP_GPIO_PORT, &output);
    input.GPIO_Pin = YIDAP_PIN_TDO;
    input.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(YIDAP_GPIO_PORT, &input);
    reset.GPIO_Pin = YIDAP_PIN_RESET;
    reset.GPIO_Speed = GPIO_Speed_Very_High;
    reset.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_Init(YIDAP_GPIO_PORT, &reset);
    GPIO_SetBits(YIDAP_GPIO_PORT, YIDAP_PIN_TCK | YIDAP_PIN_TMS | YIDAP_PIN_TDI);
    GPIO_SetBits(YIDAP_GPIO_PORT, YIDAP_PIN_RESET);
}

/** Configure PA6 and PA7 for GPIO-based SWD signaling. */
__STATIC_INLINE void PORT_SWD_SETUP(void)
{
    GPIO_InitTypeDef output = {0};
    GPIO_InitTypeDef unused = {0};
    GPIO_InitTypeDef reset = {0};

    RCC_HB2PeriphClockCmd(RCC_HB2Periph_GPIOA, ENABLE);
    output.GPIO_Pin = YIDAP_PIN_TCK | YIDAP_PIN_TMS;
    output.GPIO_Speed = GPIO_Speed_Very_High;
    output.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(YIDAP_GPIO_PORT, &output);
    unused.GPIO_Pin = YIDAP_PIN_TDO | YIDAP_PIN_TDI;
    unused.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(YIDAP_GPIO_PORT, &unused);
    reset.GPIO_Pin = YIDAP_PIN_RESET;
    reset.GPIO_Speed = GPIO_Speed_Very_High;
    reset.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_Init(YIDAP_GPIO_PORT, &reset);
    GPIO_SetBits(YIDAP_GPIO_PORT, YIDAP_PIN_TCK | YIDAP_PIN_TMS);
    GPIO_SetBits(YIDAP_GPIO_PORT, YIDAP_PIN_RESET);
}

/** Return all target-facing pins to floating inputs. */
__STATIC_INLINE void PORT_OFF(void)
{
    GPIO_InitTypeDef input = {0};

    input.GPIO_Pin = YIDAP_PIN_TDO | YIDAP_PIN_TDI | YIDAP_PIN_TCK |
                     YIDAP_PIN_TMS | YIDAP_PIN_RESET;
    input.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(YIDAP_GPIO_PORT, &input);
}

/** Sample the SWCLK/TCK input level. */
__STATIC_FORCEINLINE uint32_t PIN_SWCLK_TCK_IN(void) { return GPIO_ReadInputDataBit(YIDAP_GPIO_PORT, YIDAP_PIN_TCK); }
/** Drive SWCLK/TCK high. */
__STATIC_FORCEINLINE void PIN_SWCLK_TCK_SET(void) { YIDAP_GPIO_PORT->BSHR = YIDAP_PIN_TCK; }
/** Drive SWCLK/TCK low. */
__STATIC_FORCEINLINE void PIN_SWCLK_TCK_CLR(void) { YIDAP_GPIO_PORT->BCR = YIDAP_PIN_TCK; }
/** Sample SWDIO/TMS. */
__STATIC_FORCEINLINE uint32_t PIN_SWDIO_TMS_IN(void) { return GPIO_ReadInputDataBit(YIDAP_GPIO_PORT, YIDAP_PIN_TMS); }
/** Drive SWDIO/TMS high. */
__STATIC_FORCEINLINE void PIN_SWDIO_TMS_SET(void) { YIDAP_GPIO_PORT->BSHR = YIDAP_PIN_TMS; }
/** Drive SWDIO/TMS low. */
__STATIC_FORCEINLINE void PIN_SWDIO_TMS_CLR(void) { YIDAP_GPIO_PORT->BCR = YIDAP_PIN_TMS; }
/** Sample JTAG TDI for SWJ pin commands. */
__STATIC_FORCEINLINE uint32_t PIN_TDI_IN(void) { return GPIO_ReadInputDataBit(YIDAP_GPIO_PORT, YIDAP_PIN_TDI); }
/** Drive JTAG TDI high. */
__STATIC_FORCEINLINE void PIN_TDI_OUT(uint32_t bit) { GPIO_WriteBit(YIDAP_GPIO_PORT, YIDAP_PIN_TDI, bit ? Bit_SET : Bit_RESET); }
/** Sample JTAG TDO. */
__STATIC_FORCEINLINE uint32_t PIN_TDO_IN(void) { return GPIO_ReadInputDataBit(YIDAP_GPIO_PORT, YIDAP_PIN_TDO); }
/** Sample target reset. */
__STATIC_FORCEINLINE uint32_t PIN_nRESET_IN(void) { return GPIO_ReadInputDataBit(YIDAP_GPIO_PORT, YIDAP_PIN_RESET); }
/** Drive the open-drain target reset signal. */
__STATIC_FORCEINLINE void PIN_nRESET_OUT(uint32_t bit) { GPIO_WriteBit(YIDAP_GPIO_PORT, YIDAP_PIN_RESET, bit ? Bit_SET : Bit_RESET); }
/** Sample the shared reset line for JTAG TRST commands. */
__STATIC_FORCEINLINE uint32_t PIN_nTRST_IN(void) { return PIN_nRESET_IN(); }
/** Drive the shared reset line for JTAG TRST commands. */
__STATIC_FORCEINLINE void PIN_nTRST_OUT(uint32_t bit) { PIN_nRESET_OUT(bit); }
/** Sample SWDIO during input phases. */
__STATIC_FORCEINLINE uint32_t PIN_SWDIO_IN(void) { return PIN_SWDIO_TMS_IN(); }
/** Drive SWDIO during output phases. */
__STATIC_FORCEINLINE void PIN_SWDIO_OUT(uint32_t bit) { GPIO_WriteBit(YIDAP_GPIO_PORT, YIDAP_PIN_TMS, bit ? Bit_SET : Bit_RESET); }

/** Switch SWDIO to a push-pull output. */
__STATIC_FORCEINLINE void PIN_SWDIO_OUT_ENABLE(void)
{
    GPIO_InitTypeDef gpio = {YIDAP_PIN_TMS, GPIO_Speed_Very_High, GPIO_Mode_Out_PP};
    GPIO_Init(YIDAP_GPIO_PORT, &gpio);
}

/** Switch SWDIO to a floating input. */
__STATIC_FORCEINLINE void PIN_SWDIO_OUT_DISABLE(void)
{
    GPIO_InitTypeDef gpio = {YIDAP_PIN_TMS, GPIO_Speed_Very_High, GPIO_Mode_IN_FLOATING};
    GPIO_Init(YIDAP_GPIO_PORT, &gpio);
}

/** Ignore the optional connected LED on the generic evaluation board. */
__STATIC_INLINE void LED_CONNECTED_OUT(uint32_t bit) { (void)bit; }
/** Ignore the optional activity LED on the generic evaluation board. */
__STATIC_INLINE void LED_RUNNING_OUT(uint32_t bit) { (void)bit; }
/** Return zero because timestamp capture is disabled. */
__STATIC_INLINE uint32_t TIMESTAMP_GET(void) { return 0U; }

/** Initialize target reset as released open-drain and leave the port disconnected. */
__STATIC_INLINE void DAP_SETUP(void)
{
    GPIO_InitTypeDef reset = {0};

    RCC_HB2PeriphClockCmd(RCC_HB2Periph_GPIOA, ENABLE);
    reset.GPIO_Pin = YIDAP_PIN_RESET;
    reset.GPIO_Speed = GPIO_Speed_Very_High;
    reset.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_Init(YIDAP_GPIO_PORT, &reset);
    GPIO_SetBits(YIDAP_GPIO_PORT, YIDAP_PIN_RESET);
    PORT_OFF();
}

/** No additional resources need release at DAP shutdown. */
__STATIC_INLINE void DAP_DISCONNECT(void) { PORT_OFF(); }
/** Report that no target-specific reset sequence is implemented. */
__STATIC_INLINE uint8_t RESET_TARGET(void) { return 0U; }

#endif
