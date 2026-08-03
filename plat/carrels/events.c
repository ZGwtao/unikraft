/* SPDX-License-Identifier: BSD-3-Clause */

#include <uk/assert.h>
#include <uk/print.h>
#include <uk/sddf.h>

#include <carrels/events.h>

#define INPUT_CAP 1
#define REPLY_CAP 4

#define BADGE_FAULT_BIT    62
#define BADGE_ENDPOINT_BIT 63

static bool carrels_event_poll_enabled;

struct carrels_event_entry {
	carrels_event_handler_t handler;
	void *arg;
};

static struct carrels_event_entry
	carrels_event_table[MICROKIT_MAX_CHANNELS];


static void carrels_dispatch_badge(seL4_Word badge)
{
	UK_ASSERT((badge >> MICROKIT_MAX_CHANNELS) == 0);

	microkit_channel c = 0;

	while (badge != 0) {
		if (badge & 1UL) {
			notified(c);
		}
		badge >>= 1;
		c++;
	}
}

static bool carrels_handle_badge(seL4_Word badge,
				 seL4_MessageInfo_t tag)
{
	seL4_Word is_fault;
	seL4_Word is_endpoint;

	(void)tag;

	if (badge == 0)
		return false;

	is_endpoint = badge >> BADGE_ENDPOINT_BIT;
	is_fault = (badge >> BADGE_FAULT_BIT) & 1UL;

	if (unlikely(is_fault))
		UK_CRASH("CARRELS: unsupported Microkit fault");

	if (unlikely(is_endpoint))
		UK_CRASH("CARRELS: unsupported Microkit PPC");

	carrels_dispatch_badge(badge);

	return true;
}

/* --- external interfaces --- */

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

void uk_carrels_microkit_poll_enable(void)
{
	carrels_event_poll_enabled = true;
}

void uk_carrels_microkit_wait(void)
{
	seL4_Word badge = 0;
	seL4_MessageInfo_t tag;

	tag = seL4_Recv(INPUT_CAP, &badge, REPLY_CAP);

	(void)carrels_handle_badge(badge, tag);
}

bool uk_carrels_microkit_poll(void)
{
	seL4_Word badge = 0;
	seL4_MessageInfo_t tag;

	if (!carrels_event_poll_enabled)
		return false;

	tag = seL4_NBRecv(INPUT_CAP, &badge, REPLY_CAP);

	return carrels_handle_badge(badge, tag);
}
