/**
 * @file yi_mcuboot_layout_stm32f103xe.h
 * @brief YiCore mcuboot layout stm32f103xe interface.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#ifndef YI_MCUBOOT_LAYOUT_STM32F103XE_H
#define YI_MCUBOOT_LAYOUT_STM32F103XE_H

/* STM32F103xE: 256 KiB internal flash, 2 KiB erase pages. */
#define YI_MCUBOOT_FLASH_BASE        0x08000000U
#define YI_MCUBOOT_FLASH_SIZE        0x00040000U
#define YI_MCUBOOT_BOOT_OFFSET       0x00000000U
#define YI_MCUBOOT_BOOT_SIZE         0x0000C000U
#define YI_MCUBOOT_PRIMARY_OFFSET    0x0000C000U
#define YI_MCUBOOT_SECONDARY_OFFSET  0x00026000U
#define YI_MCUBOOT_SLOT_SIZE         0x0001A000U
#define YI_MCUBOOT_IMAGE_HEADER_SIZE 0x00000200U
#define YI_MCUBOOT_ERASE_SIZE        0x00000800U

#if (YI_MCUBOOT_BOOT_SIZE + (2U * YI_MCUBOOT_SLOT_SIZE)) != YI_MCUBOOT_FLASH_SIZE
#error "MCUboot partitions do not fill STM32F103xE flash"
#endif

#if ((YI_MCUBOOT_BOOT_SIZE % YI_MCUBOOT_ERASE_SIZE) != 0U) || \
    ((YI_MCUBOOT_SLOT_SIZE % YI_MCUBOOT_ERASE_SIZE) != 0U)
#error "MCUboot partitions must be erase-page aligned"
#endif

#endif
