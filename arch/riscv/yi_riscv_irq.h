/**
 * @file yi_riscv_irq.h
 * @brief Provide vendor-neutral RV32 machine-mode interrupt primitives.
 * @author Don
 * @date 2026-07-29
 * @version 1.0.0
 */

#ifndef YI_RISCV_IRQ_H
#define YI_RISCV_IRQ_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Machine interrupt-enable bit in the RISC-V mstatus CSR. */
#define YI_RISCV_MSTATUS_MIE (1UL << 3)

/**
 * @brief Disable machine-mode interrupts and return the previous MIE state.
 * @return Zero when interrupts were disabled, otherwise YI_RISCV_MSTATUS_MIE.
 * @note Safe in thread or interrupt context; nesting is supported by the key.
 */
uint32_t yi_riscv_irq_lock(void);

/**
 * @brief Restore machine-mode interrupt enable state saved by irq_lock.
 * @param key Previous MIE state returned by yi_riscv_irq_lock().
 * @return None.
 * @note Safe in thread or interrupt context; pass the matching nesting key.
 */
void yi_riscv_irq_unlock(uint32_t key);

/**
 * @brief Order all preceding memory accesses before following accesses.
 * @return None.
 * @note Emits a full RISC-V fence and has no peripheral side effects.
 */
static inline void yi_riscv_memory_barrier(void)
{
    __asm volatile("fence iorw, iorw" ::: "memory");
}

/**
 * @brief Synchronize instruction fetch after executable memory changes.
 * @return None.
 * @note Required after modifying code or instruction-visible mappings.
 */
static inline void yi_riscv_instruction_barrier(void)
{
    __asm volatile("fence.i" ::: "memory");
}

#ifdef __cplusplus
}
#endif

#endif /* YI_RISCV_IRQ_H */
