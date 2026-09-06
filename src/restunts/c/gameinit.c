#include "restunts.h"

#define CAR_START_LONGITUDINAL_OFFSET 210
#define CAR_START_LATERAL_OFFSET 36
#define CAR_WORLD_POSITION_SHIFT 6U
#define CAR_WORLD_POSITION_SCALE 64L
#define CAR_INITIAL_BODY_HEIGHT 512L
#define CAR_INITIAL_GEAR_INDEX 1U
#define CAR_WHEELS_PER_AXLE (CARSTATE_WHEEL_COUNT / 2U)
#define CAR_INITIAL_SURFACE_GRIP 1000
#define GAMESTATE_CHECKPOINT_INTERVAL_SECONDS 30U
#define TIMER_TICKS_PER_SECOND 100U
#define INITIAL_CAMERA_LATERAL_OFFSET 512
#define INITIAL_CAMERA_DISTANCE 4096
#define INITIAL_CAMERA_TILE_SHIFT 10U
#define INITIAL_CAMERA_HEIGHT 960

enum GAMESTATE_CHECKPOINT_VALIDITY {
	GAMESTATE_CHECKPOINT_INVALID = 0,
	GAMESTATE_CHECKPOINT_VALID = 1
};

#define GAMESTATE_CHECKPOINT_INDEX_STEP 1U
#define GAMESTATE_INITIAL_TIMING_VALUE 1

static legacy_s16 angle_with_offset(legacy_s16 angle, legacy_s16 offset)
{
	return LEGACY_S16_WRAP_ADD(angle, offset);
}

static legacy_s32 track_coordinate_to_world(legacy_s16 base,
	legacy_s16 offset)
{
	legacy_s16 coordinate;

	coordinate = LEGACY_S16_WRAP_ADD(base, offset);
	return LEGACY_S32_SHL((legacy_s32)coordinate,
		CAR_WORLD_POSITION_SHIFT);
}

static void calculate_car_start_offset(legacy_s16 track_direction,
	legacy_s16 side_direction, legacy_s16* column_offset,
	legacy_s16* row_offset)
{
	*column_offset = LEGACY_S16_WRAP_ADD(
		multiply_and_scale(sin_fast(angle_with_offset(
			track_direction, ANGLE_HALF_TURN)),
			CAR_START_LONGITUDINAL_OFFSET),
		multiply_and_scale(sin_fast(angle_with_offset(
			track_direction, side_direction)), CAR_START_LATERAL_OFFSET));
	*row_offset = LEGACY_S16_WRAP_ADD(
		multiply_and_scale(cos_fast(angle_with_offset(
			track_direction, ANGLE_HALF_TURN)),
			CAR_START_LONGITUDINAL_OFFSET),
		multiply_and_scale(cos_fast(angle_with_offset(
			track_direction, side_direction)), CAR_START_LATERAL_OFFSET));
}

static void init_car_at_start(struct CARSTATE* carstate, struct SIMD* simd,
	legacy_s8 transmission, legacy_s16 column_offset,
	legacy_s16 row_offset)
{
	init_carstate_from_simd(
		carstate,
		simd,
		transmission,
		track_coordinate_to_world(
			trackcenterpos2[startcol2], column_offset),
		LEGACY_S32_SHL((legacy_s32)hillHeightConsts[hillFlag],
			CAR_WORLD_POSITION_SHIFT),
		track_coordinate_to_world(
			trackcenterpos[startrow2], row_offset),
		LEGACY_S16_WRAP_NEGATE(track_angle));
}

