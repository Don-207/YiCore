/**
 * @file yi_poll.c
 * @brief Bare-metal event polling and idle implementation.
 * @author Don
 * @date 2026-08-02
 * @version 1.0.0
 */

#include "yi_poll.h"
#include <stddef.h>

#define YI_POLL_MAX_EVENTS 16U
#define YI_POLL_MAX_HOOKS 8U

typedef struct
{
    yi_poll_hook_t hook; /**< Registered non-blocking service function. */
    void *user_data; /**< Caller-owned service context. */
} yi_poll_hook_entry_t;

static yi_event_t *yi_events[YI_POLL_MAX_EVENTS];
static yi_poll_hook_entry_t yi_hooks[YI_POLL_MAX_HOOKS];
static uint8_t yi_event_count;
static uint8_t yi_hook_count;
static bool yi_work_observed;

static void yi_event_register(yi_event_t *event)
{
    uint8_t index;

    for(index = 0U; index < yi_event_count; index++)
    {
        if(yi_events[index] == event)
        {
            return;
        }
    }
    if(yi_event_count < YI_POLL_MAX_EVENTS)
    {
        yi_events[yi_event_count++] = event;
    }
}

void yi_event_init(yi_event_t *event, yi_event_handler_t handler,
                   void *user_data)
{
    if(event == NULL)
    {
        return;
    }
    event->pending = 0U;
    event->handler = handler;
    event->user_data = user_data;
    yi_event_register(event);
}

void yi_event_post(yi_event_t *event, uint32_t events)
{
    if((event != NULL) && (events != 0U))
    {
        __atomic_fetch_or(&event->pending, events, __ATOMIC_RELEASE);
    }
}

bool yi_event_pending(const yi_event_t *event)
{
    return (event != NULL) &&
           (__atomic_load_n(&event->pending, __ATOMIC_ACQUIRE) != 0U);
}

int yi_poll_hook_register(yi_poll_hook_t hook, void *user_data)
{
    if((hook == NULL) || (yi_hook_count >= YI_POLL_MAX_HOOKS))
    {
        return -1;
    }
    yi_hooks[yi_hook_count].hook = hook;
    yi_hooks[yi_hook_count].user_data = user_data;
    yi_hook_count++;
    return 0;
}

bool yi_poll(void)
{
    uint8_t index;
    bool work = false;

    for(index = 0U; index < yi_event_count; index++)
    {
        yi_event_t *event = yi_events[index];
        uint32_t pending = __atomic_exchange_n(
            &event->pending, 0U, __ATOMIC_ACQ_REL);

        if(pending != 0U)
        {
            work = true;
            if(event->handler != NULL)
            {
                event->handler(pending, event->user_data);
            }
        }
    }
    for(index = 0U; index < yi_hook_count; index++)
    {
        if(yi_hooks[index].hook(yi_hooks[index].user_data))
        {
            work = true;
        }
    }
    yi_work_observed = work;
    return work;
}

void yi_idle(void)
{
    uint8_t index;

    if(yi_work_observed)
    {
        yi_work_observed = false;
        return;
    }
    for(index = 0U; index < yi_event_count; index++)
    {
        if(yi_event_pending(yi_events[index]))
        {
            return;
        }
    }
    yi_arch_idle();
}

#if defined(__GNUC__)
__attribute__((weak))
#endif
void yi_arch_idle(void)
{
#if defined(__arm__) || defined(__thumb__)
    __asm volatile("wfi");
#elif defined(__riscv)
    __asm volatile("wfi");
#endif
}
