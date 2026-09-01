#ifndef RESTUNTS_PENALTYROUTE_H
#define RESTUNTS_PENALTYROUTE_H

#include "legacy.h"

#define PENALTY_ROUTE_CAPACITY 0x385U

legacy_s16 penalty_route_find_backtrack(
	legacy_s16 current_track,
	legacy_s16 track_count,
	const legacy_s16 far* next_tracks,
	const legacy_s16 far* alternate_tracks);

#endif