void init_carstate_from_simd(struct CARSTATE* playerstate, struct SIMD* simd,
	legacy_s8 transmission, legacy_s32 posX, legacy_s32 posY,
	legacy_s32 posZ, legacy_s16 track_angle)
{
	legacy_s16 i;
	struct VECTOR whlPos;

	playerstate->car_posWorld1.lx = posX;
	playerstate->car_posWorld2.lx = posX;
	playerstate->car_posWorld1.ly = LEGACY_S32_WRAP_ADD(
		posY, CAR_INITIAL_BODY_HEIGHT);
	playerstate->car_posWorld2.ly = posY;
	playerstate->car_posWorld1.lz = posZ;
	playerstate->car_posWorld2.lz = posZ;

	playerstate->car_rotate.x = track_angle;
	playerstate->car_rotate.y = 0;
	playerstate->car_rotate.z = 0;
	playerstate->car_36MwhlAngle = 0;
	playerstate->car_pseudoGravity = 0;
	playerstate->car_steeringAngle = CAR_STEERING_CENTERED;
	playerstate->car_is_braking = CAR_PEDAL_RELEASED;
	playerstate->car_is_accelerating = CAR_PEDAL_RELEASED;
	playerstate->car_currpm = simd->idle_rpm;
	playerstate->car_lastrpm = playerstate->car_currpm;
	playerstate->car_idlerpm2 = playerstate->car_currpm;
	playerstate->car_current_gear = CAR_INITIAL_GEAR_INDEX;
	playerstate->car_speeddiff = 0;
	playerstate->car_speed = CAR_SPEED_STOPPED;
	playerstate->car_speed2 = CAR_SPEED_STOPPED;
	playerstate->car_lastspeed = CAR_SPEED_STOPPED;
	playerstate->car_gearratio = simd->gear_ratios[CAR_INITIAL_GEAR_INDEX];
	playerstate->car_gearratioshr8 =
		playerstate->car_gearratio >> LEGACY_BYTE_BITS;
	playerstate->car_knob_x = simd->knob_points[CAR_INITIAL_GEAR_INDEX].px;
	playerstate->car_knob_x2 = playerstate->car_knob_x;
	playerstate->car_knob_y = simd->knob_points[CAR_INITIAL_GEAR_INDEX].py;
	playerstate->car_knob_y2 = playerstate->car_knob_y;
	playerstate->car_angle_z = 0;
	playerstate->car_40MfrontWhlAngle = 0;
	playerstate->field_42 = 0;
	playerstate->field_48 = 0;
	playerstate->car_trackdata3_index = 0;
	playerstate->car_sumSurfFrontWheels = CAR_WHEELS_PER_AXLE;
	playerstate->car_sumSurfRearWheels = CAR_WHEELS_PER_AXLE;
	playerstate->car_sumSurfAllWheels = CARSTATE_WHEEL_COUNT;
	playerstate->car_demandedGrip = 0;
	playerstate->car_surfacegrip_sum = CAR_INITIAL_SURFACE_GRIP;

	whlPos.x = LEGACY_S16_FROM_BITS((legacy_u16)
		LEGACY_S32_DIV_OR_ZERO(posX, CAR_WORLD_POSITION_SCALE));
	whlPos.y = LEGACY_S16_FROM_BITS((legacy_u16)
		LEGACY_S32_DIV_OR_ZERO(posY, CAR_WORLD_POSITION_SCALE));
	whlPos.z = LEGACY_S16_FROM_BITS((legacy_u16)
		LEGACY_S32_DIV_OR_ZERO(posZ, CAR_WORLD_POSITION_SCALE));

	for (i = 0; i < CARSTATE_WHEEL_COUNT; ++i) {
		playerstate->car_surfaceWhl[i] = CAR_SURFACE_PAVED;
		playerstate->car_rc1[i] = 0;
		playerstate->car_rc2[i] = 0;
		playerstate->car_rc3[i] = 0;
		playerstate->car_rc4[i] = 0;
		playerstate->car_rc5[i] = 0;

		playerstate->car_whlWorldCrds1[i] = whlPos;
		playerstate->car_whlWorldCrds2[i] = whlPos;
	}

	playerstate->car_engineLimiterTimer = 0;
	playerstate->car_slidingFlag = CAR_SLIDING_INACTIVE;
	playerstate->field_C8 = CAR_COLLISION_LATCH_CLEAR;
	playerstate->car_crashBmpFlag = CRASH_EVENT_NONE;
	playerstate->car_changing_gear = CAR_GEAR_CHANGE_INACTIVE;
	playerstate->car_fpsmul2 = GEAR_CHANGE_DELAY_EXPIRED;
	playerstate->car_transmission = transmission;
	playerstate->field_CD = 0;
	playerstate->field_CE = ROUTE_POINT_FIRST;
	playerstate->field_CF = CAR_SOUND_ENGINE_ACTIVE_FLAG;
}

