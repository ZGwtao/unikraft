/* SPDX-License-Identifier: BSD-3-Clause */

#include <uk/assert.h>
#include <uk/print.h>
#include <uk/microkit.h>

#include <carrels/events.h>

#define INPUT_CAP 1
#define REPLY_CAP 4

#define BADGE_FAULT_BIT    62
#define BADGE_ENDPOINT_BIT 63

struct carrels_event_entry {
	carrels_event_handler_t handler;
	void *arg;
};

static struct carrels_event_entry
	carrels_event_table[MICROKIT_MAX_CHANNELS];

int carrels_event_register(microkit_channel ch,
			   carrels_event_handler_t handler,
			   void *arg)
{
	struct carrels_event_entry *entry;

	if (ch > MICROKIT_MAX_CHANNEL_ID || handler == NULL)
		return CARRELS_EVENT_EINVAL;

	entry = &carrels_event_table[ch];

	if (entry->handler != NULL)
		return CARRELS_EVENT_EBUSY;

	entry->arg = arg;
	entry->handler = handler;

	return CARRELS_EVENT_OK;
}

int carrels_event_unregister(microkit_channel ch,
			     carrels_event_handler_t handler,
			     void *arg)
{
	struct carrels_event_entry *entry;

	if (ch > MICROKIT_MAX_CHANNEL_ID || handler == NULL)
		return CARRELS_EVENT_EINVAL;

	entry = &carrels_event_table[ch];

	if (entry->handler != handler || entry->arg != arg)
		return CARRELS_EVENT_ENOENT;

	entry->handler = NULL;
	entry->arg = NULL;

	return CARRELS_EVENT_OK;
}

void carrels_event_dispatch(microkit_channel ch)
{
	struct carrels_event_entry *entry;
	carrels_event_handler_t handler;
	void *arg;

	if (unlikely(ch > MICROKIT_MAX_CHANNEL_ID)) {
		uk_pr_warn("CARRELS: invalid notification channel %u\n",
			   (unsigned int)ch);
		return;
	}

	entry = &carrels_event_table[ch];
	handler = entry->handler;
	arg = entry->arg;

	if (unlikely(handler == NULL)) {
		uk_pr_debug("CARRELS: unhandled notification channel %u\n",
			    (unsigned int)ch);
		return;
	}

	handler(ch, arg);
}

static void carrels_dispatch_notification_badge(seL4_Word badge)
{
	unsigned int channel = 0;

	UK_ASSERT((badge >> MICROKIT_MAX_CHANNELS) == 0);

	while (badge != 0) {
		if (badge & 1UL)
			notified((microkit_channel)channel);

		badge >>= 1;
		channel++;
	}
}

void uk_carrels_microkit_wait(void)
{
	seL4_Word badge;
	seL4_MessageInfo_t tag;
	seL4_Word is_fault;
	seL4_Word is_endpoint;

	tag = seL4_Recv(INPUT_CAP, &badge, REPLY_CAP);

	(void)tag;

	is_endpoint = badge >> BADGE_ENDPOINT_BIT;
	is_fault = (badge >> BADGE_FAULT_BIT) & 1UL;

	if (unlikely(is_fault))
		UK_CRASH("CARRELS: unsupported Microkit fault");

	if (unlikely(is_endpoint))
		UK_CRASH("CARRELS: unsupported Microkit PPC");

	carrels_dispatch_notification_badge(badge);
}
