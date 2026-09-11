#include "state_internal.h"
#include "game_input.h"
#include "crash_state.h"
#include "race_stats.h"
#include "car_speed.h"
#include "track_objects.h"
#include "externs.h"
#include "residue.h"

#define ROUTE_POINT_ADVANCE_DISTANCE 275
#define ROUTE_ALIGNMENT_WRAP_LIMIT (ANGLE_FULL_TURN - ANGLE_EIGHTH_TURN)
#define ROUTE_GUIDANCE_DIRECTION_SHIFT 8U
#define ROUTE_DIRECTION_LEFT_SECTOR 1
#define ROUTE_DIRECTION_RIGHT_SECTOR 3
#define ROUTE_GEOMETRY_POINT_COUNT 4U
enum ROUTE_HEIGHT_REFERENCE {
	ROUTE_HEIGHT_REFERENCE_TRACK_LEVEL = 0,
	ROUTE_HEIGHT_REFERENCE_CAR_LEVEL = 1
};
#define PENALTY_ROUTE_DECISION_DISTANCE 3
#define PENALTY_ROUTE_CONFIRMATION_COUNT 3
#define PENALTY_SECONDS_PER_SKIPPED_ROUTE 3
#define PENALTY_DISPLAY_DURATION_SHIFT 2U

/* Vector from the player's car to its current route point. A y of -1 marks
 * a route point with no height of its own: the route search still measures
 * the drop to the car, the steering hint treats the point as level. */
static void route_point_delta(struct VECTOR *delta, legacy_s16 unspecified_height_reference)
{
	*delta = state.playerstate.car_route_target;
	delta->x = LEGACY_S16_WRAP_SUB(delta->x, position_to_word(state.playerstate.car_position.lx));
	if (delta->y == ROUTE_POINT_HEIGHT_UNSPECIFIED) {
		delta->y =
			unspecified_height_reference == ROUTE_HEIGHT_REFERENCE_CAR_LEVEL
				? 0
				: LEGACY_S16_WRAP_NEGATE(position_to_word(state.playerstate.car_position.ly));
	} else {
		delta->y =
			LEGACY_S16_WRAP_SUB(delta->y, position_to_word(state.playerstate.car_position.ly));
	}
	delta->z = LEGACY_S16_WRAP_SUB(delta->z, position_to_word(state.playerstate.car_position.lz));
}

static legacy_u8 confirm_player_route(legacy_s16 route_index, legacy_s16 skipped_route_count)
{
	legacy_u8 commit_penalty = 0;

	if (route_index == 0 && state.game_player_previous_route != 0) {
		state.playerstate.car_lap_count = LEGACY_S8_WRAP_ADD(state.playerstate.car_lap_count, 1);
		commit_penalty = 1;
	} else if (skipped_route_count >= 0 && skipped_route_count < PENALTY_ROUTE_DECISION_DISTANCE) {
		state.game_route_confirmation_count = ROUTE_CONFIRMATION_NONE;
		state.game_player_confirmed_route = route_index;
	} else if (skipped_route_count == PENALTY_ROUTE_FINISH_REACHED ||
			   skipped_route_count > PENALTY_ROUTE_DECISION_DISTANCE) {
		if (track_primary_route_links[state.game_player_previous_route] == route_index ||
			track_alternate_route_links[state.game_player_previous_route] == route_index) {
			state.game_route_confirmation_count =
				LEGACY_S8_WRAP_ADD(state.game_route_confirmation_count, ROUTE_CONFIRMATION_STEP);
		} else {
			if (track_primary_route_links[route_index] == state.game_player_previous_route ||
				track_alternate_route_links[route_index] == state.game_player_previous_route) {
				state.game_player_route_status = ROUTE_TRACKING_WRONG_WAY;
			}
			state.game_route_confirmation_count = ROUTE_CONFIRMATION_INITIAL;
		}
		if (state.game_route_confirmation_count >= PENALTY_ROUTE_CONFIRMATION_COUNT) {
			commit_penalty = 1;
		}
	}
	return commit_penalty;
}

