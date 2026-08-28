#ifndef RESTUNTS_RESIDUE_H
#define RESTUNTS_RESIDUE_H

#include "legacy.h"

/*
 * Deterministic representation of values which the original executable read
 * from overlapping, otherwise uninitialized stack locations.  Static storage
 * supplies a defined zero value before the first simulated frame; thereafter
 * each member retains the words written by the corresponding legacy call site.
 */
struct LEGACY_EXECUTION_RESIDUE {
	legacy_s16 wheel_plane_angles[4];
	legacy_s16 wheel_angle_stack_words[4];
	legacy_s16 grip_stack_words[4];
};

typedef char legacy_execution_residue_must_be_24_bytes[
	(sizeof(struct LEGACY_EXECUTION_RESIDUE) == 24) ? 1 : -1];

extern struct LEGACY_EXECUTION_RESIDUE legacy_execution_residue;

#endif
