/**
 * @file yi_soft_timer.h
 * @brief YiCore soft timer interface.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#ifndef YI_SOFT_TIMER_H
#define YI_SOFT_TIMER_H

#include <stdbool.h>
#include <stdint.h>

typedef struct yi_soft_timer yi_soft_timer_t;

/**
 * @brief Perform the void operation.
 * @param timer Timer value.
 * @param user_data User data value.
 */
typedef void (*yi_soft_timer_callback_t)(yi_soft_timer_t *timer,
                                         void *user_data);

struct yi_soft_timer
{
    yi_soft_timer_t *next; /**< Next value. */
    yi_soft_timer_callback_t callback; /**< Callback value. */
    void *user_data; /**< User data value. */
    uint32_t deadline_ms; /**< Deadline ms value. */
    uint32_t period_ms; /**< Period ms value. */
    bool active; /**< Active value. */
    bool pending; /**< Pending value. */
    bool registered; /**< Registered value. */};

/*
 * Software timer callbacks run from yi_soft_timer_process(), not from SysTick.
 * Timer objects must remain allocated after they have been started.
 * These APIs are intended for main-loop context and are not interrupt-safe.
 */
void yi_soft_timer_init(yi_soft_timer_t *timer,
                        yi_soft_timer_callback_t callback,
                        void *user_data);
/**
 * @brief Start the module.
 * @param timer Timer value.
 * @param delay_ms Delay ms value.
 * @param period_ms Period ms value.
 */
int yi_soft_timer_start(yi_soft_timer_t *timer,
                        uint32_t delay_ms,
                        uint32_t period_ms);
/**
 * @brief Stop the module.
 * @param timer Timer value.
 */
int yi_soft_timer_stop(yi_soft_timer_t *timer);
/**
 * @brief Check whether active.
 * @param timer Timer value.
 */
bool yi_soft_timer_is_active(const yi_soft_timer_t *timer);
/**
 * @brief Perform the yi soft timer process operation.
 */
void yi_soft_timer_process(void);

#endif