static void update_player_route_penalty(void)
{
	legacy_s16 route_index = state.game_player_confirmed_route;
	legacy_s16 skipped_route_count;
	legacy_s16 route_distance = detect_penalty(&route_index, &skipped_route_count);
	if (route_distance != 0) {
		legacy_u8 commit_penalty = 0;
		if (skipped_route_count == PENALTY_ROUTE_OUTSIDE_TRACK) {
			state.game_player_route_status = ROUTE_TRACKING_OUTSIDE_TRACK;
			state.game_route_confirmation_count = ROUTE_CONFIRMATION_NONE;
		} else {
			if (state.game_player_route_status == ROUTE_TRACKING_OUTSIDE_TRACK) {
				state.game_player_route_status = ROUTE_TRACKING_NORMAL;
				state.game_route_confirmation_count = ROUTE_CONFIRMATION_NONE;
			}
			if (state.game_player_route_status == ROUTE_TRACKING_NORMAL) {
				commit_penalty = confirm_player_route(route_index, skipped_route_count);
			}
		}
		if (commit_penalty != 0) {
			state.game_player_confirmed_route = route_index;
			state.game_route_confirmation_count = ROUTE_CONFIRMATION_NONE;
			if (skipped_route_count > 0) {
				penalty_time =
					LEGACY_S16_WRAP_MUL(LEGACY_S16_WRAP_MUL(skipped_route_count, framespersec),
										PENALTY_SECONDS_PER_SKIPPED_ROUTE);
				show_penalty_counter = LEGACY_S8_FROM_BITS(
					(legacy_u8)LEGACY_U16_SHL(framespersec, PENALTY_DISPLAY_DURATION_SHIFT));
				state.game_penalty = LEGACY_S16_WRAP_ADD(state.game_penalty, penalty_time);
			}
		}
		state.game_player_previous_route = route_index;
	}
}

static legacy_s16 player_route_depth(legacy_s8 previous_route_status,
									 struct MATRIX *world_to_car_rotation)
{
	struct VECTOR local_route_delta;
	legacy_s16 route_depth = 0;
	struct VECTOR world_route_delta;
	if (state.playerstate.car_route_index != ROUTE_INDEX_NONE) {
		if ((previous_route_status == ROUTE_TRACKING_NORMAL ||
			 state.game_player_route_status != ROUTE_TRACKING_NORMAL) &&
			(state.playerstate.car_route_index == state.game_player_confirmed_route ||
			 track_primary_route_links[state.game_player_confirmed_route] ==
				 state.playerstate.car_route_index ||
			 track_alternate_route_links[state.game_player_confirmed_route] ==
				 state.playerstate.car_route_index)) {
			route_point_delta(&world_route_delta, ROUTE_HEIGHT_REFERENCE_CAR_LEVEL);
			mat_mul_vector(&world_route_delta, world_to_car_rotation, &local_route_delta);
			route_depth = local_route_delta.z;
		} else {
			state.playerstate.car_route_index = ROUTE_INDEX_NONE;
		}
	}
	return route_depth;
}

static legacy_s8 select_player_route_point(legacy_s16 route_index,
										   struct MATRIX *world_to_car_rotation)
{
	legacy_s8 candidate_route_point = ROUTE_POINT_FIRST;
	legacy_s8 selected_route_point;
	legacy_s8 route_end_reached = 0;
	struct VECTOR world_route_delta;
	struct VECTOR local_route_delta;
	struct VECTOR route_delta_or_best_depth;
	do {
		route_end_reached = LEGACY_S8_FROM_BITS(
			(legacy_u8)get_track_route_point(route_index, &state.playerstate.car_route_target,
											 (legacy_s16)(legacy_u8)candidate_route_point, 0));
		route_point_delta(&world_route_delta, ROUTE_HEIGHT_REFERENCE_TRACK_LEVEL);
		mat_mul_vector(&world_route_delta, world_to_car_rotation, &local_route_delta);
		if (candidate_route_point == ROUTE_POINT_FIRST ||
			(local_route_delta.z < route_delta_or_best_depth.z && local_route_delta.z > 0)) {
			selected_route_point = candidate_route_point;
			route_delta_or_best_depth.z = local_route_delta.z;
		}
		candidate_route_point = LEGACY_S8_WRAP_ADD(candidate_route_point, ROUTE_POINT_STEP);
	} while (route_end_reached == 0);

	return selected_route_point;
}

static void recover_player_route_direction(legacy_s16 route_index, legacy_s8 selected_route_point)
{
	struct VECTOR route_segment_start[ROUTE_GEOMETRY_POINT_COUNT];
	struct VECTOR route_segment_end[ROUTE_GEOMETRY_POINT_COUNT];
	if (selected_route_point == ROUTE_POINT_FIRST) {
		get_track_route_point(route_index, route_segment_start, ROUTE_POINT_FIRST, 0);
		get_track_route_point(route_index, route_segment_end, ROUTE_POINT_SECOND, 0);
	} else {
		get_track_route_point(
			route_index, route_segment_start,
			(legacy_s16)LEGACY_S8_WRAP_SUB(selected_route_point, ROUTE_POINT_STEP), 0);
		get_track_route_point(route_index, route_segment_end,
							  (legacy_s16)(legacy_u8)selected_route_point, 0);
	}
	legacy_s16 route_direction = LEGACY_S16_FROM_BITS(
		(legacy_u16)LEGACY_S16_WRAP_SUB(
			state.playerstate.car_rotate.x,
			polarAngle(LEGACY_S16_WRAP_SUB(route_segment_start[0].x, route_segment_end[0].x),
					   LEGACY_S16_WRAP_SUB(route_segment_end[0].z, route_segment_start[0].z))) &
		ANGLE_MASK);
	if (route_direction > ROUTE_ALIGNMENT_WRAP_LIMIT || route_direction < ANGLE_EIGHTH_TURN) {
		state.game_player_route_status = ROUTE_TRACKING_NORMAL;
		state.game_route_confirmation_count = ROUTE_CONFIRMATION_INITIAL;
		state.playerstate.car_route_index = route_index;
		state.playerstate.car_route_point_index = selected_route_point;
	}
}

