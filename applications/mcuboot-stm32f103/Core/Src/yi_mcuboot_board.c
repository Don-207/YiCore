/**
 * @file yi_mcuboot_board.c
 * @brief YiCore mcuboot board implementation.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#include "yi_mcuboot_board.h"

#include <stdint.h>

#include "bootutil/image.h"
#include "flash_map_backend/flash_map_backend.h"
#include "stm32f1xx_hal.h"
#include "yi_generated.h"
#include "yi_mcuboot_layout_stm32f103xe.h"

static yi_partition_t boot_partition;
static yi_partition_t primary_partition;
static yi_partition_t secondary_partition;
static yi_partition_t storage_partition;
static yi_partition_t scratch_partition;
static struct flash_area flash_areas[4];

/**
 * @brief Atomically replace MSP and branch to the application reset handler.
 * @param initial_sp Application vector-table stack pointer.
 * @param reset_handler Thumb reset-handler address.
 * @return This function never returns.
 * @note Naked implementation is required because no C stack access is valid
 *       after MSP changes from the bootloader stack to the application stack.
 */
__attribute__((naked, noreturn))
static void yi_mcuboot_branch(uint32_t initial_sp, uint32_t reset_handler)
{
    __asm volatile(
        "msr msp, r0\n"
        "cpsie i\n"
        "bx r1\n");
}

/**
 * @brief Set the module.
 * @param partition Partition value.
 * @param name Registered device name.
 * @param flash Flash value.
 * @param offset Byte offset from the start of the device.
 * @param size Size value.
 */
static void yi_mcuboot_partition_set(yi_partition_t *partition,
                                     const char *name,
                                     yi_device_t *flash,
                                     uint32_t offset,
                                     uint32_t size)
{
    partition->name = name;
    partition->flash = flash;
    partition->offset = offset;
    partition->size = size;
}

/**
 * @brief Set the module.
 * @param area Area value.
 * @param id Id value.
 * @param partition Partition value.
 */
static void yi_mcuboot_area_set(struct flash_area *area,
                                uint8_t id,
                                const yi_partition_t *partition)
{
    area->fa_id = id;
    area->fa_device_id = 0U;
    area->pad16 = 0U;
    area->fa_off = partition->offset;
    area->fa_size = partition->size;
    area->partition = partition;
}

/**
 * @brief Initialize the module.
 */
int yi_mcuboot_board_flash_map_init(void)
{
    yi_device_t *internal_flash = YI_DT_GET(FLASH0);

    if(!yi_device_is_ready(internal_flash))
    {
        return -1;
    }

    yi_mcuboot_partition_set(&boot_partition, "mcuboot", internal_flash,
                             YI_MCUBOOT_BOOT_OFFSET,
                             YI_MCUBOOT_BOOT_SIZE);
    yi_mcuboot_partition_set(&primary_partition, "image-0", internal_flash,
                             YI_MCUBOOT_PRIMARY_OFFSET,
                             YI_MCUBOOT_SLOT_SIZE);
    yi_mcuboot_partition_set(&secondary_partition, "image-1", internal_flash,
                             YI_MCUBOOT_SECONDARY_OFFSET,
                             YI_MCUBOOT_SLOT_SIZE);
    yi_mcuboot_partition_set(&storage_partition, "update-state", internal_flash,
                             YI_MCUBOOT_UPDATE_STATE_OFFSET,
                             YI_MCUBOOT_UPDATE_STATE_SIZE);
    yi_mcuboot_partition_set(&scratch_partition, "image-scratch", internal_flash,
                             YI_MCUBOOT_SCRATCH_OFFSET,
                             YI_MCUBOOT_SCRATCH_SIZE);

    if((yi_partition_validate(&storage_partition) != 0) ||
       (yi_partition_validate(&scratch_partition) != 0))
    {
        return -1;
    }

    yi_mcuboot_area_set(&flash_areas[0], FLASH_AREA_BOOTLOADER,
                        &boot_partition);
    yi_mcuboot_area_set(&flash_areas[1], FLASH_AREA_IMAGE_PRIMARY(0),
                        &primary_partition);
    yi_mcuboot_area_set(&flash_areas[2], FLASH_AREA_IMAGE_SECONDARY(0),
                        &secondary_partition);
    yi_mcuboot_area_set(&flash_areas[3], FLASH_AREA_IMAGE_SCRATCH,
                        &scratch_partition);

    /**
     * @brief Set the module.
     * @param flash_areas Flash areas value.
     */
    return yi_mcuboot_flash_map_set(flash_areas,
                                    sizeof(flash_areas) /
                                    sizeof(flash_areas[0]));
}

/**
 * @brief Perform the yi mcuboot jump operation.
 * @param response Response value.
 */
void yi_mcuboot_jump(const struct boot_rsp *response)
{
    uint32_t vector_address;
    uint32_t initial_sp;
    uint32_t reset_handler;

    if((response == NULL) || (response->br_hdr == NULL) ||
       (response->br_flash_dev_id != 0U) ||
       (response->br_image_off != YI_MCUBOOT_PRIMARY_OFFSET) ||
       (response->br_hdr->ih_hdr_size != YI_MCUBOOT_IMAGE_HEADER_SIZE))
    {
        return;
    }

    vector_address = YI_MCUBOOT_FLASH_BASE + response->br_image_off +
                     response->br_hdr->ih_hdr_size;
    initial_sp = *(const uint32_t *)vector_address;
    reset_handler = *(const uint32_t *)(vector_address + sizeof(uint32_t));

    if((initial_sp < SRAM_BASE) || (initial_sp > (SRAM_BASE + 0x0000C000U)) ||
       ((initial_sp & 0x7U) != 0U) || ((reset_handler & 1U) == 0U) ||
       ((reset_handler & ~1U) < vector_address) ||
       ((reset_handler & ~1U) >= (YI_MCUBOOT_FLASH_BASE +
                                  YI_MCUBOOT_PRIMARY_OFFSET +
                                  YI_MCUBOOT_SLOT_SIZE)))
    {
        return;
    }

    __disable_irq();
    SysTick->CTRL = 0U;
    SysTick->LOAD = 0U;
    SysTick->VAL = 0U;
    SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk | SCB_ICSR_PENDSVCLR_Msk;
    NVIC->ICER[0] = 0xFFFFFFFFU;
    NVIC->ICER[1] = 0xFFFFFFFFU;
    NVIC->ICPR[0] = 0xFFFFFFFFU;
    NVIC->ICPR[1] = 0xFFFFFFFFU;
    (void)HAL_RCC_DeInit();
    (void)HAL_DeInit();
    SCB->VTOR = vector_address;
    __DSB();
    __ISB();

    yi_mcuboot_branch(initial_sp, reset_handler);
}
