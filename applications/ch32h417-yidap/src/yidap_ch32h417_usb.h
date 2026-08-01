/**
 * @file yidap_ch32h417_usb.h
 * @brief Declare the native YiDAP USBFS CMSIS-DAP v2 transport.
 * @author Don
 * @date 2026-08-01
 * @version 1.0.0
 */
#ifndef YIDAP_CH32H417_USB_H
#define YIDAP_CH32H417_USB_H
/** Initialize descriptors, bulk endpoints, and the CH32H417 USBFS device. */
int yidap_ch32h417_usb_init(void);
/** Process at most one queued request outside interrupt context. */
void yidap_ch32h417_usb_process(void);
#endif
