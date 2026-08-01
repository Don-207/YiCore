/**
 * @file yidap_ch32h417_usb_device.c
 * @brief Implement a native CMSIS-DAP v2 bulk device on CherryUSB.
 * @author Don
 * @date 2026-08-01
 * @version 1.0.0
 */
#include "yidap_ch32h417_usb.h"
#include "yi_dap_protocol.h"
#include "usbd_core.h"

/** USB device controller index used by the single CH32H417 USBFS instance. */
#define YIDAP_USB_BUS 0U
/** CH32H417 USBFS register base passed to CherryUSB. */
#define YIDAP_USB_BASE 0x40023400UL
/** CMSIS-DAP v2 bulk OUT endpoint. */
#define YIDAP_OUT_EP 0x02U
/** CMSIS-DAP v2 bulk IN endpoint. */
#define YIDAP_IN_EP 0x81U
/** USBFS maximum packet size and YiDAP protocol packet size. */
#define YIDAP_PACKET_SIZE 64U
/** Total bytes in one-interface configuration descriptor. */
#define YIDAP_CONFIG_SIZE 32U

/** Device descriptor identifying the independent YiDAP implementation. */
static const uint8_t g_device_descriptor[]={USB_DEVICE_DESCRIPTOR_INIT(USB_2_1,0x00,0x00,0x00,0x1209,0x4717,0x0100,0x01)};
/** Configuration containing one vendor-specific CMSIS-DAP bulk interface. */
static const uint8_t g_config_descriptor[]={USB_CONFIG_DESCRIPTOR_INIT(YIDAP_CONFIG_SIZE,0x01,0x01,USB_CONFIG_BUS_POWERED,100),USB_INTERFACE_DESCRIPTOR_INIT(0x00,0x00,0x02,0xFF,0x00,0x00,0x02),USB_ENDPOINT_DESCRIPTOR_INIT(YIDAP_OUT_EP,USB_ENDPOINT_TYPE_BULK,YIDAP_PACKET_SIZE,0x00),USB_ENDPOINT_DESCRIPTOR_INIT(YIDAP_IN_EP,USB_ENDPOINT_TYPE_BULK,YIDAP_PACKET_SIZE,0x00)};
/** USB strings indexed by the device and interface descriptors. */
static const char *g_strings[]={(const char[]){0x09,0x04},"YiLink","YiDAP CH32H417","YIDAP-CH32H417"};
/** Microsoft OS 2.0 descriptor set binding interface zero to WinUSB. */
static const uint8_t g_ms_os_20[]={0x0A,0x00,0x00,0x00,0x00,0x00,0x03,0x06,0xAC,0x00,0x08,0x00,0x02,0x00,0x00,0x00,0xA2,0x00,0x14,0x00,0x03,0x00,'W','I','N','U','S','B',0,0,0,0,0,0,0,0,0,0,0,0,0x84,0x00,0x04,0x00,0x07,0x00,0x2A,0x00,'D',0,'e',0,'v',0,'i',0,'c',0,'e',0,'I',0,'n',0,'t',0,'e',0,'r',0,'f',0,'a',0,'c',0,'e',0,'G',0,'U',0,'I',0,'D',0,'s',0,0,0,0x50,0x00,0x7B,0x00,'C',0,'D',0,'B',0,'3',0,'B',0,'5',0,'A',0,'D',0,'-',0,'2',0,'9',0,'3',0,'B',0,'-',0,'4',0,'6',0,'6',0,'3',0,'-',0,'A',0,'A',0,'3',0,'6',0,'-',0,'1',0,'A',0,'A',0,'E',0,'4',0,'6',0,'4',0,'6',0,'3',0,'7',0,'7',0,'6',0,0x7D,0x00,0x00,0x00,0x00,0x00};
/** BOS platform capability advertising Microsoft OS 2.0 support. */
static const uint8_t g_bos[]={0x05,0x0F,0x21,0x00,0x01,0x1C,0x10,0x05,0x00,0xDF,0x60,0xDD,0xD8,0x89,0x45,0xC7,0x4C,0x9C,0xD2,0x65,0x9D,0x9E,0x64,0x8A,0x9F,0x00,0x00,0x03,0x06,0xAC,0x00,0x20,0x00};
/** DMA-safe request packet owned by the USB OUT endpoint. */
static USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t g_request[YIDAP_PACKET_SIZE];
/** DMA-safe response packet owned by the USB IN endpoint. */
static USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t g_response[YIDAP_PACKET_SIZE];
/** Set by the OUT callback and cleared after foreground command processing. */
static volatile uint8_t g_request_pending;
/** Set while the IN endpoint owns g_response. */
static volatile uint8_t g_response_pending;
/** Number of response bytes submitted to the IN endpoint. */
static uint16_t g_response_length;
/** Return the full-speed device descriptor. */
static const uint8_t *device_desc(uint8_t speed){(void)speed;return g_device_descriptor;}
/** Return the full-speed configuration descriptor. */
static const uint8_t *config_desc(uint8_t speed){(void)speed;return g_config_descriptor;}
/** Return a USB string or NULL for an unsupported index. */
static const char *string_desc(uint8_t speed,uint8_t index){(void)speed;return(index<(sizeof(g_strings)/sizeof(g_strings[0])))?g_strings[index]:0;}
/** Descriptor callback table consumed by CherryUSB only as a USB stack. */
/** CherryUSB wrapper for the WinUSB compatible-ID descriptor. */
static struct usb_msosv2_descriptor g_msos={.vendor_code=0x20U,.compat_id=g_ms_os_20,.compat_id_len=sizeof(g_ms_os_20)};
/** CherryUSB wrapper for the binary object store descriptor. */
static struct usb_bos_descriptor g_bos_descriptor={.string=g_bos,.string_len=sizeof(g_bos)};
/** Descriptor callback table consumed by CherryUSB only as a USB stack. */
static const struct usb_descriptor g_descriptor={.device_descriptor_callback=device_desc,.config_descriptor_callback=config_desc,.string_descriptor_callback=string_desc,.bos_descriptor=&g_bos_descriptor,.msosv2_descriptor=&g_msos};
/** Mark one received request for foreground processing; never parses in IRQ. */
static void out_callback(uint8_t busid,uint8_t ep,uint32_t nbytes){(void)busid;(void)ep;if(nbytes!=0U)g_request_pending=1U;}
/** Release the response buffer after the host consumes the IN transfer. */
static void in_callback(uint8_t busid,uint8_t ep,uint32_t nbytes){(void)busid;(void)ep;(void)nbytes;g_response_pending=0U;}
/** Bulk endpoint registrations retained for the USB device lifetime. */
static struct usbd_endpoint g_out_endpoint={.ep_addr=YIDAP_OUT_EP,.ep_cb=out_callback};
static struct usbd_endpoint g_in_endpoint={.ep_addr=YIDAP_IN_EP,.ep_cb=in_callback};
/** Vendor-specific CMSIS-DAP interface retained for the USB device lifetime. */
static struct usbd_interface g_interface;
/** Arm the first request after enumeration and reset queue state on disconnect. */
static void event_handler(uint8_t busid,uint8_t event){(void)busid;if(event==USBD_EVENT_CONFIGURED){g_request_pending=0U;g_response_pending=0U;(void)usbd_ep_start_read(YIDAP_USB_BUS,YIDAP_OUT_EP,g_request,YIDAP_PACKET_SIZE);}else if(event==USBD_EVENT_RESET||event==USBD_EVENT_DISCONNECTED){g_request_pending=0U;g_response_pending=0U;}}
/** Register descriptors/endpoints and initialize USBFS. */
int yidap_ch32h417_usb_init(void){yi_dap_protocol_init();usbd_desc_register(YIDAP_USB_BUS,&g_descriptor);usbd_add_interface(YIDAP_USB_BUS,&g_interface);usbd_add_endpoint(YIDAP_USB_BUS,&g_out_endpoint);usbd_add_endpoint(YIDAP_USB_BUS,&g_in_endpoint);return usbd_initialize(YIDAP_USB_BUS,YIDAP_USB_BASE,event_handler);}
/** Parse one request and submit one response in foreground context. */
void yidap_ch32h417_usb_process(void){if((g_request_pending!=0U)&&(g_response_pending==0U)){g_request_pending=0U;g_response_length=yi_dap_protocol_process(g_request,g_response);if(g_response_length>YIDAP_PACKET_SIZE)g_response_length=YIDAP_PACKET_SIZE;g_response_pending=1U;(void)usbd_ep_start_write(YIDAP_USB_BUS,YIDAP_IN_EP,g_response,g_response_length);(void)usbd_ep_start_read(YIDAP_USB_BUS,YIDAP_OUT_EP,g_request,YIDAP_PACKET_SIZE);}}