static void advance_player_route_point(void)
{
	legacy_u8 route_point = (legacy_u8)state.playerstate.car_route_point_index;
	state.playerstate.car_route_point_index = LEGACY_S8_WRAP_ADD(route_point, ROUTE_POINT_STEP);
	if (get_track_route_point(state.playerstate.car_route_index,
							  &state.playerstate.car_route_target, (legacy_s16)route_point,
							  0) != 0) {
		if (track_alternate_route_links[state.game_player_confirmed_route] !=
			TRACK_ROUTE_LINK_NONE) {
			state.playerstate.car_route_index = ROUTE_INDEX_NONE;
		} else {
			state.playerstate.car_route_index =
				track_primary_route_links[state.game_player_confirmed_route];
		}
		state.playerstate.car_route_point_index = ROUTE_POINT_FIRST;
	}
}

static void update_player_route_indicator(void)
{
	struct MATRIX *world_to_car_rotation;
	struct VECTOR world_route_delta;
	struct VECTOR local_route_delta;
	if (state.playerstate.car_route_index != ROUTE_INDEX_NONE &&
		state.game_player_route_status == ROUTE_TRACKING_NORMAL) {
		route_point_delta(&world_route_delta, ROUTE_HEIGHT_REFERENCE_CAR_LEVEL);
		world_to_car_rotation =
			mat_rot_zxy(state.playerstate.car_rotate.z, state.playerstate.car_rotate.y,
						state.playerstate.car_rotate.x, MATRIX_ROTATION_ORDER_YXZ);
		mat_mul_vector(&world_route_delta, world_to_car_rotation, &local_route_delta);
		state.playerstate.car_route_heading_error =
			LEGACY_S16_FROM_BITS((legacy_u16)polarAngle(LEGACY_S16_WRAP_NEGATE(local_route_delta.x),
														local_route_delta.z) &
								 ANGLE_MASK);
		if (state.playerstate.car_crashBmpFlag == CRASH_EVENT_NONE) {
			legacy_s16 route_direction = LEGACY_U16_SAR(
				LEGACY_U16_WRAP_ADD(state.playerstate.car_route_heading_error, ANGLE_EIGHTH_TURN) &
					ANGLE_MASK,
				ROUTE_GUIDANCE_DIRECTION_SHIFT);
			if (route_direction == ROUTE_DIRECTION_LEFT_SECTOR) {
				state.game_player_route_indicator = ROUTE_INDICATOR_LEFT;
			} else if (route_direction == ROUTE_DIRECTION_RIGHT_SECTOR &&
					   state.playerstate.car_route_has_reverse_path == 0) {
				state.game_player_route_indicator = ROUTE_INDICATOR_RIGHT;
			} else {
				state.game_player_route_indicator = ROUTE_INDICATOR_NONE;
			}
		}
	}
}

static void check_player_finish(void)
{
	if (state.playerstate.car_lap_count != 0) {
		legacy_s16 route_distance = multiply_and_scale(
			cos_fast(track_angle),
			LEGACY_S16_WRAP_SUB(track_row_centers[start_finish_row],
								position_to_word(state.playerstate.car_position.lz)));
		route_distance = LEGACY_S16_WRAP_ADD(
			route_distance,
			multiply_and_scale(
				sin_fast(track_angle),
				LEGACY_S16_WRAP_SUB(track_column_centers[start_finish_column],
									position_to_word(state.playerstate.car_position.lx))));
		if (route_distance < 0) {
			update_crash_state(CRASH_EVENT_FINISH, PLAYER_CAR_INDEX);
		}
	}
}

