#include "legacy.h"
#include "math.h"
#include "physics_internal.h"
#include "residue.h"

#define PLAYER_PHYSICS_WHEEL_COUNT 4
#define PLAYER_PHYSICS_FRONT_WHEEL_COUNT 2

enum PLAYER_PHYSICS_WHEEL_INDEX {
	PLAYER_PHYSICS_FRONT_WHEEL_FIRST = 0,
	PLAYER_PHYSICS_FRONT_WHEEL_SECOND = 1,
	PLAYER_PHYSICS_REAR_WHEEL_FIRST = 2,
	PLAYER_PHYSICS_REAR_WHEEL_SECOND = 3
};

#define PLAYER_PHYSICS_WHEEL_CENTROID_SHIFT 2U
#define PLAYER_PHYSICS_POSE_VECTOR_COUNT 2

enum PLAYER_PHYSICS_POSE_VECTOR_INDEX {
	PLAYER_PHYSICS_POSE_POSITION_INDEX = 0,
	PLAYER_PHYSICS_POSE_ROTATION_INDEX = 1
};

#define PLAYER_PHYSICS_COLLISION_POINT_CAPACITY 32
#define PLAYER_PHYSICS_POSITION_SCALE_SHIFT 6U
#define PLAYER_PHYSICS_TRACK_COORDINATE_SHIFT 10U
#define PLAYER_PHYSICS_LOW_RATE_TRAVEL_DIVISOR 7680U
#define PLAYER_PHYSICS_NORMAL_RATE_TRAVEL_DIVISOR 15360U
#define PLAYER_PHYSICS_LOW_SPEED_LIMIT 7680U
#define PLAYER_PHYSICS_SUSPENSION_TRAVEL_LIMIT 384
#define PLAYER_PHYSICS_UP_VECTOR_LENGTH 30000
#define PLAYER_PHYSICS_INVERTED_WHEEL_ADJUSTMENT 192
#define PLAYER_PHYSICS_PSEUDO_GRAVITY_LENGTH 130
#define PLAYER_PHYSICS_WALL_PUSH_DISTANCE 768
#define PLAYER_PHYSICS_WALL_SPEED_ANGLE_MULTIPLIER 70
#define PLAYER_PHYSICS_WALL_SPEED_BIAS 100
#define PLAYER_PHYSICS_WALL_SPEED_SHIFT 8U
#define PLAYER_PHYSICS_WALL_SOUND_FLAG 16U
#define PLAYER_PHYSICS_SUSPENSION_SOUND_FLAG 32U
#define PLAYER_PHYSICS_CONTACT_DISTANCE_LIMIT 12
#define PLAYER_PHYSICS_INVERTED_CONTACT_DISTANCE_LIMIT 24
#define PLAYER_PHYSICS_PLANE_PENETRATION_BIAS 6
#define PLAYER_PHYSICS_PLANE_RETRACE_DISTANCE 64
#define PLAYER_PHYSICS_SUSPENSION_SOUND_THRESHOLD 250
#define PLAYER_PHYSICS_SUSPENSION_CRASH_THRESHOLD 23275
#define PLAYER_PHYSICS_WORLD_MIN_POSITION 3840L
#define PLAYER_PHYSICS_WORLD_MAX_POSITION 1962239L
#define PLAYER_PHYSICS_WORLD_MAX_EXCLUSIVE 1962240L
#define PLAYER_PHYSICS_TRACK_GRID_LAST_COORDINATE 29
#define PLAYER_PHYSICS_TRACK_GRID_SIZE 30
#define PLAYER_PHYSICS_START_FINISH_POLE_OFFSET 126
#define PLAYER_PHYSICS_COLLISION_RETRY_LIMIT 5
#define PLAYER_PHYSICS_RESIDUE_MATRIX_FIRST_VALUE 4U
#define PLAYER_PHYSICS_ROTATION_DEADBAND 2
#define PLAYER_PHYSICS_PLANE_INDEX_NONE (-1)
#define PLAYER_PHYSICS_GROUND_PLANE_INDEX 0
#define PLAYER_PHYSICS_WALL_INDEX_NONE (-1)
#define PLAYER_PHYSICS_TRACK_CAMERA_INDEX_NONE (-1)
#define PLAYER_PHYSICS_OBJECT_PARTICLE_KIND_OFFSET 2

enum PLAYER_PHYSICS_FLOW {
	PLAYER_FLOW_BEGIN_COLLISION_PASS,
	PLAYER_FLOW_CHECK_ALL_WHEELS_IN_WATER,
	PLAYER_FLOW_BEGIN_SUSPENSION_UPDATE,
	PLAYER_FLOW_BEGIN_WHEEL_CONTACT_SCAN,
	PLAYER_FLOW_LOOK_UP_WHEEL_TRACK_OBJECT,
	PLAYER_FLOW_STORE_WHEEL_SURFACE,
	PLAYER_FLOW_MEASURE_WHEEL_PLANE_DISTANCE,
	PLAYER_FLOW_CHECK_WALL_PRESENT,
	PLAYER_FLOW_CHECK_WALL_LOWER_BOUND,
	PLAYER_FLOW_CHECK_WALL_UPPER_BOUND,
	PLAYER_FLOW_TRANSFORM_WALL_CROSSING,
	PLAYER_FLOW_CHECK_WALL_NEGATIVE_SIDE,
	PLAYER_FLOW_ORDER_WALL_CROSSING_ENDPOINTS,
	PLAYER_FLOW_KEEP_WALL_ENDPOINT_ORDER,
	PLAYER_FLOW_CHECK_WALL_END_ON_PLANE,
	PLAYER_FLOW_CHECK_WALL_START_ON_PLANE,
	PLAYER_FLOW_SPLIT_WALL_CROSSING_TRAVEL,
	PLAYER_FLOW_SELECT_WALL_RESPONSE_SIDE,
	PLAYER_FLOW_PUSH_ALONG_FORWARD_WALL,
	PLAYER_FLOW_PUSH_ALONG_REVERSE_WALL,
	PLAYER_FLOW_ADJUST_WALL_PUSH_DIRECTION,
	PLAYER_FLOW_ROTATE_WALL_RESPONSE,
	PLAYER_FLOW_CALCULATE_WALL_SAFE_SPEED,
	PLAYER_FLOW_SET_FORWARD_IMPACT_TURN,
	PLAYER_FLOW_APPLY_WALL_CRASH,
	PLAYER_FLOW_BEGIN_WALL_WHEEL_REPOSITION,
	PLAYER_FLOW_CLEAR_RETAINED_WHEEL_TRAVEL,
	PLAYER_FLOW_APPLY_WALL_WHEEL_POSITION,
	PLAYER_FLOW_CHECK_WALL_REPOSITION_REMAINING,
	PLAYER_FLOW_SCALE_RETAINED_WHEEL_TRAVEL,
	PLAYER_FLOW_APPLY_WHEEL_GRAVITY,
	PLAYER_FLOW_RECHECK_DISTANCE_AFTER_GRAVITY,
	PLAYER_FLOW_CHECK_WHEEL_CONTACT_RANGE,
	PLAYER_FLOW_SAVE_WHEEL_CONTACT_DISTANCE,
	PLAYER_FLOW_CHECK_WHEEL_PENETRATION,
	PLAYER_FLOW_TRANSFORM_WHEEL_PLANE_CROSSING,
	PLAYER_FLOW_CHECK_CROSSING_END_ON_PLANE,
	PLAYER_FLOW_RETRACE_WHEEL_FROM_PLANE,
	PLAYER_FLOW_FALL_BACK_TO_GROUND_PLANE,
	PLAYER_FLOW_CLASSIFY_WHEEL_PLANE_DISTANCE,
	PLAYER_FLOW_CHECK_INVERTED_WHEEL_ADJUSTMENT,
	PLAYER_FLOW_CHECK_INVERTED_CONTACT_RANGE,
	PLAYER_FLOW_APPLY_INVERTED_WHEEL_ADJUSTMENT,
	PLAYER_FLOW_CLASSIFY_PLANE_CROSSING_DIRECTION,
	PLAYER_FLOW_ADVANCE_WHEEL_ALONG_PLANE,
	PLAYER_FLOW_SPLIT_PLANE_CROSSING_TRAVEL,
	PLAYER_FLOW_RECHECK_PLANE_PENETRATION,
	PLAYER_FLOW_PUSH_WHEEL_OUT_OF_PLANE,
	PLAYER_FLOW_FINISH_PLANE_CORRECTION,
	PLAYER_FLOW_CHECK_SUSPENSION_IMPACT_SOUND,
	PLAYER_FLOW_CHECK_SUSPENSION_IMPACT_CRASH,
	PLAYER_FLOW_STOP_WHEEL_VERTICAL_SPEED,
	PLAYER_FLOW_ADVANCE_WHEEL_CONTACT_SCAN,
	PLAYER_FLOW_CHECK_WHEEL_CONTACT_SCAN_REMAINING,
	PLAYER_FLOW_PREPARE_WHEEL_CONTACT_LOOKUP,
	PLAYER_FLOW_USE_INTRO_GROUND_PLANE,
	PLAYER_FLOW_APPLY_UPRIGHT_SUSPENSION_OFFSET,
	PLAYER_FLOW_ADVANCE_SUSPENSION_UPDATE,
	PLAYER_FLOW_CHECK_SUSPENSION_UPDATE_REMAINING,
	PLAYER_FLOW_UPDATE_WHEEL_SUSPENSION,
	PLAYER_FLOW_APPLY_ROTATED_SUSPENSION_OFFSET,
	PLAYER_FLOW_AVERAGE_WHEEL_POSITIONS,
	PLAYER_FLOW_CALCULATE_WHEEL_OFFSETS,
	PLAYER_FLOW_CHECK_WORLD_X_MAXIMUM,
	PLAYER_FLOW_CLAMP_WORLD_X_MAXIMUM,
	PLAYER_FLOW_CHECK_WORLD_X_MINIMUM,
	PLAYER_FLOW_CLAMP_WORLD_X_MINIMUM,
	PLAYER_FLOW_CHECK_WORLD_Z_MAXIMUM,
	PLAYER_FLOW_CLAMP_WORLD_Z_MAXIMUM,
	PLAYER_FLOW_CHECK_WORLD_Z_MINIMUM,
	PLAYER_FLOW_CLAMP_WORLD_Z_MINIMUM,
	PLAYER_FLOW_DERIVE_YAW_FROM_WHEELS,
	PLAYER_FLOW_REMOVE_WHEEL_YAW,
	PLAYER_FLOW_DERIVE_PITCH_FROM_WHEELS,
	PLAYER_FLOW_CHECK_POSITIVE_PITCH_DEADBAND,
	PLAYER_FLOW_CHECK_NEGATIVE_PITCH_DEADBAND,
	PLAYER_FLOW_CLEAR_SMALL_PITCH,
	PLAYER_FLOW_PREPARE_WHEEL_PITCH_REMOVAL,
	PLAYER_FLOW_REMOVE_WHEEL_PITCH,
	PLAYER_FLOW_MEASURE_WHEEL_ROLL,
	PLAYER_FLOW_DERIVE_ROLL_FROM_WHEELS,
	PLAYER_FLOW_CHECK_POSITIVE_ROLL_DEADBAND,
	PLAYER_FLOW_CHECK_NEGATIVE_ROLL_DEADBAND,
	PLAYER_FLOW_CLEAR_SMALL_ROLL,
	PLAYER_FLOW_SUM_FRONT_AND_REAR_SURFACES,
	PLAYER_FLOW_PLAY_CAR_IMPACT_SOUNDS,
	PLAYER_FLOW_PLAY_PLAYER_IMPACT_SOUNDS,
	PLAYER_FLOW_FINISH_CAR_IMPACT_SOUNDS,
	PLAYER_FLOW_BEGIN_BODY_PLANE_SCAN,
	PLAYER_FLOW_COMPARE_PREVIOUS_BODY_PLANE,
	PLAYER_FLOW_CHECK_BODY_CROSSING_FROM_ABOVE,
	PLAYER_FLOW_STOP_CAR_AT_BODY_PLANE_COLLISION,
	PLAYER_FLOW_SAVE_BODY_CORNER_POSITION,
	PLAYER_FLOW_CHECK_BODY_PLANE_SCAN_REMAINING,
	PLAYER_FLOW_LOOK_UP_BODY_CORNER_PLANE,
	PLAYER_FLOW_CHECK_BODY_GROUND_PENETRATION,
	PLAYER_FLOW_HANDLE_BODY_GROUND_PENETRATION,
	PLAYER_FLOW_DETECT_PLAYER_JUMP,
	PLAYER_FLOW_PREPARE_CAR_COLLISION_POSE,
	PLAYER_FLOW_CHECK_OTHER_CAR_COLLISION,
	PLAYER_FLOW_RESOLVE_CAR_COLLISION_SPEEDS,
	PLAYER_FLOW_CRASH_BOTH_CARS,
	PLAYER_FLOW_RETURN_AFTER_COLLISION,
	PLAYER_FLOW_PREPARE_SCENERY_COLLISION_SCAN,
	PLAYER_FLOW_CHECK_SCENERY_COLUMN_MAXIMUM,
	PLAYER_FLOW_CHECK_SCENERY_ROW_MINIMUM,
	PLAYER_FLOW_CHECK_SCENERY_ROW_MAXIMUM,
	PLAYER_FLOW_LOAD_AUXILIARY_OBSTACLES,
	PLAYER_FLOW_ADVANCE_AUXILIARY_OBSTACLE,
	PLAYER_FLOW_CHECK_AUXILIARY_OBSTACLE_COLLISION,
	PLAYER_FLOW_CRASH_INTO_FIXED_OBSTACLE,
	PLAYER_FLOW_FIND_BREAKABLE_OBJECT,
	PLAYER_FLOW_CHECK_BREAKABLE_OBJECT_INTACT,
	PLAYER_FLOW_COLLIDE_WITH_BREAKABLE_OBJECT,
	PLAYER_FLOW_CHECK_START_FINISH_COLUMN,
	PLAYER_FLOW_CHECK_START_FINISH_ROW,
	PLAYER_FLOW_CHECK_START_FINISH_POLES,
	PLAYER_FLOW_HANDLE_START_FINISH_POLE_COLLISION,
	PLAYER_FLOW_COMMIT_CAR_POSE,
	PLAYER_FLOW_RETURN_FROM_PHYSICS,
};

