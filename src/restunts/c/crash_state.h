#ifndef RESTUNTS_CRASH_STATE_H
#define RESTUNTS_CRASH_STATE_H

#include "legacy.h"

/* Race lifecycle events shared by driving, input, and replay control. */

enum CRASH_EVENT {
	CRASH_EVENT_NONE = 0,
	CRASH_EVENT_COLLISION = 1,
	CRASH_EVENT_WATER = 2,
	CRASH_EVENT_FINISH = 3,
	CRASH_EVENT_EXIT = 4,
	CRASH_EVENT_IMMEDIATE_STOP = 5
};

void update_crash_state(legacy_s16 crash_event, legacy_s16 car_index);

#endif