static void update_player_route_guidance(legacy_s8 previous_route_status)
{
	state.game_player_route_indicator = ROUTE_INDICATOR_NONE;
	legacy_s16 route_index;
	struct MATRIX *world_to_car_rotation;
	if (state.game_player_route_status != ROUTE_TRACKING_OUTSIDE_TRACK) {
		world_to_car_rotation =
			mat_rot_zxy(state.playerstate.car_rotate.z, state.playerstate.car_rotate.y,
						state.playerstate.car_rotate.x, MATRIX_ROTATION_ORDER_YXZ);

		legacy_u8 route_selection_required = 0;
		legacy_u8 route_advance_required = 0;
		if (state.game_player_route_status == ROUTE_TRACKING_WRONG_WAY) {
			if (state.playerstate.car_crashBmpFlag == CRASH_EVENT_NONE) {
				state.game_player_route_indicator = ROUTE_INDICATOR_WRONG_WAY;
			}
			route_index = state.game_player_previous_route;
			route_selection_required = 1;
		} else {
			legacy_s16 route_distance_or_direction =
				player_route_depth(previous_route_status, world_to_car_rotation);
			if (route_distance_or_direction < ROUTE_POINT_ADVANCE_DISTANCE) {
				if (state.playerstate.car_route_index == ROUTE_INDEX_NONE) {
					route_index = state.game_player_confirmed_route;
					route_selection_required = 1;
				} else {
					route_advance_required = 1;
				}
			}
		}

		legacy_u8 guidance_required = 1;
		if (route_selection_required != 0) {
			if (track_alternate_route_links[route_index] != TRACK_ROUTE_LINK_NONE) {
				guidance_required = 0;
			} else {
				legacy_s8 selected_route_point =
					select_player_route_point(route_index, world_to_car_rotation);
				if (state.game_player_route_status == ROUTE_TRACKING_WRONG_WAY) {
					recover_player_route_direction(route_index, selected_route_point);
				} else {
					state.playerstate.car_route_index = state.game_player_confirmed_route;
					state.playerstate.car_route_point_index = selected_route_point;
				}
				route_advance_required = 1;
			}
		}

		if (route_advance_required != 0) {
			advance_player_route_point();
		}

		if (guidance_required != 0) {
			update_player_route_indicator();
		}

		check_player_finish();
	}
}

void update_player_tick(legacy_s8 input_flags)
{
	update_player_tick_with_legacy_si(input_flags, LEGACY_DEFAULT_PLAYER_TICK_SI);
}

void update_player_tick_with_legacy_si(legacy_s8 input_flags, legacy_s16 caller_si)
{
	if (show_penalty_counter != 0) {
		show_penalty_counter = LEGACY_S8_WRAP_SUB(show_penalty_counter, 1);
	}

	state.playerstate.car_sound_flags = CAR_SOUND_ENGINE_ACTIVE_FLAG;
	if (state.playerstate.car_crashBmpFlag != CRASH_EVENT_NONE) {
		state.game_player_route_indicator = ROUTE_INDICATOR_NONE;
		input_flags = INPUT_BRAKE_FLAG;

		if (state.playerstate.car_actual_speed == CAR_SPEED_STOPPED) {
			state.playerstate.car_sound_flags = CAR_SOUND_NONE;

			if (state.playerstate.car_rev_speed == CAR_SPEED_STOPPED &&
				state.playerstate.car_wheel_vertical_speed[0] == 0 &&
				state.playerstate.car_wheel_vertical_speed[1] == 0 &&
				state.playerstate.car_wheel_vertical_speed[2] == 0 &&
				state.playerstate.car_wheel_vertical_speed[3] == 0) {
				return;
			}
		}
	}

	update_car_speed(input_flags, PLAYER_CAR_INDEX, &state.playerstate, &simd_player);
	legacy_execution_residue.grip_stack_words[LEGACY_RESIDUE_FIRST_WORD] =
		state.playerstate.car_lastrpm;
	legacy_execution_residue.grip_stack_words[LEGACY_RESIDUE_SECOND_WORD] =
		(legacy_s16)state.playerstate.car_rev_speed;
	legacy_execution_residue.grip_stack_words[LEGACY_RESIDUE_THIRD_WORD] =
		(legacy_s16)state.playerstate.car_gearratio;
	update_player_steering_input(LEGACY_S16_SAR((legacy_s16)input_flags, INPUT_STEERING_SHIFT) &
								 INPUT_PEDAL_MASK);
	legacy_u16 rev_speed_before_grip = state.playerstate.car_rev_speed;
	legacy_u16 actual_speed_before_grip = state.playerstate.car_actual_speed;
	update_grip(&state.playerstate, &simd_player, GRIP_BEHAVIOR_PLAYER);
	update_legacy_grip_stack_words(&state.playerstate, &simd_player, rev_speed_before_grip,
								   actual_speed_before_grip, caller_si);
	update_player_state(&state.playerstate, &simd_player, &state.opponentstate, &simd_opponent,
						PLAYER_CAR_INDEX);
	state.game_travDist = LEGACY_S32_WRAP_ADD(
		state.game_travDist, (legacy_s32)(legacy_u16)state.playerstate.car_actual_speed);
	legacy_s8 previous_route_status = state.game_player_route_status;
	update_player_route_penalty();
	update_player_route_guidance(previous_route_status);
}
