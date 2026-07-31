/**
 * @file yidap_ch32h417_usb.c
 * @brief Configure CH32H417 clocks and interrupts for CherryUSB USBFS.
 * @author Don
 * @date 2026-08-01
 * @version 1.0.0
 */

#include "ch32h417.h"

/**
 * @brief Enable a 48 MHz USBFS clock derived from the 480 MHz USBHS PLL.
 * @note Called once from CherryUSB initialization; changes RCC and USBFS IRQ state.
 */
void usb_dc_low_level_init(void)
{
    if ((RCC->PLLCFGR & RCC_SYSPLL_SEL) != RCC_SYSPLL_USBHS) {
        RCC_USBHS_PLLCmd(DISABLE);
        RCC_USBHSPLLCLKConfig((RCC->CTLR & RCC_HSERDY) != 0U
                                  ? RCC_USBHSPLLSource_HSE
                                  : RCC_USBHSPLLSource_HSI);
        RCC_USBHSPLLReferConfig(RCC_USBHSPLLRefer_25M);
        RCC_USBHSPLLClockSourceDivConfig(RCC_USBHSPLL_IN_Div1);
        RCC_USBHS_PLLCmd(ENABLE);
        while ((RCC->CTLR & RCC_USBHS_PLLRDY) == 0U) {
        }
    }

    RCC_USBFSCLKConfig(RCC_USBFSCLKSource_USBHSPLL);
    RCC_USBFS48ClockSourceDivConfig(RCC_USBFS_Div10);
    RCC_HBPeriphClockCmd(RCC_HBPeriph_OTG_FS, ENABLE);
    RCC_HB2PeriphClockCmd(RCC_HB2Periph_GPIOA, ENABLE);
    NVIC_SetPriority(USBFS_IRQn, 2U);
    NVIC_EnableIRQ(USBFS_IRQn);
}

/**
 * @brief Disable the USBFS interrupt and peripheral clock.
 * @note Used only if the CherryUSB controller is deinitialized.
 */
void usb_dc_low_level_deinit(void)
{
    NVIC_DisableIRQ(USBFS_IRQn);
    RCC_HBPeriphClockCmd(RCC_HBPeriph_OTG_FS, DISABLE);
}
