/* SPDX-License-Identifier: BSD-3-Clause */

#include <uk/assert.h>
#include <errno.h>
#include <uk/init.h>
#include <uk/isr/semaphore.h>
#include <uk/print.h>
#include <uk/sched.h>
#include <uk/sddf.h>

#include <carrels/events.h>

#define INPUT_CAP 1
#define REPLY_CAP 4

#define BADGE_FAULT_BIT    62
#define BADGE_ENDPOINT_BIT 63

static bool carrels_event_poll_enabled;

typedef struct {
	carrels_event_handler_t handler;
	void *arg;
	struct uk_semaphore events;
	struct uk_thread *thread;
	microkit_channel channel;
} carrels_event_entry_t;

static carrels_event_entry_t carrels_event_table[MICROKIT_MAX_CHANNELS];

static bool __carrels_event_threads_ready;

static bool carrels_event_threads_are_ready(void)
{
	return __carrels_event_threads_ready;
}

static void carrels_event_threads_mark_ready(void)
{
	__carrels_event_threads_ready = true;
}

// @gt ??
// This is a wrapper thread for the events.
// With this wrapper, the scheduler does not need to dispatch events
// by executing on the scheduler's context. Instead, it can simply
// schedule the thread that wraps the event handler using a semaphore,
// as the thread blocks on the semaphore when the event is not arrived
// The event thread can execute on its own context, preventing recursive
// scheduler event dispatching.
//??
static __noreturn void carrels_event_thread_wrapper(void *arg)
{
	carrels_event_entry_t *entry = arg;
	carrels_event_handler_t handler;
	void *handler_arg;

	UK_ASSERT(entry);

	for (;;) {
		// @gt ??
		// An implicit uk_sched_yield is embedded in uk_semaphore_down
		// Before yielding, this macro will set the current thread as "blocked"
		// So, even if the scheduler is ready for next scheduling event,
		// this handler thread will not get picked up until the event has arrived
		// and the semaphore is released.
		// Ref: lib/uksched/include/uk/wait.h/uk_waitq_wait_event(wq, condition)
		uk_semaphore_down(&entry->events);

		handler = entry->handler;
		handler_arg = entry->arg;

		if (handler) {
			handler(entry->channel, handler_arg);
		}
	}
}

static int carrels_event_thread_create(carrels_event_entry_t *entry)
{
	UK_ASSERT(entry);
	UK_ASSERT(entry->handler);

	if (entry->thread)
		return 0;

	uk_semaphore_init(&entry->events, 0);
	entry->thread = uk_sched_thread_create(uk_sched_current(),
					       carrels_event_thread_wrapper, entry,
					       "carrels-event");
	if (!entry->thread)
		return -ENOMEM;

	return 0;
}

static int carrels_event_threads_init(struct uk_init_ctx *ictx __unused)
{
	carrels_event_entry_t *entry;
	microkit_channel ch;
	int rc;

	carrels_event_threads_mark_ready();

	/* For each channel, prepare an individual handler thread to receive
	 * asynchronous events. This helps the scheduler to dispatch event
	 * each time it polls an event bunch. What the scheduler does when
	 * dispatching events is simply schedule the handler thread for this
	 * event, as the thread has already been created.
	 */
	for (ch = 0; ch < MICROKIT_MAX_CHANNELS; ch++) {
		entry = &carrels_event_table[ch];
		entry->channel = ch;
		// If a handler hasn't been assigned yet, delay thread creation
		// until an event handler is registered to save resources.
		if (!entry->handler)
			continue;

		rc = carrels_event_thread_create(entry);
		if (rc < 0) {
			uk_pr_err("CARRELS: failed to create event thread for channel %u: %d\n",
				  (unsigned int)ch, rc);
			return rc;
		}
	}

	return 0;
}

uk_plat_initcall_prio(carrels_event_threads_init, 0, UK_PRIO_LATEST);


static void carrels_dispatch_badge(seL4_Word badge)
{
	UK_ASSERT((badge >> MICROKIT_MAX_CHANNELS) == 0);

	microkit_channel c = 0;

	while (badge != 0) {
		if (badge & 1UL) {
			carrels_event_dispatch(c);
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
	carrels_event_entry_t *entry;
	int rc;

	if (ch > MICROKIT_MAX_CHANNEL_ID || handler == NULL)
		return CARRELS_EVENT_EINVAL;

	entry = &carrels_event_table[ch];

	if (entry->handler != NULL)
		return CARRELS_EVENT_EBUSY;

	entry->arg = arg;
	entry->handler = handler;
	entry->channel = ch;

	if (carrels_event_threads_are_ready()) {
		rc = carrels_event_thread_create(entry);
		if (rc < 0) {
			entry->handler = NULL;
			entry->arg = NULL;
			return rc;
		}
	}

	return CARRELS_EVENT_OK;
}

int carrels_event_unregister(microkit_channel ch,
			     carrels_event_handler_t handler,
			     void *arg)
{
	carrels_event_entry_t *entry;

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
	carrels_event_entry_t *entry;

	if (unlikely(ch > MICROKIT_MAX_CHANNEL_ID)) {
		uk_pr_warn("CARRELS: invalid notification channel %u\n",
			   (unsigned int)ch);
		return;
	}

	entry = &carrels_event_table[ch];

	if (unlikely(entry->handler == NULL)) {
		uk_pr_debug("CARRELS: unhandled notification channel %u\n",
			    (unsigned int)ch);
		return;
	}

	/* Scheduler ingress must remain run-to-completion.  The semaphore only
	 * records the event and makes this channel's dedicated worker runnable;
	 * the registered handler executes later in normal thread context.
	 */
	UK_ASSERT(entry->thread);
	uk_semaphore_up_isr(&entry->events);
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
