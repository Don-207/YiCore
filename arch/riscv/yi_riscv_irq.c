/**
 * @file yi_riscv_irq.c
 * @brief Implement RV32 machine-mode interrupt save and restore.
 * @author Don
 * @date 2026-07-29
 * @version 1.0.0
 */

#include "yi_riscv_irq.h"

/**
 * @brief Atomically clear mstatus.MIE and capture its previous state.
 * @return Previous mstatus.MIE value, masked to YI_RISCV_MSTATUS_MIE.
 * @note Executes in the caller context and does not alter individual IRQs.
 */
uint32_t yi_riscv_irq_lock(void)
{
    /** Complete mstatus value observed before MIE is cleared. */
    uint32_t previous_status;

    __asm volatile(
        "csrrc %0, mstatus, %1"
        : "=r"(previous_status)
        : "r"(YI_RISCV_MSTATUS_MIE)
        : "memory"
    );
    return previous_status & YI_RISCV_MSTATUS_MIE;
}

/**
 * @brief Restore mstatus.MIE only when the saved key had MIE enabled.
 * @param key Previous MIE state returned by yi_riscv_irq_lock().
 * @return None.
 * @note Executes in the caller context and preserves nested critical sections.
 */
void yi_riscv_irq_unlock(uint32_t key)
{
    if ((key & YI_RISCV_MSTATUS_MIE) != 0U) {
        __asm volatile(
            "csrs mstatus, %0"
            :
            : "r"(YI_RISCV_MSTATUS_MIE)
            : "memory"
        );
    }
}
