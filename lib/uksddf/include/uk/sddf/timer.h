/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __UK_SDDF_TIMER_H__
#define __UK_SDDF_TIMER_H__

#include <uk/arch/types.h>
#include <uk/sddf.h>

#ifdef __cplusplus
extern "C" {
#endif

int uk_sddf_timer_init(sddf_channel channel);

void uk_sddf_timer_set_timeout(__nsec duration);

__u64 uk_sddf_timer_event_count(void);

#ifdef __cplusplus
}
#endif

#endif /* __UK_SDDF_TIMER_H__ */