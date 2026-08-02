/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __CARRELS_EVENTS_H__
#define __CARRELS_EVENTS_H__

#include <microkit.h>

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

void carrels_event_dispatch(microkit_channel ch);

void uk_carrels_microkit_wait(void);

#endif /* __CARRELS_EVENTS_H__ */