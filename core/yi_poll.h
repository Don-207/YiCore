/**
 * @file yi_poll.h
 * @brief Bare-metal event polling and idle interface.
 * @author Don
 * @date 2026-08-02
 * @version 1.0.0
 */

#ifndef YI_POLL_H
#define YI_POLL_H

#include <stdbool.h>
#include <stdint.h>

typedef void (*yi_event_handler_t)(uint32_t events, void *user_data);
typedef bool (*yi_poll_hook_t)(void *user_data);

typedef struct
{
    volatile uint32_t pending; /**< Event bits shared with interrupt context. */
    yi_event_handler_t handler; /**< Main-loop callback for pending bits. */
    void *user_data; /**< Caller-owned callback context. */
} yi_event_t;

void yi_event_init(yi_event_t *event, yi_event_handler_t handler,
                   void *user_data);
void yi_event_post(yi_event_t *event, uint32_t events);
bool yi_event_pending(const yi_event_t *event);
int yi_poll_hook_register(yi_poll_hook_t hook, void *user_data);
bool yi_poll(void);
void yi_idle(void);
void yi_arch_idle(void);

#endif
