/**
 * @file mcuboot_config.h
 * @brief YiCore mcuboot config interface.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#ifndef YI_MCUBOOT_CONFIG_H
#define YI_MCUBOOT_CONFIG_H

/* Initial YiCore profile: one image, overwrite-only upgrade, EC-P256 signing. */
#define MCUBOOT_SIGN_EC256
#define MCUBOOT_USE_TINYCRYPT
#define MCUBOOT_OVERWRITE_ONLY
#define MCUBOOT_VALIDATE_PRIMARY_SLOT

#define MCUBOOT_USE_FLASH_AREA_GET_SECTORS
#define MCUBOOT_MAX_IMG_SECTORS 64
#define MCUBOOT_IMAGE_NUMBER 1

#define MCUBOOT_CPU_IDLE() do { } while (0)

#endif