void init_game_state(legacy_s16 arg)
{
	legacy_s16 i, tmpcol, tmprow;
	legacy_s16 route_track_index;
	legacy_u16 route_table_offset;
	legacy_u8 route_point;

	if (arg == GAMESTATE_INIT_RESET_CHECKPOINTS) {
		elapsed_time1 = 0;
		for (i = 0; i < GAMESTATE_CHECKPOINT_COUNT; ++i)
			cvxptr[i].field_3F4 = GAMESTATE_CHECKPOINT_INVALID;
	}

	if (framespersec == GAME_FRAME_RATE_LOW)
		steerWhlRespTable_ptr = steerWhlRespTable_10fps;
	else
		steerWhlRespTable_ptr = steerWhlRespTable_20fps;

	word_45A00 = LEGACY_S16_FROM_BITS(
		LEGACY_U16_WRAP_MUL(framespersec,
			GAMESTATE_CHECKPOINT_INTERVAL_SECONDS));
	word_4499C = LEGACY_S16_FROM_BITS(
		LEGACY_U16_DIV_OR_ZERO(TIMER_TICKS_PER_SECOND, framespersec));

	if (arg != GAMESTATE_INIT_TIMING_ONLY) {
		init_unknown();

		state.field_3F4 = GAMESTATE_CHECKPOINT_VALID;
		state.game_frames_per_sec = GAMESTATE_INITIAL_TIMING_VALUE;
		state.game_inputmode = GAME_INPUT_MODE_WAITING;
		state.game_3F6autoLoadEvalFlag = 0;
		state.game_frame_in_sec = 0;
		state.field_2F4 = 0;
		state.field_3F7[0] = 0;
		state.field_3F7[1] = 0;

		for (i = 0; i < GAMESTATE_FIELD_3FA_SIZE; ++i)
			state.field_3FA[i] = 0;
		for (i = 0; i < GAMESTATE_PARTICLE_SLOT_COUNT; ++i)
			state.field_38E[i] = 0;

		state.game_vec1[PLAYER_CAR_INDEX].x =
			LEGACY_S16_WRAP_ADD(LEGACY_S16_WRAP_ADD(
				multiply_and_scale(sin_fast(angle_with_offset(
					track_angle, ANGLE_THREE_QUARTER_TURN)),
					INITIAL_CAMERA_LATERAL_OFFSET),
				multiply_and_scale(sin_fast(angle_with_offset(
					track_angle, ANGLE_HALF_TURN)), INITIAL_CAMERA_DISTANCE)),
				LEGACY_S16_SHL((legacy_s16)startcol2,
					INITIAL_CAMERA_TILE_SHIFT));
		state.game_vec1[PLAYER_CAR_INDEX].y = LEGACY_S16_WRAP_ADD(
			hillHeightConsts[hillFlag], INITIAL_CAMERA_HEIGHT);
		state.game_vec1[PLAYER_CAR_INDEX].z =
			LEGACY_S16_WRAP_ADD(LEGACY_S16_WRAP_ADD(
				multiply_and_scale(cos_fast(angle_with_offset(
					track_angle, ANGLE_HALF_TURN)), INITIAL_CAMERA_DISTANCE),
				trackpos[startrow2]),
				multiply_and_scale(cos_fast(angle_with_offset(
					track_angle, ANGLE_THREE_QUARTER_TURN)),
					INITIAL_CAMERA_LATERAL_OFFSET));

		state.game_vec1[OPPONENT_CAR_INDEX] =
			state.game_vec1[PLAYER_CAR_INDEX];
		state.game_vec3 = state.game_vec1[PLAYER_CAR_INDEX];
		state.game_vec4 = state.game_vec1[PLAYER_CAR_INDEX];
		state.game_travDist = 0;
		state.game_frame = 0;
		state.game_total_finish = 0;
		state.field_144 = 0;
		state.game_pEndFrame = 0;
		state.game_oEndFrame = 0;
		state.game_penalty = 0;
		state.game_impactSpeed = 0;
		state.game_topSpeed = 0;
		state.game_jumpCount = 0;

		calculate_car_start_offset(track_angle, ANGLE_QUARTER_TURN,
			&tmpcol, &tmprow);

		init_car_at_start(
			&state.playerstate,
			&simd_player,
			gameconfig.game_playertransmission,
			tmpcol,
			tmprow);

		state.field_2F2 = 0;
		state.field_45D = ROUTE_INDICATOR_NONE;
		state.field_45E = ROUTE_INDICATOR_NONE;
		state.field_45B = ROUTE_TRACKING_NORMAL;
		state.field_45C = ROUTE_CONFIRMATION_NONE;
		state.game_startcol = startcol2;
		state.game_startcol2 = startcol2;
		state.game_startrow = startrow2;
		state.game_startrow2 = startrow2;

		if (arg != GAMESTATE_INIT_SKIP_ROUTE_SETUP) {
			route_point = (legacy_u8)state.playerstate.field_CE;
			sub_18D60(
				state.playerstate.car_trackdata3_index,
				&state.playerstate.car_vec_unk3,
				(legacy_s16)route_point,
				0);
			state.playerstate.field_CE = LEGACY_S8_WRAP_ADD(
				route_point, ROUTE_POINT_STEP);
		}

		calculate_car_start_offset(track_angle, ANGLE_THREE_QUARTER_TURN,
			&tmpcol, &tmprow);

		init_car_at_start(
			&state.opponentstate,
			&simd_opponent,
			TRANSMISSION_AUTOMATIC,
			tmpcol,
			tmprow);

		if (gameconfig.game_opponenttype &&
			arg != GAMESTATE_INIT_SKIP_ROUTE_SETUP) {
			route_point = (legacy_u8)state.opponentstate.field_CE;
			opponent_route_advance((legacy_s16)route_point);
			state.opponentstate.field_CE = LEGACY_S8_WRAP_ADD(
				route_point, ROUTE_POINT_STEP);
		}

		state.field_42A = 0;
	}
}

