/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __CARRELS_EVENTS_H__
#define __CARRELS_EVENTS_H__

#include <microkit.h>
#include <stdbool.h>

typedef void (*carrels_event_handler_t)(microkit_channel ch, void *arg);

enum carrels_event_error {
	CARRELS_EVENT_OK = 0,
	CARRELS_EVENT_EINVAL = -1,
	CARRELS_EVENT_EBUSY = -2,
	CARRELS_EVENT_ENOENT = -3,
};

int carrels_event_register(microkit_channel ch,
			   carrels_event_handler_t handler,
			   void *arg);

int carrels_event_unregister(microkit_channel ch,
			     carrels_event_handler_t handler,
			     void *arg);

void carrels_dispatch_event_to(microkit_channel ch);

void uk_carrels_microkit_wait(void);
/* Consume one channel's notification while dispatching other channels. */
void uk_carrels_microkit_wait_on(seL4_Word channel_badge);
bool uk_carrels_microkit_poll(void);

#endif /* __CARRELS_EVENTS_H__ */