/* Hand a long-precision position back as the word-precision vector the rest
   of the engine works with. */
static void physics_position_to_vector(struct VECTOR* destination,
	const struct VECTORLONG* source)
{
	destination->x = position_to_word(source->lx);
	destination->y = position_to_word(source->ly);
	destination->z = position_to_word(source->lz);
}

/* Wheel positions are long-precision; the forces acting on them arrive as
   word-precision vectors, so every step shifts the position the same way. */
static void physics_position_offset(struct VECTORLONG* destination,
	const struct VECTORLONG* source, const struct VECTOR* offset)
{
	destination->lx = LEGACY_S32_WRAP_ADD_S16(source->lx, offset->x);
	destination->ly = LEGACY_S32_WRAP_ADD_S16(source->ly, offset->y);
	destination->lz = LEGACY_S32_WRAP_ADD_S16(source->lz, offset->z);
}

static void physics_position_pull_back(struct VECTORLONG* position,
	const struct VECTOR* offset)
{
	position->lx = LEGACY_S32_WRAP_SUB_S16(position->lx, offset->x);
	position->ly = LEGACY_S32_WRAP_SUB_S16(position->ly, offset->y);
	position->lz = LEGACY_S32_WRAP_SUB_S16(position->lz, offset->z);
}

static void prepare_opponent_rear_wheel(struct VECTOR* wheel,
	struct VECTOR* rotated, struct MATRIX* angle_rotation,
	legacy_s16 wheel_index, legacy_s16 y_adjustment)
{
	*wheel = simd_opponent.wheel_coords[wheel_index];
	wheel->y = LEGACY_S16_WRAP_ADD(LEGACY_S16_WRAP_NEGATE(
		LEGACY_S16_WRAP_ADD(
			state.opponentstate.car_suspension_deflection[wheel_index],
			PLAYER_PHYSICS_SUSPENSION_TRAVEL_LIMIT)),
		y_adjustment);
	if ((state.opponentstate.car_slide_yaw_delta & ANGLE_MASK) != 0) {
		mat_mul_vector(wheel, angle_rotation, rotated);
		*wheel = *rotated;
	}
}

static legacy_s16 scaled_vector_separation(struct VECTOR* first,
	struct VECTOR* second, struct VECTOR* intersection,
	struct VECTOR* delta)
{
	vector_interpolate_at_z(first, second, intersection, 0);
	delta->x = LEGACY_S16_SHL(
		LEGACY_S16_WRAP_SUB(first->x, intersection->x),
		PLAYER_PHYSICS_POSITION_SCALE_SHIFT);
	delta->y = LEGACY_S16_SHL(
		LEGACY_S16_WRAP_SUB(first->y, intersection->y),
		PLAYER_PHYSICS_POSITION_SCALE_SHIFT);
	delta->z = LEGACY_S16_SHL(
		LEGACY_S16_WRAP_SUB(first->z, intersection->z),
		PLAYER_PHYSICS_POSITION_SCALE_SHIFT);
	return polarRadius3D(delta);
}