static void init_game_state_with_rates(legacy_u16 frame_rate,
	legacy_u16 stored_frame_rate)
{
	framespersec = frame_rate;
	gameconfig.game_framespersec = stored_frame_rate;
	init_game_state(GAMESTATE_INIT_RESET_CHECKPOINTS);
}

void init_game_state_with_frame_rate(legacy_u16 frame_rate)
{
	init_game_state_with_rates(frame_rate, frame_rate);
}

void init_game_state_with_frame_rate_byte(legacy_u16 frame_rate)
{
	init_game_state_with_rates(frame_rate,
		LEGACY_U16_REPLACE_LOW_BYTE(
			gameconfig.game_framespersec, frame_rate));
}

void restore_gamestate(legacy_u16 frame)
{
	legacy_u16 curframe;

	if (frame == 0 && elapsed_time1 == 0)
		init_game_state(GAMESTATE_INIT_NORMAL);

	curframe = LEGACY_U16_DIV_OR_ZERO(frame, word_45A00);
	if (curframe == GAMESTATE_CHECKPOINT_COUNT)
		curframe = LEGACY_U16_WRAP_SUB(
			curframe, GAMESTATE_CHECKPOINT_INDEX_STEP);

	/* Find the newest valid checkpoint preceding the requested frame. */
	if (frame >= state.game_frame) {
		while (1) {
			if (LEGACY_U16_WRAP_MUL(curframe, word_45A00) <=
				state.game_frame)
				return;
			if (cvxptr[curframe].field_3F4 != GAMESTATE_CHECKPOINT_INVALID)
				break;
			/* A newly loaded replay has no checkpoints yet.  Stop at the
			 * initial state instead of wrapping before the checkpoint array. */
			if (curframe == 0)
				return;
			curframe = LEGACY_U16_WRAP_SUB(
				curframe, GAMESTATE_CHECKPOINT_INDEX_STEP);
		}
	}

	state = cvxptr[curframe];
	init_kevinrandom(state.kevinseed);
	elapsed_time2 = state.game_frame;
}
