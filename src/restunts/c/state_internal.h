#ifndef RESTUNTS_STATE_INTERNAL_H
#define RESTUNTS_STATE_INTERNAL_H

#include "externs.h"

legacy_s16 detect_penalty(legacy_s16* current_track,
	legacy_s16* penalty_count);
void update_car_speed(legacy_s8 input, legacy_s16 is_opponent,
	struct CARSTATE* carstate, struct SIMD* simd);
void update_grip(struct CARSTATE* carstate, struct SIMD* simd,
	legacy_s16 is_player);
void update_legacy_grip_stack_words(struct CARSTATE* carstate,
	struct SIMD* simd, legacy_u16 speed_before_grip,
	legacy_u16 speed2_before_grip);
void update_player_state(struct CARSTATE* playerstate,
	struct SIMD* playersimd, struct CARSTATE* opponentstate,
	struct SIMD* opponentsimd, legacy_s16 is_opponent);

legacy_s16 world_position_word(legacy_s32 position);
struct VECTOR* track_vector_from_legacy_offset(legacy_u16 offset);
void upd_statef20_from_steer_input(legacy_s8 steering_input);

legacy_u8 subst_hillroad_track(legacy_u8 terrain, legacy_u8 track);

#endif