void update_player_state(struct CARSTATE* carstate, struct SIMD* simd,
	struct CARSTATE* other_carstate, struct SIMD* other_simd,
	legacy_s16 car_index) {
	struct MATRIX wheel_adjustment_rotation;
	legacy_s16 travel_per_tick;
	struct VECTOR transformed_vector;
	struct VECTOR wheel_vector;
	legacy_s16 wheel_plane_headings[PLAYER_PHYSICS_WHEEL_COUNT];
	struct VECTORLONG* current_wheel_position;
	struct VECTORLONG* previous_wheel_position;
	legacy_s16 front_wheel_heading_offset;
	legacy_s8 aux_rotation_or_obstacle_count;
	legacy_s16 inverted_wheel_adjustment;
	struct VECTOR inverted_wheel_offset;
	struct VECTORLONG current_wheel_positions[PLAYER_PHYSICS_WHEEL_COUNT];
	struct VECTORLONG previous_wheel_positions[PLAYER_PHYSICS_WHEEL_COUNT];
	legacy_s16 wheel_index;
	legacy_s8 collision_pass_count;
	struct VECTOR relative_or_suspension_vector, current_relative_position, contact_start_or_delta, contact_end_or_response, intersection_delta_or_body_position, plane_world_origin;
	struct VECTOR car_collision_pose[PLAYER_PHYSICS_POSE_VECTOR_COUNT];
	struct VECTOR obstacle_collision_pose[PLAYER_PHYSICS_POSE_VECTOR_COUNT];
	struct MATRIX collision_plane_rotation;
	legacy_s8 contact_response_reversed;
	legacy_s16 vertical_delta_or_travel, horizontal_delta_or_travel, contact_angle_distance_or_deflection, impact_turn_distance_or_overlap;
	legacy_u16 wall_safe_speed;
	struct MATRIX* response_rotation;
	legacy_s16 collision_angle_distance_or_index;
	legacy_s16 wheel_contact_distances[PLAYER_PHYSICS_WHEEL_COUNT];
	struct PLANE far* contact_plane;
	struct VECTOR wheel_offsets[PLAYER_PHYSICS_WHEEL_COUNT];
	legacy_s16 previous_plane_index;
	legacy_s8 wheel_surface_sum;
	struct VECTOR obstacle_positions[PLAYER_PHYSICS_COLLISION_POINT_CAPACITY];
	enum PLAYER_PHYSICS_FLOW physics_flow;

	//return ported_update_player_state_(carstate, simd, other_carstate, other_simd, car_index);

	/*
	 * Seed the four collision-plane results from the explicit model of the
	 * original overlapping stack window. Each successful wheel lookup below
	 * replaces its corresponding entry.
	 */
	wheel_contact_distances[LEGACY_RESIDUE_FIRST_WORD] =
		legacy_execution_residue.grip_stack_words[LEGACY_RESIDUE_FIRST_WORD];
	wheel_contact_distances[LEGACY_RESIDUE_SECOND_WORD] =
		legacy_execution_residue.grip_stack_words[LEGACY_RESIDUE_SECOND_WORD];
	wheel_contact_distances[LEGACY_RESIDUE_THIRD_WORD] =
		legacy_execution_residue.grip_stack_words[LEGACY_RESIDUE_THIRD_WORD];
	wheel_contact_distances[LEGACY_RESIDUE_FOURTH_WORD] =
		legacy_execution_residue.grip_stack_words[LEGACY_RESIDUE_FOURTH_WORD];

	/* Initialize the working position and rotation from the current car pose. */
	car_working_x = carstate->car_position.lx;
	car_working_y = carstate->car_position.ly;
	car_working_z = carstate->car_position.lz;
	carstate->car_previous_position.lx = carstate->car_position.lx;
	carstate->car_previous_position.ly = carstate->car_position.ly;
	carstate->car_previous_position.lz = carstate->car_position.lz;
	car_working_roll = carstate->car_rotate.z;
	car_initial_roll = carstate->car_rotate.z;
	car_working_pitch = carstate->car_rotate.y;
	car_initial_pitch = carstate->car_rotate.y;
	car_working_yaw = carstate->car_rotate.x;
	car_initial_yaw = carstate->car_rotate.x;

	/*
	 * While the car has surface contact, offset the first two wheel-plane
	 * angles by one quarter of the front-wheel angle.
	 */
	if (carstate->car_sumSurfAllWheels != CAR_WHEEL_CONTACT_NONE) {
		front_wheel_heading_offset = LEGACY_S16_SAR2(
			carstate->car_front_wheel_response_angle);
	} else {
		front_wheel_heading_offset = 0;
	}

	/* Convert speed to per-tick travel, accounting for the simulation rate. */
	if (framespersec == GAME_FRAME_RATE_LOW) {
		travel_per_tick = scale_speed_to_travel(
			carstate->car_actual_speed,
			PLAYER_PHYSICS_LOW_RATE_TRAVEL_DIVISOR);
	} else {
		travel_per_tick = scale_speed_to_travel(
			carstate->car_actual_speed,
			PLAYER_PHYSICS_NORMAL_RATE_TRAVEL_DIVISOR);
	}

	/*
	 * At zero per-tick travel the original skips initialization of its four
	 * wheel-angle locals. Restore the retained words from the stack window
	 * appropriate to the opponent or player invocation.
	 */
	if (travel_per_tick == 0) {
		if (car_index == OPPONENT_CAR_INDEX) {
			wheel_plane_headings[LEGACY_RESIDUE_FIRST_WORD] =
				legacy_execution_residue.wheel_angle_stack_words[
					LEGACY_RESIDUE_FIRST_WORD];
			wheel_plane_headings[LEGACY_RESIDUE_SECOND_WORD] =
				legacy_execution_residue.wheel_angle_stack_words[
					LEGACY_RESIDUE_SECOND_WORD];
			wheel_plane_headings[LEGACY_RESIDUE_THIRD_WORD] =
				legacy_execution_residue.wheel_angle_stack_words[
					LEGACY_RESIDUE_THIRD_WORD];
			wheel_plane_headings[LEGACY_RESIDUE_FOURTH_WORD] =
				legacy_execution_residue.wheel_angle_stack_words[
					LEGACY_RESIDUE_FOURTH_WORD];
		} else {
			wheel_plane_headings[LEGACY_RESIDUE_FIRST_WORD] =
				legacy_execution_residue.wheel_plane_angles[
					LEGACY_RESIDUE_FIRST_WORD];
			wheel_plane_headings[LEGACY_RESIDUE_SECOND_WORD] =
				legacy_execution_residue.wheel_plane_angles[
					LEGACY_RESIDUE_SECOND_WORD];
			wheel_plane_headings[LEGACY_RESIDUE_THIRD_WORD] =
				legacy_execution_residue.wheel_plane_angles[
					LEGACY_RESIDUE_THIRD_WORD];
			wheel_plane_headings[LEGACY_RESIDUE_FOURTH_WORD] =
				legacy_execution_residue.wheel_plane_angles[
					LEGACY_RESIDUE_FOURTH_WORD];
		}
	}

	/*
	 * On the first stopped frame of a player crash in an opponent race, the
	 * retained words come from rear-opponent wheel coordinates rather than
	 * ordinary wheel angles. Reconstruct those words from explicit game state.
	 */
	if (car_index == PLAYER_CAR_INDEX && gameconfig.game_opponenttype != 0 &&
		travel_per_tick == 0 &&
		carstate->car_lastspeed != CAR_SPEED_STOPPED &&
		carstate->car_crashBmpFlag != CRASH_EVENT_NONE) {
		/*
		 * On the zero-speed crash transition, the original wheel-angle locals
		 * reuse opponent wheel-coordinate words left at the same stack addresses.
		 */
		car_to_world_rotation = *mat_rot_zxy(
			LEGACY_S16_WRAP_NEGATE(state.opponentstate.car_rotate.z),
			LEGACY_S16_WRAP_NEGATE(state.opponentstate.car_rotate.y),
			LEGACY_S16_WRAP_NEGATE(state.opponentstate.car_rotate.x),
			MATRIX_ROTATION_ORDER_ZXY
		);
		/*
		if (opponent has wheel contact &&
			opponent speed <= 30 mph &&
			opponent is upside down)
		{
			wheel_y_adjustment = 192;
		}
		*/
		inverted_wheel_adjustment = 0;
		if (state.opponentstate.car_sumSurfAllWheels != CAR_WHEEL_CONTACT_NONE &&
			state.opponentstate.car_actual_speed <= PLAYER_PHYSICS_LOW_SPEED_LIMIT) {
			wheel_vector.x = 0;
			wheel_vector.y = PLAYER_PHYSICS_UP_VECTOR_LENGTH;
			wheel_vector.z = 0;
			mat_mul_vector(&wheel_vector, &car_to_world_rotation, &transformed_vector);
			if (transformed_vector.y < 0) {
				inverted_wheel_adjustment = PLAYER_PHYSICS_INVERTED_WHEEL_ADJUSTMENT;
			}
		}

		/* Prepare the optional auxiliary wheel rotation once for both wheels. */
		if ((state.opponentstate.car_slide_yaw_delta & ANGLE_MASK) != 0) {
			wheel_adjustment_rotation = *mat_rot_zxy(0, 0,
				LEGACY_S16_WRAP_NEGATE(
					state.opponentstate.car_slide_yaw_delta),
				MATRIX_ROTATION_ORDER_ZXY);
		}

		/*
		 * Rebuild the first rear wheel in car-local coordinates, including
		 * suspension travel and the low-speed inverted-car adjustment.
		 */
		prepare_opponent_rear_wheel(&wheel_vector, &transformed_vector,
			&wheel_adjustment_rotation, PLAYER_PHYSICS_REAR_WHEEL_FIRST,
			inverted_wheel_adjustment);

		/*
		 * Rotate the first rear wheel into world axes and recover the high word
		 * of its world Z coordinate, the first aliased stack word.
		 */
		mat_mul_vector(&wheel_vector, &car_to_world_rotation, &transformed_vector);
		wheel_plane_headings[LEGACY_RESIDUE_FIRST_WORD] = (legacy_u16)(
			(legacy_u32)LEGACY_S32_WRAP_ADD_S16(
				state.opponentstate.car_position.lz, transformed_vector.z) >>
				LEGACY_WORD_BITS
		);
		legacy_execution_residue.wheel_plane_angles[
			LEGACY_RESIDUE_FIRST_WORD] =
			wheel_plane_headings[LEGACY_RESIDUE_FIRST_WORD];

		/* Rebuild the second rear wheel with the same local adjustments. */
		prepare_opponent_rear_wheel(&wheel_vector, &transformed_vector,
			&wheel_adjustment_rotation, PLAYER_PHYSICS_REAR_WHEEL_SECOND,
			inverted_wheel_adjustment);

		/*
		 * Rotate the second rear wheel into world axes and recover the low word
		 * of its world X coordinate, the second aliased stack word.
		 */
		mat_mul_vector(&wheel_vector, &car_to_world_rotation, &transformed_vector);
		wheel_plane_headings[LEGACY_RESIDUE_SECOND_WORD] =
			(legacy_u16)LEGACY_S32_WRAP_ADD_S16(
			state.opponentstate.car_position.lx, transformed_vector.x);

		/* Commit the second word and recover world X's high word as the third. */
		legacy_execution_residue.wheel_plane_angles[
			LEGACY_RESIDUE_SECOND_WORD] =
			wheel_plane_headings[LEGACY_RESIDUE_SECOND_WORD];
		wheel_plane_headings[LEGACY_RESIDUE_THIRD_WORD] = (legacy_u16)(
			(legacy_u32)LEGACY_S32_WRAP_ADD_S16(
				state.opponentstate.car_position.lx, transformed_vector.x) >>
				LEGACY_WORD_BITS
		);

		/* Commit the third word and recover world Y's low word as the fourth. */
		legacy_execution_residue.wheel_plane_angles[
			LEGACY_RESIDUE_THIRD_WORD] =
			wheel_plane_headings[LEGACY_RESIDUE_THIRD_WORD];
		wheel_plane_headings[LEGACY_RESIDUE_FOURTH_WORD] =
			(legacy_u16)LEGACY_S32_WRAP_ADD_S16(
			state.opponentstate.car_position.ly, transformed_vector.y);
		legacy_execution_residue.wheel_plane_angles[
			LEGACY_RESIDUE_FOURTH_WORD] =
			wheel_plane_headings[LEGACY_RESIDUE_FOURTH_WORD];
	}

	car_to_world_rotation = *mat_rot_zxy(
		LEGACY_S16_WRAP_NEGATE(car_working_roll),
		LEGACY_S16_WRAP_NEGATE(car_working_pitch),
		LEGACY_S16_WRAP_NEGATE(car_working_yaw),
		MATRIX_ROTATION_ORDER_ZXY);
	if (car_working_pitch != 0 || car_working_roll != 0) {
		wheel_vector.x = 0;
		wheel_vector.y = 0;
		wheel_vector.z = PLAYER_PHYSICS_PSEUDO_GRAVITY_LENGTH;
		mat_mul_vector(&wheel_vector, &car_to_world_rotation, &transformed_vector);
		carstate->car_pseudoGravity = LEGACY_S16_WRAP_NEGATE(transformed_vector.y);
	} else {
		carstate->car_pseudoGravity = 0;
	}

	if ((carstate->car_slide_yaw_delta & ANGLE_MASK) != 0) {
		aux_rotation_or_obstacle_count = 1;
		wheel_adjustment_rotation = *mat_rot_zxy(0, 0,
			LEGACY_S16_WRAP_NEGATE(carstate->car_slide_yaw_delta),
			MATRIX_ROTATION_ORDER_ZXY);
	} else {
		aux_rotation_or_obstacle_count = 0;
	}

	wheel_vector.x = 0;
	wheel_vector.y = PLAYER_PHYSICS_UP_VECTOR_LENGTH;
	wheel_vector.z = 0;
	mat_mul_vector(&wheel_vector, &car_to_world_rotation, &transformed_vector);
	if (carstate->car_sumSurfAllWheels == CAR_WHEEL_CONTACT_NONE ||
		transformed_vector.y >= 0) {
		inverted_wheel_adjustment = 0;
	} else if (carstate->car_actual_speed <= PLAYER_PHYSICS_LOW_SPEED_LIMIT) {
		inverted_wheel_adjustment = -PLAYER_PHYSICS_INVERTED_WHEEL_ADJUSTMENT;
	} else {
		inverted_wheel_adjustment = PLAYER_PHYSICS_INVERTED_WHEEL_ADJUSTMENT;
		wheel_vector.y = -PLAYER_PHYSICS_INVERTED_WHEEL_ADJUSTMENT;
		mat_mul_vector(&wheel_vector, &car_to_world_rotation, &inverted_wheel_offset);
	}
	wheel_forward_travel.x = 0;
	wheel_forward_travel.y = 0;
	planindex_copy = PLAYER_PHYSICS_PLANE_INDEX_NONE;
	current_wheel_position = current_wheel_positions;
	previous_wheel_position = previous_wheel_positions;
	for (wheel_index = 0;
		wheel_index < PLAYER_PHYSICS_WHEEL_COUNT; wheel_index++) {
	wheel_vector = simd->wheel_coords[wheel_index];
	wheel_vector.y = LEGACY_S16_WRAP_NEGATE(LEGACY_S16_WRAP_ADD(
		carstate->car_suspension_deflection[wheel_index],
		PLAYER_PHYSICS_SUSPENSION_TRAVEL_LIMIT));
	if (inverted_wheel_adjustment < 0)
		wheel_vector.y = LEGACY_S16_WRAP_SUB(wheel_vector.y, inverted_wheel_adjustment);
	if (aux_rotation_or_obstacle_count != 0) {
		mat_mul_vector(&wheel_vector, &wheel_adjustment_rotation, &transformed_vector);
		wheel_vector = transformed_vector;
	}
	mat_mul_vector(&wheel_vector, &car_to_world_rotation, &transformed_vector);
	current_wheel_position->lx = LEGACY_S32_WRAP_ADD_S16(
		car_working_x, transformed_vector.x);
	current_wheel_position->ly = LEGACY_S32_WRAP_ADD_S16(
		car_working_y, transformed_vector.y);
	current_wheel_position->lz = LEGACY_S32_WRAP_ADD_S16(
		car_working_z, transformed_vector.z);

	previous_wheel_position->lx = current_wheel_position->lx;
	previous_wheel_position->ly = current_wheel_position->ly;
	previous_wheel_position->lz = current_wheel_position->lz;
	if (travel_per_tick != 0) {
		wheel_forward_travel.z = travel_per_tick;
		wheel_heading_offset = carstate->car_velocity_heading_offset;
		if (front_wheel_heading_offset != 0 &&
			wheel_index < PLAYER_PHYSICS_FRONT_WHEEL_COUNT) {
			wheel_heading_offset = LEGACY_S16_WRAP_SUB(
				carstate->car_velocity_heading_offset, front_wheel_heading_offset);
		}
		wheel_plane_headings[wheel_index] = wheel_heading_offset;
		legacy_execution_residue.wheel_plane_angles[wheel_index] =
			wheel_heading_offset;
		if (car_index == OPPONENT_CAR_INDEX) {
			legacy_execution_residue.wheel_angle_stack_words[
				wheel_index] = wheel_heading_offset;
		}
		plane_rotate_op();
		physics_position_offset(current_wheel_position, current_wheel_position,
			&wheel_world_travel);
	}
	current_wheel_position++;
	previous_wheel_position++;
	}
	collision_pass_count = 0;
	physics_flow = PLAYER_FLOW_BEGIN_COLLISION_PASS;
	for (;;) {
	switch (physics_flow) {
case PLAYER_FLOW_BEGIN_COLLISION_PASS:
	collision_pass_count = LEGACY_S8_WRAP_ADD(collision_pass_count, 1);
	if (collision_pass_count != PLAYER_PHYSICS_COLLISION_RETRY_LIMIT)
		{ physics_flow = PLAYER_FLOW_BEGIN_WHEEL_CONTACT_SCAN; continue; }
	carstate->car_velocity_heading_offset = ANGLE_HALF_TURN;
	update_crash_state(CRASH_EVENT_COLLISION, car_index);

case PLAYER_FLOW_CHECK_ALL_WHEELS_IN_WATER:
	if (carstate->car_surfaceWhl[PLAYER_PHYSICS_FRONT_WHEEL_FIRST] !=
		CAR_SURFACE_WATER)
		{ physics_flow = PLAYER_FLOW_BEGIN_SUSPENSION_UPDATE; continue; }
	if (carstate->car_surfaceWhl[PLAYER_PHYSICS_FRONT_WHEEL_SECOND] !=
		CAR_SURFACE_WATER)
		{ physics_flow = PLAYER_FLOW_BEGIN_SUSPENSION_UPDATE; continue; }
	if (carstate->car_surfaceWhl[PLAYER_PHYSICS_REAR_WHEEL_FIRST] !=
		CAR_SURFACE_WATER)
		{ physics_flow = PLAYER_FLOW_BEGIN_SUSPENSION_UPDATE; continue; }
	if (carstate->car_surfaceWhl[PLAYER_PHYSICS_REAR_WHEEL_SECOND] !=
		CAR_SURFACE_WATER)
		{ physics_flow = PLAYER_FLOW_BEGIN_SUSPENSION_UPDATE; continue; }
	update_crash_state(CRASH_EVENT_WATER, car_index);

case PLAYER_FLOW_BEGIN_SUSPENSION_UPDATE:
	current_wheel_position = current_wheel_positions;
	wheel_index = 0;
	{ physics_flow = PLAYER_FLOW_CHECK_SUSPENSION_UPDATE_REMAINING; continue; }

case PLAYER_FLOW_BEGIN_WHEEL_CONTACT_SCAN:
	current_wheel_position = current_wheel_positions;
	previous_wheel_position = previous_wheel_positions;
	wheel_index = 0;
	{ physics_flow = PLAYER_FLOW_CHECK_WHEEL_CONTACT_SCAN_REMAINING; continue; }

case PLAYER_FLOW_LOOK_UP_WHEEL_TRACK_OBJECT:
	build_track_object(&wheel_vector, &carstate->car_wheel_contact_positions[wheel_index]);

case PLAYER_FLOW_STORE_WHEEL_SURFACE:
	carstate->car_surfaceWhl[wheel_index] = current_surf_type;
	physics_position_to_vector(&wheel_vector, current_wheel_position);

	if (state.game_inputmode != GAME_INPUT_MODE_INTRO)
		{ physics_flow = PLAYER_FLOW_MEASURE_WHEEL_PLANE_DISTANCE; continue; }
	nextPosAndNormalIP = wheel_vector.y;
	{ physics_flow = PLAYER_FLOW_CHECK_WALL_PRESENT; continue; }

case PLAYER_FLOW_MEASURE_WHEEL_PLANE_DISTANCE:
	nextPosAndNormalIP = plane_origin_op(planindex, wheel_vector.x, wheel_vector.y, wheel_vector.z);

case PLAYER_FLOW_CHECK_WALL_PRESENT:
	if (wallindex != PLAYER_PHYSICS_WALL_INDEX_NONE)
		{ physics_flow = PLAYER_FLOW_CHECK_WALL_LOWER_BOUND; continue; }
	{ physics_flow = PLAYER_FLOW_CLASSIFY_WHEEL_PLANE_DISTANCE; continue; }

case PLAYER_FLOW_CHECK_WALL_LOWER_BOUND:
	if (nextPosAndNormalIP > elRdWallRelated)
		{ physics_flow = PLAYER_FLOW_CHECK_WALL_UPPER_BOUND; continue; }
	{ physics_flow = PLAYER_FLOW_CLASSIFY_WHEEL_PLANE_DISTANCE; continue; }

case PLAYER_FLOW_CHECK_WALL_UPPER_BOUND:
	if (nextPosAndNormalIP < wallHeight)
		{ physics_flow = PLAYER_FLOW_TRANSFORM_WALL_CROSSING; continue; }
	{ physics_flow = PLAYER_FLOW_CLASSIFY_WHEEL_PLANE_DISTANCE; continue; }

case PLAYER_FLOW_TRANSFORM_WALL_CROSSING:
	relative_or_suspension_vector.x = LEGACY_S16_WRAP_SUB(
		carstate->car_wheel_contact_positions[wheel_index].x, wallStartX);
	relative_or_suspension_vector.y = 0;
	relative_or_suspension_vector.z = LEGACY_S16_WRAP_SUB(
		carstate->car_wheel_contact_positions[wheel_index].z, wallStartZ);
	current_relative_position.x = LEGACY_S16_WRAP_SUB(
		position_to_word(current_wheel_position->lx), wallStartX);
	current_relative_position.y = 0;
	current_relative_position.z = LEGACY_S16_WRAP_SUB(
		position_to_word(current_wheel_position->lz), wallStartZ);

	mat_rot_y(&collision_plane_rotation, LEGACY_S16_WRAP_SUB(
		LEGACY_S16_WRAP_NEGATE(wallOrientation), ANGLE_QUARTER_TURN));
	if (car_index == PLAYER_CAR_INDEX) {
		legacy_execution_residue.wheel_angle_stack_words[
			LEGACY_RESIDUE_FIRST_WORD] = collision_plane_rotation.vals[
			PLAYER_PHYSICS_RESIDUE_MATRIX_FIRST_VALUE +
				LEGACY_RESIDUE_FIRST_WORD];
		legacy_execution_residue.wheel_angle_stack_words[
			LEGACY_RESIDUE_SECOND_WORD] = collision_plane_rotation.vals[
			PLAYER_PHYSICS_RESIDUE_MATRIX_FIRST_VALUE +
				LEGACY_RESIDUE_SECOND_WORD];
		legacy_execution_residue.wheel_angle_stack_words[
			LEGACY_RESIDUE_THIRD_WORD] = collision_plane_rotation.vals[
			PLAYER_PHYSICS_RESIDUE_MATRIX_FIRST_VALUE +
				LEGACY_RESIDUE_THIRD_WORD];
		legacy_execution_residue.wheel_angle_stack_words[
			LEGACY_RESIDUE_FOURTH_WORD] = collision_plane_rotation.vals[
			PLAYER_PHYSICS_RESIDUE_MATRIX_FIRST_VALUE +
				LEGACY_RESIDUE_FOURTH_WORD];
	}
	mat_mul_vector(&relative_or_suspension_vector, &collision_plane_rotation, &contact_start_or_delta);
	mat_mul_vector(&current_relative_position, &collision_plane_rotation, &contact_end_or_response);
	if (contact_end_or_response.z <= 0)
		{ physics_flow = PLAYER_FLOW_CHECK_WALL_NEGATIVE_SIDE; continue; }
	if (contact_start_or_delta.z <= 0)
		{ physics_flow = PLAYER_FLOW_CHECK_WALL_NEGATIVE_SIDE; continue; }
	{ physics_flow = PLAYER_FLOW_CLASSIFY_WHEEL_PLANE_DISTANCE; continue; }

case PLAYER_FLOW_CHECK_WALL_NEGATIVE_SIDE:
	if (contact_end_or_response.z >= 0)
		{ physics_flow = PLAYER_FLOW_ORDER_WALL_CROSSING_ENDPOINTS; continue; }
	if (contact_start_or_delta.z >= 0)
		{ physics_flow = PLAYER_FLOW_ORDER_WALL_CROSSING_ENDPOINTS; continue; }
	{ physics_flow = PLAYER_FLOW_CLASSIFY_WHEEL_PLANE_DISTANCE; continue; }

case PLAYER_FLOW_ORDER_WALL_CROSSING_ENDPOINTS:
	if (contact_end_or_response.z <= contact_start_or_delta.z)
		{ physics_flow = PLAYER_FLOW_KEEP_WALL_ENDPOINT_ORDER; continue; }
	contact_response_reversed = 1;
	transformed_vector = contact_end_or_response;
	contact_end_or_response = contact_start_or_delta;
	contact_start_or_delta = transformed_vector;
	{ physics_flow = PLAYER_FLOW_CHECK_WALL_END_ON_PLANE; continue; }

case PLAYER_FLOW_KEEP_WALL_ENDPOINT_ORDER:
	contact_response_reversed = 0;
case PLAYER_FLOW_CHECK_WALL_END_ON_PLANE:
	if (contact_end_or_response.z != 0)
		{ physics_flow = PLAYER_FLOW_CHECK_WALL_START_ON_PLANE; continue; }
	vertical_delta_or_travel = travel_per_tick;
	horizontal_delta_or_travel = 0;
	{ physics_flow = PLAYER_FLOW_SELECT_WALL_RESPONSE_SIDE; continue; }

case PLAYER_FLOW_CHECK_WALL_START_ON_PLANE:
	if (contact_start_or_delta.z != 0)
		{ physics_flow = PLAYER_FLOW_SPLIT_WALL_CROSSING_TRAVEL; continue; }
	vertical_delta_or_travel = 0;
	horizontal_delta_or_travel = travel_per_tick;
	{ physics_flow = PLAYER_FLOW_SELECT_WALL_RESPONSE_SIDE; continue; }

case PLAYER_FLOW_SPLIT_WALL_CROSSING_TRAVEL:
	horizontal_delta_or_travel = scaled_vector_separation(
		&contact_end_or_response, &contact_start_or_delta, &transformed_vector, &intersection_delta_or_body_position);
	vertical_delta_or_travel = LEGACY_S16_WRAP_SUB(travel_per_tick, horizontal_delta_or_travel);

case PLAYER_FLOW_SELECT_WALL_RESPONSE_SIDE:
	contact_angle_distance_or_deflection = LEGACY_S16_FROM_BITS((legacy_u16)LEGACY_S16_WRAP_SUB(
		LEGACY_S16_WRAP_NEGATE(car_working_yaw),
		wallOrientation) & ANGLE_MASK);
	transformed_vector.z = horizontal_delta_or_travel;
	transformed_vector.y = 0;
	if (contact_angle_distance_or_deflection < ANGLE_QUARTER_TURN)
		{ physics_flow = PLAYER_FLOW_PUSH_ALONG_FORWARD_WALL; continue; }
	if (contact_angle_distance_or_deflection <= ANGLE_THREE_QUARTER_TURN)
		{ physics_flow = PLAYER_FLOW_PUSH_ALONG_REVERSE_WALL; continue; }

case PLAYER_FLOW_PUSH_ALONG_FORWARD_WALL:
	contact_angle_distance_or_deflection = wallOrientation;
	transformed_vector.x = PLAYER_PHYSICS_WALL_PUSH_DISTANCE;
	{ physics_flow = PLAYER_FLOW_ADJUST_WALL_PUSH_DIRECTION; continue; }

case PLAYER_FLOW_PUSH_ALONG_REVERSE_WALL:
	contact_angle_distance_or_deflection = LEGACY_S16_FROM_BITS((legacy_u16)
		LEGACY_S16_WRAP_ADD(wallOrientation, ANGLE_HALF_TURN) & ANGLE_MASK);
	transformed_vector.x = -PLAYER_PHYSICS_WALL_PUSH_DISTANCE;

case PLAYER_FLOW_ADJUST_WALL_PUSH_DIRECTION:
	if (contact_response_reversed == 0)
		{ physics_flow = PLAYER_FLOW_ROTATE_WALL_RESPONSE; continue; }
	transformed_vector.x = LEGACY_S16_WRAP_NEGATE(transformed_vector.x);

case PLAYER_FLOW_ROTATE_WALL_RESPONSE:
	response_rotation = mat_rot_zxy(
		LEGACY_S16_WRAP_NEGATE(car_working_roll),
		LEGACY_S16_WRAP_NEGATE(car_working_pitch), contact_angle_distance_or_deflection,
		MATRIX_ROTATION_ORDER_ZXY);
	mat_mul_vector(&transformed_vector, response_rotation, &contact_end_or_response);
	collision_angle_distance_or_index = LEGACY_S16_FROM_BITS((legacy_u16)LEGACY_S16_WRAP_SUB(
		LEGACY_S16_WRAP_NEGATE(car_working_yaw), contact_angle_distance_or_deflection) &
		ANGLE_MASK);
	impact_turn_distance_or_overlap = 0;
	if (collision_angle_distance_or_index <= ANGLE_QUARTER_TURN)
		{ physics_flow = PLAYER_FLOW_CALCULATE_WALL_SAFE_SPEED; continue; }
		collision_angle_distance_or_index = LEGACY_S16_WRAP_SUB(ANGLE_FULL_TURN, collision_angle_distance_or_index);
	impact_turn_distance_or_overlap = 1;

case PLAYER_FLOW_CALCULATE_WALL_SAFE_SPEED:
	wall_safe_speed = LEGACY_U16_SHL(
		(legacy_u8)LEGACY_S16_WRAP_NEGATE(
			LEGACY_S16_WRAP_SUB(LEGACY_S16_SAR(
				LEGACY_S16_WRAP_MUL(collision_angle_distance_or_index,
					PLAYER_PHYSICS_WALL_SPEED_ANGLE_MULTIPLIER),
				PLAYER_PHYSICS_WALL_SPEED_SHIFT),
				PLAYER_PHYSICS_WALL_SPEED_BIAS)),
		PLAYER_PHYSICS_WALL_SPEED_SHIFT);
	if (carstate->car_actual_speed <= wall_safe_speed)
		{ physics_flow = PLAYER_FLOW_BEGIN_WALL_WHEEL_REPOSITION; continue; }
	if (impact_turn_distance_or_overlap == 0)
		{ physics_flow = PLAYER_FLOW_SET_FORWARD_IMPACT_TURN; continue; }
	impact_turn_distance_or_overlap = LEGACY_S16_SHL(LEGACY_S16_WRAP_NEGATE(collision_angle_distance_or_index), 1U);
	{ physics_flow = PLAYER_FLOW_APPLY_WALL_CRASH; continue; }

case PLAYER_FLOW_SET_FORWARD_IMPACT_TURN:
	impact_turn_distance_or_overlap = LEGACY_S16_SHL(collision_angle_distance_or_index, 1U);
case PLAYER_FLOW_APPLY_WALL_CRASH:
	carstate->car_velocity_heading_offset = impact_turn_distance_or_overlap;
	update_crash_state(CRASH_EVENT_COLLISION, car_index);

case PLAYER_FLOW_BEGIN_WALL_WHEEL_REPOSITION:
	carstate->car_sound_flags |= PLAYER_PHYSICS_WALL_SOUND_FLAG;
	current_wheel_position = current_wheel_positions;
	previous_wheel_position = previous_wheel_positions;
	collision_angle_distance_or_index = 0;
	{ physics_flow = PLAYER_FLOW_CHECK_WALL_REPOSITION_REMAINING; continue; }

case PLAYER_FLOW_CLEAR_RETAINED_WHEEL_TRAVEL:
	contact_start_or_delta.x = 0;
	contact_start_or_delta.y = 0;
	contact_start_or_delta.z = 0;

case PLAYER_FLOW_APPLY_WALL_WHEEL_POSITION:
	current_wheel_position->lx = LEGACY_S32_WRAP_ADD_S16(
		LEGACY_S32_WRAP_ADD_S16(previous_wheel_position->lx, contact_start_or_delta.x),
		contact_end_or_response.x);
	current_wheel_position->ly = LEGACY_S32_WRAP_ADD_S16(
		LEGACY_S32_WRAP_ADD_S16(previous_wheel_position->ly, contact_start_or_delta.y),
		contact_end_or_response.y);
	current_wheel_position->lz = LEGACY_S32_WRAP_ADD_S16(
		LEGACY_S32_WRAP_ADD_S16(previous_wheel_position->lz, contact_start_or_delta.z),
		contact_end_or_response.z);
	current_wheel_position++;
	previous_wheel_position++;
	collision_angle_distance_or_index++;

case PLAYER_FLOW_CHECK_WALL_REPOSITION_REMAINING:
	if (collision_angle_distance_or_index < PLAYER_PHYSICS_WHEEL_COUNT)
		{ physics_flow = PLAYER_FLOW_SCALE_RETAINED_WHEEL_TRAVEL; continue; }
	{ physics_flow = PLAYER_FLOW_BEGIN_COLLISION_PASS; continue; }

case PLAYER_FLOW_SCALE_RETAINED_WHEEL_TRAVEL:
	if (vertical_delta_or_travel == 0)
		{ physics_flow = PLAYER_FLOW_CLEAR_RETAINED_WHEEL_TRAVEL; continue; }
	contact_start_or_delta.x = scale_position_delta(current_wheel_position->lx,
		previous_wheel_position->lx, vertical_delta_or_travel, travel_per_tick);
	contact_start_or_delta.y = scale_position_delta(current_wheel_position->ly,
		previous_wheel_position->ly, vertical_delta_or_travel, travel_per_tick);
	contact_start_or_delta.z = scale_position_delta(current_wheel_position->lz,
		previous_wheel_position->lz, vertical_delta_or_travel, travel_per_tick);
	{ physics_flow = PLAYER_FLOW_APPLY_WALL_WHEEL_POSITION; continue; }

case PLAYER_FLOW_APPLY_WHEEL_GRAVITY:
	carstate->car_wheel_vertical_speed[wheel_index] = LEGACY_S16_WRAP_ADD(
		carstate->car_wheel_vertical_speed[wheel_index],
		wheel_gravity_steps[wheel_index]);
	current_wheel_position->ly = LEGACY_S32_WRAP_SUB_S16(
		current_wheel_position->ly, carstate->car_wheel_vertical_speed[wheel_index]);
	if (framespersec != GAME_FRAME_RATE_LOW)
		{ physics_flow = PLAYER_FLOW_RECHECK_DISTANCE_AFTER_GRAVITY; continue; }
	carstate->car_wheel_vertical_speed[wheel_index] = LEGACY_S16_WRAP_ADD(
		carstate->car_wheel_vertical_speed[wheel_index],
		wheel_gravity_steps[wheel_index]);
	current_wheel_position->ly = LEGACY_S32_WRAP_SUB_S16(
		current_wheel_position->ly, carstate->car_wheel_vertical_speed[wheel_index]);

case PLAYER_FLOW_RECHECK_DISTANCE_AFTER_GRAVITY:
	wheel_vector.y = position_to_word(current_wheel_position->ly);
	if (state.game_inputmode == GAME_INPUT_MODE_INTRO) {
		nextPosAndNormalIP = wheel_vector.y;
	} else {
		nextPosAndNormalIP = plane_origin_op(planindex, wheel_vector.x, wheel_vector.y, wheel_vector.z);
	}

case PLAYER_FLOW_CHECK_WHEEL_CONTACT_RANGE:
	if (nextPosAndNormalIP <= PLAYER_PHYSICS_CONTACT_DISTANCE_LIMIT)
		{ physics_flow = PLAYER_FLOW_SAVE_WHEEL_CONTACT_DISTANCE; continue; }
	carstate->car_surfaceWhl[wheel_index] = 0;

case PLAYER_FLOW_SAVE_WHEEL_CONTACT_DISTANCE:
	wheel_contact_distances[wheel_index] = nextPosAndNormalIP;
	if (nextPosAndNormalIP != 0)
		{ physics_flow = PLAYER_FLOW_CHECK_WHEEL_PENETRATION; continue; }
	{ physics_flow = PLAYER_FLOW_CHECK_SUSPENSION_IMPACT_SOUND; continue; }

case PLAYER_FLOW_CHECK_WHEEL_PENETRATION:
	if (nextPosAndNormalIP < 0)
		{ physics_flow = PLAYER_FLOW_TRANSFORM_WHEEL_PLANE_CROSSING; continue; }
	{ physics_flow = PLAYER_FLOW_ADVANCE_WHEEL_CONTACT_SCAN; continue; }

case PLAYER_FLOW_TRANSFORM_WHEEL_PLANE_CROSSING:
	contact_plane = &planptr[planindex];
	plane_world_origin.x = LEGACY_S16_WRAP_ADD(
		contact_plane->plane_origin.x, elem_xCenter);
	plane_world_origin.y = LEGACY_S16_WRAP_ADD(
		contact_plane->plane_origin.y, terrainHeight);
	plane_world_origin.z = LEGACY_S16_WRAP_ADD(
		contact_plane->plane_origin.z, elem_zCenter);

	relative_or_suspension_vector.x = LEGACY_S16_WRAP_SUB(
		position_to_word(previous_wheel_position->lx), plane_world_origin.x);
	relative_or_suspension_vector.y = LEGACY_S16_WRAP_SUB(
		position_to_word(previous_wheel_position->ly), plane_world_origin.y);
	relative_or_suspension_vector.z = LEGACY_S16_WRAP_SUB(
		position_to_word(previous_wheel_position->lz), plane_world_origin.z);

	current_relative_position.x = LEGACY_S16_WRAP_SUB(
		position_to_word(current_wheel_position->lx), plane_world_origin.x);
	current_relative_position.y = LEGACY_S16_WRAP_SUB(
		position_to_word(current_wheel_position->ly), plane_world_origin.y);
	current_relative_position.z = LEGACY_S16_WRAP_SUB(
		position_to_word(current_wheel_position->lz), plane_world_origin.z);

	collision_plane_rotation = contact_plane->plane_rotation;
	if (car_index == PLAYER_CAR_INDEX) {
		legacy_execution_residue.wheel_angle_stack_words[
			LEGACY_RESIDUE_FIRST_WORD] = collision_plane_rotation.vals[
			PLAYER_PHYSICS_RESIDUE_MATRIX_FIRST_VALUE +
				LEGACY_RESIDUE_FIRST_WORD];
		legacy_execution_residue.wheel_angle_stack_words[
			LEGACY_RESIDUE_SECOND_WORD] = collision_plane_rotation.vals[
			PLAYER_PHYSICS_RESIDUE_MATRIX_FIRST_VALUE +
				LEGACY_RESIDUE_SECOND_WORD];
		legacy_execution_residue.wheel_angle_stack_words[
			LEGACY_RESIDUE_THIRD_WORD] = collision_plane_rotation.vals[
			PLAYER_PHYSICS_RESIDUE_MATRIX_FIRST_VALUE +
				LEGACY_RESIDUE_THIRD_WORD];
		legacy_execution_residue.wheel_angle_stack_words[
			LEGACY_RESIDUE_FOURTH_WORD] = collision_plane_rotation.vals[
			PLAYER_PHYSICS_RESIDUE_MATRIX_FIRST_VALUE +
				LEGACY_RESIDUE_FOURTH_WORD];
	}
	mat_invert(&collision_plane_rotation, &wheel_adjustment_rotation);
	mat_mul_vector(&relative_or_suspension_vector, &wheel_adjustment_rotation, &contact_start_or_delta);

	mat_mul_vector(&current_relative_position, &wheel_adjustment_rotation, &contact_end_or_response);
	contact_response_reversed = 0;
	if (track_wall_collision_enabled != 0)
		{ physics_flow = PLAYER_FLOW_CHECK_CROSSING_END_ON_PLANE; continue; }
	if (contact_start_or_delta.y >= -PLAYER_PHYSICS_CONTACT_DISTANCE_LIMIT)
		{ physics_flow = PLAYER_FLOW_CHECK_CROSSING_END_ON_PLANE; continue; }
	if (contact_end_or_response.y >= -PLAYER_PHYSICS_CONTACT_DISTANCE_LIMIT)
		{ physics_flow = PLAYER_FLOW_CHECK_CROSSING_END_ON_PLANE; continue; }
	if (contact_end_or_response.y <= -PLAYER_PHYSICS_INVERTED_CONTACT_DISTANCE_LIMIT)
		{ physics_flow = PLAYER_FLOW_FALL_BACK_TO_GROUND_PLANE; continue; }
	update_crash_state(CRASH_EVENT_IMMEDIATE_STOP, car_index);
	contact_response_reversed = 1;

case PLAYER_FLOW_CHECK_CROSSING_END_ON_PLANE:
	if (contact_end_or_response.y == 0)
		{ physics_flow = PLAYER_FLOW_RETRACE_WHEEL_FROM_PLANE; continue; }
	{ physics_flow = PLAYER_FLOW_CLASSIFY_PLANE_CROSSING_DIRECTION; continue; }

case PLAYER_FLOW_RETRACE_WHEEL_FROM_PLANE:
	wheel_forward_travel.x = 0;
	wheel_forward_travel.y = 0;
	wheel_forward_travel.z = PLAYER_PHYSICS_PLANE_RETRACE_DISTANCE;
	planindex_copy = planindex;
	wheel_heading_offset = wheel_plane_headings[wheel_index];
	plane_rotate_op();
	physics_position_pull_back(current_wheel_position, &wheel_world_travel);
	{ physics_flow = PLAYER_FLOW_FINISH_PLANE_CORRECTION; continue; }

case PLAYER_FLOW_FALL_BACK_TO_GROUND_PLANE:
	planindex = 0;
	current_planptr = planptr;
	track_wall_collision_enabled = 1;
	physics_position_to_vector(&wheel_vector, current_wheel_position);

	nextPosAndNormalIP = plane_origin_op(PLAYER_PHYSICS_GROUND_PLANE_INDEX,
		wheel_vector.x, wheel_vector.y, wheel_vector.z);

case PLAYER_FLOW_CLASSIFY_WHEEL_PLANE_DISTANCE:
	if (nextPosAndNormalIP > 0)
		{ physics_flow = PLAYER_FLOW_CHECK_INVERTED_WHEEL_ADJUSTMENT; continue; }
	{ physics_flow = PLAYER_FLOW_SAVE_WHEEL_CONTACT_DISTANCE; continue; }

case PLAYER_FLOW_CHECK_INVERTED_WHEEL_ADJUSTMENT:
	if (inverted_wheel_adjustment > 0)
		{ physics_flow = PLAYER_FLOW_CHECK_INVERTED_CONTACT_RANGE; continue; }
	{ physics_flow = PLAYER_FLOW_APPLY_WHEEL_GRAVITY; continue; }

case PLAYER_FLOW_CHECK_INVERTED_CONTACT_RANGE:
	if (nextPosAndNormalIP <
		PLAYER_PHYSICS_INVERTED_CONTACT_DISTANCE_LIMIT)
		{ physics_flow = PLAYER_FLOW_APPLY_INVERTED_WHEEL_ADJUSTMENT; continue; }
	{ physics_flow = PLAYER_FLOW_APPLY_WHEEL_GRAVITY; continue; }

case PLAYER_FLOW_APPLY_INVERTED_WHEEL_ADJUSTMENT:
	physics_position_offset(current_wheel_position, current_wheel_position, &inverted_wheel_offset);
	{ physics_flow = PLAYER_FLOW_SAVE_WHEEL_CONTACT_DISTANCE; continue; }

case PLAYER_FLOW_CLASSIFY_PLANE_CROSSING_DIRECTION:
	if (contact_start_or_delta.y <= 0)
		{ physics_flow = PLAYER_FLOW_ADVANCE_WHEEL_ALONG_PLANE; continue; }
	if (contact_end_or_response.y >= 0)
		{ physics_flow = PLAYER_FLOW_ADVANCE_WHEEL_ALONG_PLANE; continue; }
	{ physics_flow = PLAYER_FLOW_SPLIT_PLANE_CROSSING_TRAVEL; continue; }

case PLAYER_FLOW_ADVANCE_WHEEL_ALONG_PLANE:
	wheel_forward_travel.x = 0;
	wheel_forward_travel.y = 0;
	wheel_forward_travel.z = travel_per_tick;
	planindex_copy = planindex;
	wheel_heading_offset = wheel_plane_headings[wheel_index];
	plane_rotate_op();
	physics_position_offset(current_wheel_position, previous_wheel_position,
		&wheel_world_travel);
	{ physics_flow = PLAYER_FLOW_RECHECK_PLANE_PENETRATION; continue; }

case PLAYER_FLOW_SPLIT_PLANE_CROSSING_TRAVEL:
	contact_angle_distance_or_deflection = contact_start_or_delta.z;
	contact_start_or_delta.z = LEGACY_S16_WRAP_NEGATE(contact_start_or_delta.y);
	contact_start_or_delta.y = contact_angle_distance_or_deflection;

	contact_angle_distance_or_deflection = contact_end_or_response.z;
	contact_end_or_response.z = LEGACY_S16_WRAP_NEGATE(contact_end_or_response.y);
	contact_end_or_response.y = contact_angle_distance_or_deflection;
	contact_angle_distance_or_deflection = scaled_vector_separation(
		&contact_end_or_response, &contact_start_or_delta, &transformed_vector, &intersection_delta_or_body_position);
	vertical_delta_or_travel = LEGACY_S16_WRAP_ADD(
		carstate->car_wheel_vertical_speed[wheel_index], travel_per_tick);
	horizontal_delta_or_travel = LEGACY_S16_WRAP_SUB(vertical_delta_or_travel, contact_angle_distance_or_deflection);
	contact_start_or_delta.x = scale_position_delta(current_wheel_position->lx,
		previous_wheel_position->lx, horizontal_delta_or_travel, vertical_delta_or_travel);
	contact_start_or_delta.y = scale_position_delta(current_wheel_position->ly,
		previous_wheel_position->ly, horizontal_delta_or_travel, vertical_delta_or_travel);
	contact_start_or_delta.z = scale_position_delta(current_wheel_position->lz,
		previous_wheel_position->lz, horizontal_delta_or_travel, vertical_delta_or_travel);

	wheel_forward_travel.x = 0;
	wheel_forward_travel.y = 0;
	wheel_forward_travel.z = contact_angle_distance_or_deflection;
	planindex_copy = planindex;
	wheel_heading_offset = wheel_plane_headings[wheel_index];
	plane_rotate_op();
	current_wheel_position->lx = LEGACY_S32_WRAP_ADD_S16(
		LEGACY_S32_WRAP_ADD_S16(
			previous_wheel_position->lx, contact_start_or_delta.x),
		wheel_world_travel.x);
	current_wheel_position->ly = LEGACY_S32_WRAP_ADD_S16(
		LEGACY_S32_WRAP_ADD_S16(
			previous_wheel_position->ly, contact_start_or_delta.y),
		wheel_world_travel.y);
	current_wheel_position->lz = LEGACY_S32_WRAP_ADD_S16(
		LEGACY_S32_WRAP_ADD_S16(
			previous_wheel_position->lz, contact_start_or_delta.z),
		wheel_world_travel.z);

case PLAYER_FLOW_RECHECK_PLANE_PENETRATION:
	physics_position_to_vector(&wheel_vector, current_wheel_position);

	nextPosAndNormalIP = plane_origin_op(planindex, wheel_vector.x, wheel_vector.y, wheel_vector.z);
	if (nextPosAndNormalIP >= 0)
		{ physics_flow = PLAYER_FLOW_FINISH_PLANE_CORRECTION; continue; }
	if (contact_response_reversed == 0)
		{ physics_flow = PLAYER_FLOW_PUSH_WHEEL_OUT_OF_PLANE; continue; }
	nextPosAndNormalIP = LEGACY_S16_WRAP_ADD(
		LEGACY_S16_WRAP_NEGATE(nextPosAndNormalIP),
		PLAYER_PHYSICS_PLANE_PENETRATION_BIAS);

case PLAYER_FLOW_PUSH_WHEEL_OUT_OF_PLANE:
	wheel_vector.z = 0;
	wheel_vector.x = 0;
	wheel_vector.y = LEGACY_S16_SHL(
		LEGACY_S16_WRAP_NEGATE(nextPosAndNormalIP),
		PLAYER_PHYSICS_POSITION_SCALE_SHIFT);
	mat_mul_vector2(&wheel_vector, &planptr[planindex].plane_rotation, &transformed_vector);

	physics_position_offset(current_wheel_position, current_wheel_position, &transformed_vector);

case PLAYER_FLOW_FINISH_PLANE_CORRECTION:
case PLAYER_FLOW_CHECK_SUSPENSION_IMPACT_SOUND:
	if (carstate->car_wheel_vertical_speed[wheel_index] <=
		PLAYER_PHYSICS_SUSPENSION_SOUND_THRESHOLD)
		{ physics_flow = PLAYER_FLOW_CHECK_SUSPENSION_IMPACT_CRASH; continue; }
	carstate->car_sound_flags |= PLAYER_PHYSICS_SUSPENSION_SOUND_FLAG;


case PLAYER_FLOW_CHECK_SUSPENSION_IMPACT_CRASH:
	if (carstate->car_wheel_vertical_speed[wheel_index] <=
		PLAYER_PHYSICS_SUSPENSION_CRASH_THRESHOLD)
		{ physics_flow = PLAYER_FLOW_STOP_WHEEL_VERTICAL_SPEED; continue; }
	update_crash_state(CRASH_EVENT_COLLISION, car_index);

case PLAYER_FLOW_STOP_WHEEL_VERTICAL_SPEED:
	carstate->car_wheel_vertical_speed[wheel_index] = 0;

case PLAYER_FLOW_ADVANCE_WHEEL_CONTACT_SCAN:
	current_wheel_position++;
	previous_wheel_position++;
	wheel_index++;

case PLAYER_FLOW_CHECK_WHEEL_CONTACT_SCAN_REMAINING:
	if (wheel_index < PLAYER_PHYSICS_WHEEL_COUNT)
		{ physics_flow = PLAYER_FLOW_PREPARE_WHEEL_CONTACT_LOOKUP; continue; }
	{ physics_flow = PLAYER_FLOW_CHECK_ALL_WHEELS_IN_WATER; continue; }

case PLAYER_FLOW_PREPARE_WHEEL_CONTACT_LOOKUP:
	physics_position_to_vector(&wheel_vector, current_wheel_position);

	if (state.game_inputmode == GAME_INPUT_MODE_INTRO)
		{ physics_flow = PLAYER_FLOW_USE_INTRO_GROUND_PLANE; continue; }
	{ physics_flow = PLAYER_FLOW_LOOK_UP_WHEEL_TRACK_OBJECT; continue; }

case PLAYER_FLOW_USE_INTRO_GROUND_PLANE:
	wallindex = PLAYER_PHYSICS_WALL_INDEX_NONE;
	current_surf_type = CAR_SURFACE_PAVED;
	planindex = PLAYER_PHYSICS_GROUND_PLANE_INDEX;
	current_planptr = planptr;
	{ physics_flow = PLAYER_FLOW_STORE_WHEEL_SURFACE; continue; }

case PLAYER_FLOW_APPLY_UPRIGHT_SUSPENSION_OFFSET:
	current_wheel_position->ly = LEGACY_S32_WRAP_ADD_S16(
		current_wheel_position->ly, LEGACY_S16_WRAP_ADD(contact_angle_distance_or_deflection,
			PLAYER_PHYSICS_SUSPENSION_TRAVEL_LIMIT));

case PLAYER_FLOW_ADVANCE_SUSPENSION_UPDATE:
	current_wheel_position++;
	wheel_index++;

case PLAYER_FLOW_CHECK_SUSPENSION_UPDATE_REMAINING:
	if (wheel_index < PLAYER_PHYSICS_WHEEL_COUNT)
		{ physics_flow = PLAYER_FLOW_UPDATE_WHEEL_SUSPENSION; continue; }
	{ physics_flow = PLAYER_FLOW_AVERAGE_WHEEL_POSITIONS; continue; }

case PLAYER_FLOW_UPDATE_WHEEL_SUSPENSION:
	physics_position_to_vector(
		&carstate->car_wheel_contact_positions[wheel_index], current_wheel_position);


	contact_angle_distance_or_deflection = update_wheel_suspension(carstate, wheel_contact_distances[wheel_index], wheel_index);
	if (car_working_roll != 0)
		{ physics_flow = PLAYER_FLOW_APPLY_ROTATED_SUSPENSION_OFFSET; continue; }
	if (car_working_pitch != 0)
		{ physics_flow = PLAYER_FLOW_APPLY_ROTATED_SUSPENSION_OFFSET; continue; }
	{ physics_flow = PLAYER_FLOW_APPLY_UPRIGHT_SUSPENSION_OFFSET; continue; }

case PLAYER_FLOW_APPLY_ROTATED_SUSPENSION_OFFSET:
	wheel_vector.z = 0;
	wheel_vector.x = 0;
	wheel_vector.y = LEGACY_S16_WRAP_ADD(contact_angle_distance_or_deflection,
		PLAYER_PHYSICS_SUSPENSION_TRAVEL_LIMIT);
	mat_mul_vector(&wheel_vector, &car_to_world_rotation, &relative_or_suspension_vector);
	physics_position_offset(current_wheel_position, current_wheel_position, &relative_or_suspension_vector);
	{ physics_flow = PLAYER_FLOW_ADVANCE_SUSPENSION_UPDATE; continue; }

case PLAYER_FLOW_AVERAGE_WHEEL_POSITIONS:
	car_working_x = LEGACY_S32_SAR(LEGACY_S32_WRAP_ADD(
		LEGACY_S32_WRAP_ADD(
			current_wheel_positions[PLAYER_PHYSICS_FRONT_WHEEL_FIRST].lx,
			current_wheel_positions[PLAYER_PHYSICS_FRONT_WHEEL_SECOND].lx),
		LEGACY_S32_WRAP_ADD(
			current_wheel_positions[PLAYER_PHYSICS_REAR_WHEEL_FIRST].lx,
			current_wheel_positions[PLAYER_PHYSICS_REAR_WHEEL_SECOND].lx)),
		PLAYER_PHYSICS_WHEEL_CENTROID_SHIFT);
	car_working_y = LEGACY_S32_SAR(LEGACY_S32_WRAP_ADD(
		LEGACY_S32_WRAP_ADD(
			current_wheel_positions[PLAYER_PHYSICS_FRONT_WHEEL_FIRST].ly,
			current_wheel_positions[PLAYER_PHYSICS_FRONT_WHEEL_SECOND].ly),
		LEGACY_S32_WRAP_ADD(
			current_wheel_positions[PLAYER_PHYSICS_REAR_WHEEL_FIRST].ly,
			current_wheel_positions[PLAYER_PHYSICS_REAR_WHEEL_SECOND].ly)),
		PLAYER_PHYSICS_WHEEL_CENTROID_SHIFT);
	car_working_z = LEGACY_S32_SAR(LEGACY_S32_WRAP_ADD(
		LEGACY_S32_WRAP_ADD(
			current_wheel_positions[PLAYER_PHYSICS_FRONT_WHEEL_FIRST].lz,
			current_wheel_positions[PLAYER_PHYSICS_FRONT_WHEEL_SECOND].lz),
		LEGACY_S32_WRAP_ADD(
			current_wheel_positions[PLAYER_PHYSICS_REAR_WHEEL_FIRST].lz,
			current_wheel_positions[PLAYER_PHYSICS_REAR_WHEEL_SECOND].lz)),
		PLAYER_PHYSICS_WHEEL_CENTROID_SHIFT);

	current_wheel_position = current_wheel_positions;
	wheel_index = 0;

case PLAYER_FLOW_CALCULATE_WHEEL_OFFSETS:
	wheel_offsets[wheel_index].x = physics_difference_word(
		current_wheel_position->lx, car_working_x);
	wheel_offsets[wheel_index].y = physics_difference_word(
		current_wheel_position->ly, car_working_y);
	wheel_offsets[wheel_index].z = physics_difference_word(
		current_wheel_position->lz, car_working_z);
	current_wheel_position++;
	wheel_index++;
	if (wheel_index < PLAYER_PHYSICS_WHEEL_COUNT)
		{ physics_flow = PLAYER_FLOW_CALCULATE_WHEEL_OFFSETS; continue; }
	if (car_working_y >= 0)
		{ physics_flow = PLAYER_FLOW_CHECK_WORLD_X_MAXIMUM; continue; }
	car_working_y = 0;

case PLAYER_FLOW_CHECK_WORLD_X_MAXIMUM:
	if (car_working_x <= PLAYER_PHYSICS_WORLD_MAX_EXCLUSIVE)
		{ physics_flow = PLAYER_FLOW_CHECK_WORLD_X_MINIMUM; continue; }


case PLAYER_FLOW_CLAMP_WORLD_X_MAXIMUM:
	car_working_x = PLAYER_PHYSICS_WORLD_MAX_POSITION;
	{ physics_flow = PLAYER_FLOW_CHECK_WORLD_Z_MAXIMUM; continue; }

case PLAYER_FLOW_CHECK_WORLD_X_MINIMUM:
	if (car_working_x >= PLAYER_PHYSICS_WORLD_MIN_POSITION)
		{ physics_flow = PLAYER_FLOW_CHECK_WORLD_Z_MAXIMUM; continue; }


case PLAYER_FLOW_CLAMP_WORLD_X_MINIMUM:
	car_working_x = PLAYER_PHYSICS_WORLD_MIN_POSITION;
case PLAYER_FLOW_CHECK_WORLD_Z_MAXIMUM:
	if (car_working_z <= PLAYER_PHYSICS_WORLD_MAX_EXCLUSIVE)
		{ physics_flow = PLAYER_FLOW_CHECK_WORLD_Z_MINIMUM; continue; }



case PLAYER_FLOW_CLAMP_WORLD_Z_MAXIMUM:
	car_working_z = PLAYER_PHYSICS_WORLD_MAX_POSITION;
	{ physics_flow = PLAYER_FLOW_DERIVE_YAW_FROM_WHEELS; continue; }

case PLAYER_FLOW_CHECK_WORLD_Z_MINIMUM:
	if (car_working_z >= PLAYER_PHYSICS_WORLD_MIN_POSITION)
		{ physics_flow = PLAYER_FLOW_DERIVE_YAW_FROM_WHEELS; continue; }


case PLAYER_FLOW_CLAMP_WORLD_Z_MINIMUM:
	car_working_z = PLAYER_PHYSICS_WORLD_MIN_POSITION;

case PLAYER_FLOW_DERIVE_YAW_FROM_WHEELS:
	contact_angle_distance_or_deflection = wheel_pair_delta(
		wheel_offsets[PLAYER_PHYSICS_REAR_WHEEL_SECOND].x,
		wheel_offsets[PLAYER_PHYSICS_REAR_WHEEL_FIRST].x,
		wheel_offsets[PLAYER_PHYSICS_FRONT_WHEEL_FIRST].x,
		wheel_offsets[PLAYER_PHYSICS_FRONT_WHEEL_SECOND].x);
	horizontal_delta_or_travel = wheel_pair_delta(
		wheel_offsets[PLAYER_PHYSICS_REAR_WHEEL_SECOND].z,
		wheel_offsets[PLAYER_PHYSICS_REAR_WHEEL_FIRST].z,
		wheel_offsets[PLAYER_PHYSICS_FRONT_WHEEL_FIRST].z,
		wheel_offsets[PLAYER_PHYSICS_FRONT_WHEEL_SECOND].z);
	car_working_yaw = LEGACY_S16_FROM_BITS((legacy_u16)
		polarAngle(contact_angle_distance_or_deflection, LEGACY_S16_WRAP_NEGATE(horizontal_delta_or_travel)) & ANGLE_MASK);
	mat_rot_y(&wheel_adjustment_rotation, car_working_yaw);
	wheel_index = 0;

case PLAYER_FLOW_REMOVE_WHEEL_YAW:
	transformed_vector = wheel_offsets[wheel_index];
	mat_mul_vector(&transformed_vector, &wheel_adjustment_rotation, &wheel_offsets[wheel_index]);
	wheel_index++;
	if (wheel_index < PLAYER_PHYSICS_WHEEL_COUNT)
		{ physics_flow = PLAYER_FLOW_REMOVE_WHEEL_YAW; continue; }

	horizontal_delta_or_travel = wheel_pair_delta(
		wheel_offsets[PLAYER_PHYSICS_REAR_WHEEL_SECOND].z,
		wheel_offsets[PLAYER_PHYSICS_REAR_WHEEL_FIRST].z,
		wheel_offsets[PLAYER_PHYSICS_FRONT_WHEEL_FIRST].z,
		wheel_offsets[PLAYER_PHYSICS_FRONT_WHEEL_SECOND].z);
	vertical_delta_or_travel = wheel_pair_delta(
		wheel_offsets[PLAYER_PHYSICS_REAR_WHEEL_SECOND].y,
		wheel_offsets[PLAYER_PHYSICS_REAR_WHEEL_FIRST].y,
		wheel_offsets[PLAYER_PHYSICS_FRONT_WHEEL_FIRST].y,
		wheel_offsets[PLAYER_PHYSICS_FRONT_WHEEL_SECOND].y);
	//horizontal_delta_or_travel = vec_1CC.z + vec_1D2.z - wheel_offsets.z - vec_1D8.z;
	//vertical_delta_or_travel = vec_1CC.y + vec_1D2.y - wheel_offsets.y - vec_1D8.y;
	if (vertical_delta_or_travel != 0)
		{ physics_flow = PLAYER_FLOW_DERIVE_PITCH_FROM_WHEELS; continue; }
	if (horizontal_delta_or_travel < 0)
		{ physics_flow = PLAYER_FLOW_CLEAR_SMALL_PITCH; continue; }

case PLAYER_FLOW_DERIVE_PITCH_FROM_WHEELS:
	car_working_pitch = LEGACY_S16_WRAP_SUB(
		polarAngle(LEGACY_S16_WRAP_NEGATE(horizontal_delta_or_travel), vertical_delta_or_travel),
		ANGLE_QUARTER_TURN);
	if (car_working_pitch >= 0)
		{ physics_flow = PLAYER_FLOW_CHECK_POSITIVE_PITCH_DEADBAND; continue; }
	{ physics_flow = PLAYER_FLOW_CHECK_NEGATIVE_PITCH_DEADBAND; continue; }

case PLAYER_FLOW_CHECK_POSITIVE_PITCH_DEADBAND:
	if (car_working_pitch >= PLAYER_PHYSICS_ROTATION_DEADBAND)
		{ physics_flow = PLAYER_FLOW_PREPARE_WHEEL_PITCH_REMOVAL; continue; }
	{ physics_flow = PLAYER_FLOW_CLEAR_SMALL_PITCH; continue; }
case PLAYER_FLOW_CHECK_NEGATIVE_PITCH_DEADBAND:
	if (LEGACY_S16_WRAP_NEGATE(car_working_pitch) >=
		PLAYER_PHYSICS_ROTATION_DEADBAND)
		{ physics_flow = PLAYER_FLOW_PREPARE_WHEEL_PITCH_REMOVAL; continue; }

case PLAYER_FLOW_CLEAR_SMALL_PITCH:
	car_working_pitch = 0;
case PLAYER_FLOW_PREPARE_WHEEL_PITCH_REMOVAL:
	if (car_working_pitch == 0)
		{ physics_flow = PLAYER_FLOW_MEASURE_WHEEL_ROLL; continue; }
	mat_rot_x(&wheel_adjustment_rotation, car_working_pitch);
	wheel_index = 0;

case PLAYER_FLOW_REMOVE_WHEEL_PITCH:
	transformed_vector = wheel_offsets[wheel_index];
	mat_mul_vector(&transformed_vector, &wheel_adjustment_rotation, &wheel_offsets[wheel_index]);
	wheel_index++;
	if (wheel_index < PLAYER_PHYSICS_WHEEL_COUNT)
		{ physics_flow = PLAYER_FLOW_REMOVE_WHEEL_PITCH; continue; }

case PLAYER_FLOW_MEASURE_WHEEL_ROLL:
	horizontal_delta_or_travel = wheel_pair_delta(
		wheel_offsets[PLAYER_PHYSICS_FRONT_WHEEL_SECOND].x,
		wheel_offsets[PLAYER_PHYSICS_REAR_WHEEL_FIRST].x,
		wheel_offsets[PLAYER_PHYSICS_FRONT_WHEEL_FIRST].x,
		wheel_offsets[PLAYER_PHYSICS_REAR_WHEEL_SECOND].x);
	vertical_delta_or_travel = wheel_pair_delta(
		wheel_offsets[PLAYER_PHYSICS_FRONT_WHEEL_SECOND].y,
		wheel_offsets[PLAYER_PHYSICS_REAR_WHEEL_FIRST].y,
		wheel_offsets[PLAYER_PHYSICS_FRONT_WHEEL_FIRST].y,
		wheel_offsets[PLAYER_PHYSICS_REAR_WHEEL_SECOND].y);

	//horizontal_delta_or_travel = wheel_offsets[3].x + wheel_offsets[2].x - wheel_offsets[0].x - wheel_offsets[1].x;
	//vertical_delta_or_travel = wheel_offsets[3].y + wheel_offsets[2].y - wheel_offsets[0].y - wheel_offsets[1].y;

	//horizontal_delta_or_travel = vec_1D8.x + vec_1D2.x - wheel_offsets.x - vec_1CC.x;
	//vertical_delta_or_travel = vec_1D8.y + vec_1D2.y - wheel_offsets.y - vec_1CC.y;
	if (vertical_delta_or_travel != 0)
		{ physics_flow = PLAYER_FLOW_DERIVE_ROLL_FROM_WHEELS; continue; }
	if (horizontal_delta_or_travel > 0)
		{ physics_flow = PLAYER_FLOW_CLEAR_SMALL_ROLL; continue; }

case PLAYER_FLOW_DERIVE_ROLL_FROM_WHEELS:
	car_working_roll = LEGACY_S16_WRAP_SUB(
		polarAngle(horizontal_delta_or_travel, vertical_delta_or_travel), ANGLE_QUARTER_TURN);
	if (car_working_roll >= 0)
		{ physics_flow = PLAYER_FLOW_CHECK_POSITIVE_ROLL_DEADBAND; continue; }
	{ physics_flow = PLAYER_FLOW_CHECK_NEGATIVE_ROLL_DEADBAND; continue; }

case PLAYER_FLOW_CHECK_POSITIVE_ROLL_DEADBAND:
	if (car_working_roll >= PLAYER_PHYSICS_ROTATION_DEADBAND)
		{ physics_flow = PLAYER_FLOW_SUM_FRONT_AND_REAR_SURFACES; continue; }
	{ physics_flow = PLAYER_FLOW_CLEAR_SMALL_ROLL; continue; }
case PLAYER_FLOW_CHECK_NEGATIVE_ROLL_DEADBAND:
	if (LEGACY_S16_WRAP_NEGATE(car_working_roll) >=
		PLAYER_PHYSICS_ROTATION_DEADBAND)
		{ physics_flow = PLAYER_FLOW_SUM_FRONT_AND_REAR_SURFACES; continue; }
case PLAYER_FLOW_CLEAR_SMALL_ROLL:
	car_working_roll = 0;
case PLAYER_FLOW_SUM_FRONT_AND_REAR_SURFACES:
	carstate->car_sumSurfFrontWheels = LEGACY_S8_WRAP_ADD(
		carstate->car_surfaceWhl[PLAYER_PHYSICS_FRONT_WHEEL_FIRST],
		carstate->car_surfaceWhl[PLAYER_PHYSICS_FRONT_WHEEL_SECOND]);
	carstate->car_sumSurfRearWheels = LEGACY_S8_WRAP_ADD(
		carstate->car_surfaceWhl[PLAYER_PHYSICS_REAR_WHEEL_FIRST],
		carstate->car_surfaceWhl[PLAYER_PHYSICS_REAR_WHEEL_SECOND]);
	if (state.game_inputmode != GAME_INPUT_MODE_INTRO)
		{ physics_flow = PLAYER_FLOW_PLAY_CAR_IMPACT_SOUNDS; continue; }
	{ physics_flow = PLAYER_FLOW_COMMIT_CAR_POSE; continue; }

case PLAYER_FLOW_PLAY_CAR_IMPACT_SOUNDS:
#ifndef RESTUNTS_HEADLESS
	if (is_in_replay != 0)
		{ physics_flow = PLAYER_FLOW_BEGIN_BODY_PLANE_SCAN; continue; }
	if (car_index == PLAYER_CAR_INDEX)
		{ physics_flow = PLAYER_FLOW_PLAY_PLAYER_IMPACT_SOUNDS; continue; }
	audio_play_car_events(carstate->car_sound_flags, audio_opponent_engine_channel);
	{ physics_flow = PLAYER_FLOW_FINISH_CAR_IMPACT_SOUNDS; continue; }

case PLAYER_FLOW_PLAY_PLAYER_IMPACT_SOUNDS:
	audio_play_car_events(carstate->car_sound_flags, audio_player_engine_channel);
case PLAYER_FLOW_FINISH_CAR_IMPACT_SOUNDS:
	//audio_play_car_events(carstate->car_sound_flags, );
#endif

case PLAYER_FLOW_BEGIN_BODY_PLANE_SCAN:
	response_rotation = mat_rot_zxy(
		LEGACY_S16_WRAP_NEGATE(car_working_roll),
		LEGACY_S16_WRAP_NEGATE(car_working_pitch),
		LEGACY_S16_WRAP_NEGATE(car_working_yaw),
		MATRIX_ROTATION_ORDER_ZXY);
	wheel_index = 0;
	{ physics_flow = PLAYER_FLOW_CHECK_BODY_PLANE_SCAN_REMAINING; continue; }

case PLAYER_FLOW_COMPARE_PREVIOUS_BODY_PLANE:
	previous_plane_index = planindex;
	wheel_vector = carstate->car_body_corner_positions[wheel_index];
	build_track_object(&wheel_vector, &intersection_delta_or_body_position);
	if (previous_plane_index != planindex)
		{ physics_flow = PLAYER_FLOW_SAVE_BODY_CORNER_POSITION; continue; }
	impact_turn_distance_or_overlap = plane_origin_op(planindex, wheel_vector.x, wheel_vector.y, wheel_vector.z);
	if (game_replay_mode == REPLAY_MODE_PAUSED)
		{ physics_flow = PLAYER_FLOW_SAVE_BODY_CORNER_POSITION; continue; }
	if (collision_angle_distance_or_index >= 0)
		{ physics_flow = PLAYER_FLOW_CHECK_BODY_CROSSING_FROM_ABOVE; continue; }
	if (impact_turn_distance_or_overlap > 0)
		{ physics_flow = PLAYER_FLOW_STOP_CAR_AT_BODY_PLANE_COLLISION; continue; }

case PLAYER_FLOW_CHECK_BODY_CROSSING_FROM_ABOVE:
	if (collision_angle_distance_or_index <= 0)
		{ physics_flow = PLAYER_FLOW_SAVE_BODY_CORNER_POSITION; continue; }
	if (impact_turn_distance_or_overlap >= 0)
		{ physics_flow = PLAYER_FLOW_SAVE_BODY_CORNER_POSITION; continue; }

case PLAYER_FLOW_STOP_CAR_AT_BODY_PLANE_COLLISION:
	update_crash_state(CRASH_EVENT_IMMEDIATE_STOP, car_index);

case PLAYER_FLOW_SAVE_BODY_CORNER_POSITION:
	carstate->car_body_corner_positions[wheel_index] = intersection_delta_or_body_position;
	wheel_index++;

case PLAYER_FLOW_CHECK_BODY_PLANE_SCAN_REMAINING:
	if (wheel_index < PLAYER_PHYSICS_WHEEL_COUNT)
		{ physics_flow = PLAYER_FLOW_LOOK_UP_BODY_CORNER_PLANE; continue; }
	{ physics_flow = PLAYER_FLOW_DETECT_PLAYER_JUMP; continue; }

case PLAYER_FLOW_LOOK_UP_BODY_CORNER_PLANE:
	wheel_vector = simd->wheel_coords[wheel_index];
	wheel_vector.y = LEGACY_S16_SHL(
		simd->collide_points[0].py,
		PLAYER_PHYSICS_POSITION_SCALE_SHIFT);
	mat_mul_vector(&wheel_vector, response_rotation, &transformed_vector);

	wheel_vector.x = position_to_word(
		LEGACY_S32_WRAP_ADD_S16(car_working_x, transformed_vector.x));
	wheel_vector.y = position_to_word(
		LEGACY_S32_WRAP_ADD_S16(car_working_y, transformed_vector.y));
	wheel_vector.z = position_to_word(
		LEGACY_S32_WRAP_ADD_S16(car_working_z, transformed_vector.z));

	intersection_delta_or_body_position = wheel_vector;
	build_track_object(&wheel_vector, &carstate->car_body_corner_positions[wheel_index]);
	collision_angle_distance_or_index = plane_origin_op(planindex, wheel_vector.x, wheel_vector.y, wheel_vector.z);
	if (planindex < PLAYER_PHYSICS_WHEEL_COUNT)
		{ physics_flow = PLAYER_FLOW_CHECK_BODY_GROUND_PENETRATION; continue; }
	{ physics_flow = PLAYER_FLOW_COMPARE_PREVIOUS_BODY_PLANE; continue; }

case PLAYER_FLOW_CHECK_BODY_GROUND_PENETRATION:
	if (collision_angle_distance_or_index <= 0)
		{ physics_flow = PLAYER_FLOW_HANDLE_BODY_GROUND_PENETRATION; continue; }
	{ physics_flow = PLAYER_FLOW_SAVE_BODY_CORNER_POSITION; continue; }

case PLAYER_FLOW_HANDLE_BODY_GROUND_PENETRATION:
	{ physics_flow = PLAYER_FLOW_STOP_CAR_AT_BODY_PLANE_COLLISION; continue; }
case PLAYER_FLOW_DETECT_PLAYER_JUMP:
	wheel_surface_sum = LEGACY_S8_WRAP_ADD(
		carstate->car_sumSurfFrontWheels,
		carstate->car_sumSurfRearWheels);
	if (car_index == OPPONENT_CAR_INDEX)
		{ physics_flow = PLAYER_FLOW_PREPARE_CAR_COLLISION_POSE; continue; }
	if (wheel_surface_sum != 0)
		{ physics_flow = PLAYER_FLOW_PREPARE_CAR_COLLISION_POSE; continue; }
	if (carstate->car_sumSurfAllWheels == CAR_WHEEL_CONTACT_NONE)
		{ physics_flow = PLAYER_FLOW_PREPARE_CAR_COLLISION_POSE; continue; }
	state.game_jumpCount = LEGACY_S16_WRAP_ADD(state.game_jumpCount, 1);

case PLAYER_FLOW_PREPARE_CAR_COLLISION_POSE:
	carstate->car_sumSurfAllWheels = wheel_surface_sum;
	car_collision_pose[PLAYER_PHYSICS_POSE_POSITION_INDEX].x =
		position_to_word(car_working_x);
	car_collision_pose[PLAYER_PHYSICS_POSE_POSITION_INDEX].y =
		position_to_word(car_working_y);
	car_collision_pose[PLAYER_PHYSICS_POSE_POSITION_INDEX].z =
		position_to_word(car_working_z);

	car_collision_pose[PLAYER_PHYSICS_POSE_ROTATION_INDEX].x =
		car_working_roll;
	car_collision_pose[PLAYER_PHYSICS_POSE_ROTATION_INDEX].y =
		car_working_pitch;
	car_collision_pose[PLAYER_PHYSICS_POSE_ROTATION_INDEX].z =
		car_working_yaw;
	if (gameconfig.game_opponenttype != 0)
		{ physics_flow = PLAYER_FLOW_CHECK_OTHER_CAR_COLLISION; continue; }
	{ physics_flow = PLAYER_FLOW_PREPARE_SCENERY_COLLISION_SCAN; continue; }

case PLAYER_FLOW_CHECK_OTHER_CAR_COLLISION:
	physics_position_to_vector(
		&obstacle_collision_pose[PLAYER_PHYSICS_POSE_POSITION_INDEX],
		&other_carstate->car_position);

	obstacle_collision_pose[PLAYER_PHYSICS_POSE_ROTATION_INDEX].x =
		other_carstate->car_rotate.z;
	obstacle_collision_pose[PLAYER_PHYSICS_POSE_ROTATION_INDEX].y =
		other_carstate->car_rotate.y;
	obstacle_collision_pose[PLAYER_PHYSICS_POSE_ROTATION_INDEX].z =
		other_carstate->car_rotate.x;
	if (car_collision_boxes_overlap(simd->collide_points, car_collision_pose, other_simd->collide_points, obstacle_collision_pose) == 0)
		{ physics_flow = PLAYER_FLOW_PREPARE_SCENERY_COLLISION_SCAN; continue; }
	if (carstate->car_collision_latch == CAR_COLLISION_LATCH_CLEAR)
		{ physics_flow = PLAYER_FLOW_RESOLVE_CAR_COLLISION_SPEEDS; continue; }
	{ physics_flow = PLAYER_FLOW_RETURN_FROM_PHYSICS; continue; }

case PLAYER_FLOW_RESOLVE_CAR_COLLISION_SPEEDS:
	if (resolve_car_collision_speeds(carstate, other_carstate) != 0)
		{ physics_flow = PLAYER_FLOW_CRASH_BOTH_CARS; continue; }
	{ physics_flow = PLAYER_FLOW_RETURN_FROM_PHYSICS; continue; }

case PLAYER_FLOW_CRASH_BOTH_CARS:
	update_crash_state(CRASH_EVENT_COLLISION, car_index);
	update_crash_state(CRASH_EVENT_COLLISION,
		car_index == PLAYER_CAR_INDEX ?
			OPPONENT_CAR_INDEX : PLAYER_CAR_INDEX);
	return;

case PLAYER_FLOW_RETURN_AFTER_COLLISION:
	return;

case PLAYER_FLOW_PREPARE_SCENERY_COLLISION_SCAN:
	transformed_vector.x = LEGACY_S16_SAR(
		car_collision_pose[PLAYER_PHYSICS_POSE_POSITION_INDEX].x,
		PLAYER_PHYSICS_TRACK_COORDINATE_SHIFT);
	transformed_vector.z = LEGACY_S16_WRAP_NEGATE(LEGACY_S16_WRAP_SUB(
		LEGACY_S16_SAR(
			car_collision_pose[PLAYER_PHYSICS_POSE_POSITION_INDEX].z,
			PLAYER_PHYSICS_TRACK_COORDINATE_SHIFT),
		PLAYER_PHYSICS_TRACK_GRID_LAST_COORDINATE));
	obstacle_collision_pose[PLAYER_PHYSICS_POSE_ROTATION_INDEX].x = 0;
	obstacle_collision_pose[PLAYER_PHYSICS_POSE_ROTATION_INDEX].y = 0;
	obstacle_collision_pose[PLAYER_PHYSICS_POSE_ROTATION_INDEX].z = 0;
	if (transformed_vector.x >= 0)
		{ physics_flow = PLAYER_FLOW_CHECK_SCENERY_COLUMN_MAXIMUM; continue; }
	{ physics_flow = PLAYER_FLOW_COMMIT_CAR_POSE; continue; }

case PLAYER_FLOW_CHECK_SCENERY_COLUMN_MAXIMUM:
	if (transformed_vector.x < PLAYER_PHYSICS_TRACK_GRID_SIZE)
		{ physics_flow = PLAYER_FLOW_CHECK_SCENERY_ROW_MINIMUM; continue; }
	{ physics_flow = PLAYER_FLOW_COMMIT_CAR_POSE; continue; }

case PLAYER_FLOW_CHECK_SCENERY_ROW_MINIMUM:
	if (transformed_vector.z >= 0)
		{ physics_flow = PLAYER_FLOW_CHECK_SCENERY_ROW_MAXIMUM; continue; }
	{ physics_flow = PLAYER_FLOW_COMMIT_CAR_POSE; continue; }

case PLAYER_FLOW_CHECK_SCENERY_ROW_MAXIMUM:
	if (transformed_vector.z < PLAYER_PHYSICS_TRACK_GRID_SIZE)
		{ physics_flow = PLAYER_FLOW_LOAD_AUXILIARY_OBSTACLES; continue; }
	{ physics_flow = PLAYER_FLOW_COMMIT_CAR_POSE; continue; }

case PLAYER_FLOW_LOAD_AUXILIARY_OBSTACLES:
	aux_rotation_or_obstacle_count = get_track_collision_points(transformed_vector.x, transformed_vector.z, obstacle_positions);
	if (aux_rotation_or_obstacle_count == 0)
		{ physics_flow = PLAYER_FLOW_FIND_BREAKABLE_OBJECT; continue; }
	collision_angle_distance_or_index = 0;
	{ physics_flow = PLAYER_FLOW_CHECK_AUXILIARY_OBSTACLE_COLLISION; continue; }

case PLAYER_FLOW_ADVANCE_AUXILIARY_OBSTACLE:
	// NOTE: var_144 is unused
	// var_144 += 6;
	collision_angle_distance_or_index++;

case PLAYER_FLOW_CHECK_AUXILIARY_OBSTACLE_COLLISION:
	if (aux_rotation_or_obstacle_count <= collision_angle_distance_or_index)
		{ physics_flow = PLAYER_FLOW_FIND_BREAKABLE_OBJECT; continue; }
	obstacle_collision_pose[PLAYER_PHYSICS_POSE_POSITION_INDEX].x =
		obstacle_positions[collision_angle_distance_or_index].x;
	obstacle_collision_pose[PLAYER_PHYSICS_POSE_POSITION_INDEX].y =
		obstacle_positions[collision_angle_distance_or_index].y;
	obstacle_collision_pose[PLAYER_PHYSICS_POSE_POSITION_INDEX].z =
		obstacle_positions[collision_angle_distance_or_index].z;
	if (car_collision_boxes_overlap(simd->collide_points, car_collision_pose, track_auxiliary_obstacle_bounds, obstacle_collision_pose) == 0)
		{ physics_flow = PLAYER_FLOW_ADVANCE_AUXILIARY_OBSTACLE; continue; }
	carstate->car_velocity_heading_offset = LEGACY_S16_WRAP_SUB(
		carstate->car_velocity_heading_offset, ANGLE_HALF_TURN);

case PLAYER_FLOW_CRASH_INTO_FIXED_OBSTACLE:
	// crash with start/finish pole
	update_crash_state(CRASH_EVENT_COLLISION, car_index);
	return ;

case PLAYER_FLOW_FIND_BREAKABLE_OBJECT:
	collision_angle_distance_or_index = (legacy_s8)roadside_sign_indices_by_tile[trackrows[transformed_vector.z] + transformed_vector.x];
	if (collision_angle_distance_or_index != PLAYER_PHYSICS_TRACK_CAMERA_INDEX_NONE)
		{ physics_flow = PLAYER_FLOW_CHECK_BREAKABLE_OBJECT_INTACT; continue; }
	{ physics_flow = PLAYER_FLOW_CHECK_START_FINISH_COLUMN; continue; }

case PLAYER_FLOW_CHECK_BREAKABLE_OBJECT_INTACT:
	if (state.game_object_destroyed[collision_angle_distance_or_index] == 0)
		{ physics_flow = PLAYER_FLOW_COLLIDE_WITH_BREAKABLE_OBJECT; continue; }
	{ physics_flow = PLAYER_FLOW_CHECK_START_FINISH_COLUMN; continue; }

case PLAYER_FLOW_COLLIDE_WITH_BREAKABLE_OBJECT:
	obstacle_collision_pose[PLAYER_PHYSICS_POSE_POSITION_INDEX].x =
		roadside_sign_positions[collision_angle_distance_or_index].x;
	obstacle_collision_pose[PLAYER_PHYSICS_POSE_POSITION_INDEX].y =
		roadside_sign_positions[collision_angle_distance_or_index].y;
	obstacle_collision_pose[PLAYER_PHYSICS_POSE_POSITION_INDEX].z =
		roadside_sign_positions[collision_angle_distance_or_index].z;
	if (car_collision_boxes_overlap(simd->collide_points, car_collision_pose, breakable_object_bounds, obstacle_collision_pose) == 0)
		{ physics_flow = PLAYER_FLOW_CHECK_START_FINISH_COLUMN; continue; }

	state.game_object_destroyed[collision_angle_distance_or_index] = 1;

	emit_crash_particles(LEGACY_S16_WRAP_ADD(collision_angle_distance_or_index,
		PLAYER_PHYSICS_OBJECT_PARTICLE_KIND_OFFSET),
		LEGACY_S16_WRAP_NEGATE(carstate->car_rotate.x),
		scale_speed_to_travel(carstate->car_actual_speed,
			PLAYER_PHYSICS_NORMAL_RATE_TRAVEL_DIVISOR));

case PLAYER_FLOW_CHECK_START_FINISH_COLUMN:
	// following looks like collision detection against right and left start/finish poles
	if (transformed_vector.x == start_finish_column)
		{ physics_flow = PLAYER_FLOW_CHECK_START_FINISH_ROW; continue; }
	{ physics_flow = PLAYER_FLOW_COMMIT_CAR_POSE; continue; }

case PLAYER_FLOW_CHECK_START_FINISH_ROW:
	if (transformed_vector.z == start_finish_row)
		{ physics_flow = PLAYER_FLOW_CHECK_START_FINISH_POLES; continue; }
	{ physics_flow = PLAYER_FLOW_COMMIT_CAR_POSE; continue; }

case PLAYER_FLOW_CHECK_START_FINISH_POLES:
	obstacle_collision_pose[PLAYER_PHYSICS_POSE_POSITION_INDEX].x =
		LEGACY_S16_WRAP_ADD(
		track_column_centers[start_finish_column], multiply_and_scale(sin_fast(
			LEGACY_S16_WRAP_ADD(track_angle, ANGLE_QUARTER_TURN)),
			PLAYER_PHYSICS_START_FINISH_POLE_OFFSET));
	obstacle_collision_pose[PLAYER_PHYSICS_POSE_POSITION_INDEX].y =
		hillHeightConsts[hillFlag];
	obstacle_collision_pose[PLAYER_PHYSICS_POSE_POSITION_INDEX].z =
		LEGACY_S16_WRAP_ADD(
		track_row_centers[start_finish_row], multiply_and_scale(cos_fast(
			LEGACY_S16_WRAP_ADD(track_angle, ANGLE_QUARTER_TURN)),
			PLAYER_PHYSICS_START_FINISH_POLE_OFFSET));

	impact_turn_distance_or_overlap = car_collision_boxes_overlap(simd->collide_points, car_collision_pose, start_finish_pole_bounds, obstacle_collision_pose);
	if (impact_turn_distance_or_overlap != 0)
		{ physics_flow = PLAYER_FLOW_HANDLE_START_FINISH_POLE_COLLISION; continue; }

	obstacle_collision_pose[PLAYER_PHYSICS_POSE_POSITION_INDEX].x =
		LEGACY_S16_WRAP_ADD(
		track_column_centers[start_finish_column], multiply_and_scale(sin_fast(
			LEGACY_S16_WRAP_ADD(track_angle,
				ANGLE_THREE_QUARTER_TURN)),
			PLAYER_PHYSICS_START_FINISH_POLE_OFFSET));
	obstacle_collision_pose[PLAYER_PHYSICS_POSE_POSITION_INDEX].z =
		LEGACY_S16_WRAP_ADD(
		track_row_centers[start_finish_row], multiply_and_scale(cos_fast(
			LEGACY_S16_WRAP_ADD(track_angle,
				ANGLE_THREE_QUARTER_TURN)),
			PLAYER_PHYSICS_START_FINISH_POLE_OFFSET));

	impact_turn_distance_or_overlap = car_collision_boxes_overlap(simd->collide_points, car_collision_pose, start_finish_pole_bounds, obstacle_collision_pose);

case PLAYER_FLOW_HANDLE_START_FINISH_POLE_COLLISION:
	if (impact_turn_distance_or_overlap == 0)
		{ physics_flow = PLAYER_FLOW_COMMIT_CAR_POSE; continue; }
	{ physics_flow = PLAYER_FLOW_CRASH_INTO_FIXED_OBSTACLE; continue; }

case PLAYER_FLOW_COMMIT_CAR_POSE:
	carstate->car_position.lx = car_working_x;
	carstate->car_position.ly = car_working_y;
	carstate->car_position.lz = car_working_z;
	carstate->car_rotate.z = car_working_roll;
	carstate->car_rotate.y = car_working_pitch;
	carstate->car_rotate.x = car_working_yaw;
	carstate->car_collision_latch = CAR_COLLISION_LATCH_CLEAR;

case PLAYER_FLOW_RETURN_FROM_PHYSICS:
	return ;

	}
	}
}
