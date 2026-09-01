#include "penaltyroute.h"

legacy_s16 penalty_route_find_backtrack(
	legacy_s16 current_track,
	legacy_s16 track_count,
	const legacy_s16 far* next_tracks,
	const legacy_s16 far* alternate_tracks)
{
	legacy_s16 reverse_distance[PENALTY_ROUTE_CAPACITY];
	legacy_s16 source_track;
	legacy_s16 next_track;
	legacy_s16 alternate_track;
	legacy_s16 depth;
	legacy_u8 found_predecessor;
	legacy_u8 next_reaches_current;
	legacy_u8 alternate_reaches_current;

	if (track_count <= 0 ||
		track_count > (legacy_s16)PENALTY_ROUTE_CAPACITY ||
		current_track < 0 || current_track >= track_count)
		return -1;
	for (source_track = 0; source_track < track_count; source_track++)
		reverse_distance[source_track] = -1;
	reverse_distance[current_track] = 0;

	for (depth = 0; depth < track_count; depth++) {
		found_predecessor = 0;
		for (source_track = 0; source_track < track_count;
			source_track++) {
			if (reverse_distance[source_track] != -1)
				continue;
			next_track = next_tracks[source_track];
			alternate_track = alternate_tracks[source_track];
			next_reaches_current = next_track >= 0 &&
				next_track < track_count &&
				reverse_distance[next_track] == depth;
			alternate_reaches_current = alternate_track >= 0 &&
				alternate_track < track_count &&
				reverse_distance[alternate_track] == depth;
			if (next_reaches_current == 0 &&
				alternate_reaches_current == 0)
				continue;

			/*
			 * Return the nearest decision point whose other route does not
			 * lead directly into the dead-end chain.  The caller can resume
			 * its normal forward search there and retain skipped-piece
			 * distance accounting.
			 */
			if ((next_reaches_current != 0 &&
				alternate_track >= 0 && alternate_track < track_count &&
				reverse_distance[alternate_track] == -1) ||
				(alternate_reaches_current != 0 &&
				next_track >= 0 && next_track < track_count &&
				reverse_distance[next_track] == -1))
				return source_track;

			reverse_distance[source_track] = depth + 1;
			found_predecessor = 1;
		}
		if (found_predecessor == 0)
			break;
	}
	return -1;
}
