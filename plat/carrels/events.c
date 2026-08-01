
#include <uk/assert.h>
#include <uk/print.h>
#include <uk/microkit.h>

/* stolen from libmicrokit/src/main.c */

#define INPUT_CAP 1
#define REPLY_CAP 4

#define BADGE_FAULT_BIT 62
#define BADGE_ENDPOINT_BIT 63

static void carrels_dispatch_notification_badge(seL4_Word badge)
{
	unsigned int channel = 0;

	while (badge != 0) {
		if (badge & 1) {
            notified(channel);
        }

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

	is_fault = (badge >> BADGE_FAULT_BIT) & 1;
	is_endpoint = badge >> BADGE_ENDPOINT_BIT;

	if (is_fault) {
		UK_CRASH("CARRELS: Unsupported Microkit fault");
	}

	if (is_endpoint) {
		UK_CRASH("CARRELS: Unsupported ppc");
	}

	carrels_dispatch_notification_badge(badge);
}
