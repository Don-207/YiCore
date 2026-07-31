/**
 * @file usb_config.h
 * @brief Configure CherryUSB for the CH32H417 USBFS YiDAP device.
 * @author Don
 * @date 2026-08-01
 * @version 1.0.0
 */

#ifndef YIDAP_CH32H417_USB_CONFIG_H
#define YIDAP_CH32H417_USB_CONFIG_H

/** DMA buffers require native word alignment on CH32H417. */
#define CONFIG_USB_ALIGN_SIZE 4
/** Keep USB logging disabled in the standalone probe image. */
#define CONFIG_USB_DBG_LEVEL 0
/** Discard CherryUSB diagnostic output without pulling in stdio. */
#define CONFIG_USB_PRINTF(...) ((void)0)
/** Place DMA buffers in ordinary SRAM, which is coherent on the V3F core. */
#define USB_NOCACHE_RAM_SECTION
/** One USB device controller is exposed to CherryUSB. */
#define CONFIG_USBDEV_MAX_BUS 1
/** USBFS provides endpoint indices zero through seven. */
#define CONFIG_USBDEV_EP_NUM 8
/** Use CherryDAP's advanced descriptor callback table. */
#define CONFIG_USBDEV_ADVANCE_DESC
/** Control endpoint scratch space in bytes. */
#define CONFIG_USBDEV_REQUEST_BUFFER_LEN 512

#endif
