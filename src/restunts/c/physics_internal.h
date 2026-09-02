#ifndef RESTUNTS_PHYSICS_INTERNAL_H
#define RESTUNTS_PHYSICS_INTERNAL_H

#include "externs.h"

legacy_s16 scale_position_delta(legacy_s32 current,
	legacy_s32 previous, legacy_s16 factor, legacy_s16 divisor);
legacy_s16 scale_speed_to_travel(legacy_u16 speed,
	legacy_u16 divisor);
legacy_s16 physics_position_word(legacy_s32 position);
legacy_s16 physics_difference_word(legacy_s32 left, legacy_s32 right);
legacy_s16 wheel_pair_delta(legacy_s16 first, legacy_s16 second,
	legacy_s16 third, legacy_s16 fourth);

legacy_s16 bto_auxiliary1(legacy_s16 column, legacy_s16 row,
	struct VECTOR* output);
legacy_s16 carState_rc_op(struct CARSTATE* carstate,
	legacy_s16 contact_delta, legacy_s16 wheel_index);
legacy_s16 car_car_speed_adjust_maybe(struct CARSTATE* first_state,
	struct CARSTATE* second_state);
legacy_s16 car_car_coll_detect_maybe(
	struct POINT2D* first_collision_points,
	struct VECTOR* first_world_coordinates,
	struct POINT2D* second_collision_points,
	struct VECTOR* second_world_coordinates);

#endif
