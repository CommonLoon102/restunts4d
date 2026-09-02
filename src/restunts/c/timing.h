#ifndef RESTUNTS_TIMING_H
#define RESTUNTS_TIMING_H

#include "legacy.h"

void timer_reg_callback(void (far* callback)(void));
void timer_remove_callback(void (far* callback)(void));
legacy_u32 timer_get_delta_alt(void);
legacy_u32 timer_custom_delta(legacy_u32 ticks);
void timer_reset(void);
legacy_u32 timer_copy_counter(legacy_u32 ticks);
legacy_u32 timer_wait_for_dx(void);
legacy_s16 timer_compare_dx(void);
legacy_u32 timer_get_counter_unk(legacy_u32 ticks);
legacy_u32 set_add_value(legacy_u32 ticks);
legacy_s16 sub_2EB07(void);
legacy_u32 sub_2EB1E(legacy_u32 ticks);

#endif
