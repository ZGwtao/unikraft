#ifndef __CARRELS_TIMER_H__
#define __CARRELS_TIMER_H__

#include <stdbool.h>

#include <uk/arch/types.h>
#include <uk/microkit.h>

extern sddf_channel uk_carrels_timer_channel;
extern bool uk_carrels_timer_ready;

extern volatile __u64 uk_carrels_timer_event_count;

int uk_carrels_timer_event_init(void);

#endif