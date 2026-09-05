#ifndef RESTUNTS_RESIDUE_H
#define RESTUNTS_RESIDUE_H

#include "legacy.h"

#define LEGACY_RESIDUE_WORD_COUNT 4U
#define LEGACY_RESIDUE_FIRST_WORD 0U
#define LEGACY_RESIDUE_SECOND_WORD 1U
#define LEGACY_RESIDUE_THIRD_WORD 2U
#define LEGACY_RESIDUE_FOURTH_WORD 3U
#define LEGACY_EXECUTION_RESIDUE_SIZE 26U

/*
 * Deterministic representation of values which the original executable read
 * through reused stack locations or wrapped legacy-memory aliases.  Static
 * storage supplies a defined zero value before the first simulated frame;
 * thereafter each member retains the value written by its legacy source.
 */
struct LEGACY_EXECUTION_RESIDUE {
	legacy_s16 wheel_plane_angles[LEGACY_RESIDUE_WORD_COUNT];
	legacy_s16 wheel_angle_stack_words[LEGACY_RESIDUE_WORD_COUNT];
	legacy_s16 grip_stack_words[LEGACY_RESIDUE_WORD_COUNT];
	legacy_s16 penalty_route_word;
};

typedef char legacy_execution_residue_must_have_expected_size[
	(sizeof(struct LEGACY_EXECUTION_RESIDUE) ==
		LEGACY_EXECUTION_RESIDUE_SIZE) ? 1 : -1];

extern struct LEGACY_EXECUTION_RESIDUE legacy_execution_residue;

#endif
