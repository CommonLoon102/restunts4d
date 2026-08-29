#include "restunts.h"

#define RST_CVX_NUM 20

static legacy_s16 angle_with_offset(legacy_s16 angle, legacy_s16 offset)
{
	return LEGACY_S16_WRAP_ADD(angle, offset);
}

static legacy_s32 track_coordinate_to_world(legacy_s16 base,
	legacy_s16 offset)
{
	legacy_s16 coordinate;

	coordinate = LEGACY_S16_WRAP_ADD(base, offset);
	return LEGACY_S32_SHL((legacy_s32)coordinate, 6U);
}

void init_carstate_from_simd(struct CARSTATE* playerstate, struct SIMD* simd,
	legacy_s8 transmission, legacy_s32 posX, legacy_s32 posY,
	legacy_s32 posZ, legacy_s16 track_angle)
{
	legacy_s16 i;
	struct VECTOR whlPos;

	playerstate->car_posWorld1.lx = posX;
	playerstate->car_posWorld2.lx = posX;
	playerstate->car_posWorld1.ly = LEGACY_S32_WRAP_ADD(posY, 512L);
	playerstate->car_posWorld2.ly = posY;
	playerstate->car_posWorld1.lz = posZ;
	playerstate->car_posWorld2.lz = posZ;

	playerstate->car_rotate.x = track_angle;
	playerstate->car_rotate.y = 0;
	playerstate->car_rotate.z = 0;
	playerstate->car_36MwhlAngle = 0;
	playerstate->car_pseudoGravity = 0;
	playerstate->car_steeringAngle = 0;
	playerstate->car_is_braking = 0;
	playerstate->car_is_accelerating = 0;
	playerstate->car_currpm = simd->idle_rpm;
	playerstate->car_lastrpm = playerstate->car_currpm;
	playerstate->car_idlerpm2 = playerstate->car_currpm;
	playerstate->car_current_gear = 1;
	playerstate->car_speeddiff = 0;
	playerstate->car_speed = 0;
	playerstate->car_speed2 = 0;
	playerstate->car_lastspeed = 0;
	playerstate->car_gearratio = simd->gear_ratios[1];
	playerstate->car_gearratioshr8 = playerstate->car_gearratio >> 8;
	playerstate->car_knob_x = simd->knob_points[1].px;
	playerstate->car_knob_x2 = playerstate->car_knob_x;
	playerstate->car_knob_y = simd->knob_points[1].py;
	playerstate->car_knob_y2 = playerstate->car_knob_y;
	playerstate->car_angle_z = 0;
	playerstate->car_40MfrontWhlAngle = 0;
	playerstate->field_42 = 0;
	playerstate->field_48 = 0;
	playerstate->car_trackdata3_index = 0;
	playerstate->car_sumSurfFrontWheels = 2;
	playerstate->car_sumSurfRearWheels = 2;
	playerstate->car_sumSurfAllWheels = 4;
	playerstate->car_demandedGrip = 0;
	playerstate->car_surfacegrip_sum = 1000;

	whlPos.x = LEGACY_S16_FROM_BITS((legacy_u16)
		LEGACY_S32_DIV_OR_ZERO(posX, 64L));
	whlPos.y = LEGACY_S16_FROM_BITS((legacy_u16)
		LEGACY_S32_DIV_OR_ZERO(posY, 64L));
	whlPos.z = LEGACY_S16_FROM_BITS((legacy_u16)
		LEGACY_S32_DIV_OR_ZERO(posZ, 64L));

	for (i = 0; i < 4; ++i) {
		playerstate->car_surfaceWhl[i] = 1;
		playerstate->car_rc1[i] = 0;
		playerstate->car_rc2[i] = 0;
		playerstate->car_rc3[i] = 0;
		playerstate->car_rc4[i] = 0;
		playerstate->car_rc5[i] = 0;

		playerstate->car_whlWorldCrds1[i] = whlPos;
		playerstate->car_whlWorldCrds2[i] = whlPos;
	}

	playerstate->car_engineLimiterTimer = 0;
	playerstate->car_slidingFlag = 0;
	playerstate->field_C8 = 0;
	playerstate->car_crashBmpFlag = 0;
	playerstate->car_changing_gear = 0;
	playerstate->car_fpsmul2 = 0;
	playerstate->car_transmission = transmission;
	playerstate->field_CD = 0;
	playerstate->field_CE = 0;
	playerstate->field_CF = 1;
}

void init_game_state(legacy_s16 arg)
{
	legacy_s16 i, tmpcol, tmprow;
	legacy_s16 route_track_index;
	legacy_u16 route_table_offset;
	legacy_u8 route_point;

	if (arg == -1) {
		elapsed_time1 = 0;
		for (i = 0; i < RST_CVX_NUM; ++i)
			((struct GAMESTATE far*)cvxptr)[i].field_3F4 = 0;
	}

	if (framespersec == 10)
		steerWhlRespTable_ptr = steerWhlRespTable_10fps;
	else
		steerWhlRespTable_ptr = steerWhlRespTable_20fps;

	word_45A00 = LEGACY_S16_FROM_BITS(
		LEGACY_U16_WRAP_MUL(framespersec, 30U));
	word_4499C = LEGACY_S16_FROM_BITS(
		LEGACY_U16_DIV_OR_ZERO(100U, framespersec));

	if (arg != -3) {
		init_unknown();

		state.field_3F4 = 1;
		state.game_frames_per_sec = 1;
		state.game_inputmode = 0;
		state.game_3F6autoLoadEvalFlag = 0;
		state.game_frame_in_sec = 0;
		state.field_2F4 = 0;
		state.field_3F7[0] = 0;
		state.field_3F7[1] = 0;

		for (i = 0; i < 48; ++i)
			state.field_3FA[i] = 0;
		for (i = 0; i < 24; ++i)
			state.field_38E[i] = 0;

		state.game_vec1[0].x =
			LEGACY_S16_WRAP_ADD(LEGACY_S16_WRAP_ADD(
				multiply_and_scale(sin_fast(angle_with_offset(
					track_angle, 0x300)), 512),
				multiply_and_scale(sin_fast(angle_with_offset(
					track_angle, 0x200)), 4096)),
				LEGACY_S16_SHL((legacy_s16)startcol2, 10U));
		state.game_vec1[0].y = LEGACY_S16_WRAP_ADD(
			hillHeightConsts[hillFlag], 960);
		state.game_vec1[0].z =
			LEGACY_S16_WRAP_ADD(LEGACY_S16_WRAP_ADD(
				multiply_and_scale(cos_fast(angle_with_offset(
					track_angle, 0x200)), 4096),
				trackpos[startrow2]),
				multiply_and_scale(cos_fast(angle_with_offset(
					track_angle, 0x300)), 512));

		state.game_vec1[1] = state.game_vec1[0];
		state.game_vec3 = state.game_vec1[0];
		state.game_vec4 = state.game_vec1[0];
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

		tmpcol = LEGACY_S16_WRAP_ADD(
			multiply_and_scale(sin_fast(angle_with_offset(
				track_angle, 0x200)), 210),
			multiply_and_scale(sin_fast(angle_with_offset(
				track_angle, 0x100)), 36));
		tmprow = LEGACY_S16_WRAP_ADD(
			multiply_and_scale(cos_fast(angle_with_offset(
				track_angle, 0x200)), 210),
			multiply_and_scale(cos_fast(angle_with_offset(
				track_angle, 0x100)), 36));

		init_carstate_from_simd(
			&state.playerstate,
			&simd_player,
			gameconfig.game_playertransmission,
			track_coordinate_to_world(
				trackcenterpos2[startcol2], tmpcol),
			LEGACY_S32_SHL((legacy_s32)hillHeightConsts[hillFlag], 6U),
			track_coordinate_to_world(
				trackcenterpos[startrow2], tmprow),
			LEGACY_S16_WRAP_NEGATE(track_angle));

		state.field_2F2 = 0;
		state.field_45D = 0;
		state.field_45E = 0;
		state.field_45B = 0;
		state.field_45C = 0;
		state.game_startcol = startcol2;
		state.game_startcol2 = startcol2;
		state.game_startrow = startrow2;
		state.game_startrow2 = startrow2;

		if (arg != -2) {
			route_point = (legacy_u8)state.playerstate.field_CE;
			sub_18D60(
				state.playerstate.car_trackdata3_index,
				&state.playerstate.car_vec_unk3,
				(legacy_s16)route_point,
				0);
			state.playerstate.field_CE = LEGACY_S8_FROM_BITS(
				(legacy_u8)(route_point + 1U));
		}

		tmpcol = LEGACY_S16_WRAP_ADD(
			multiply_and_scale(sin_fast(angle_with_offset(
				track_angle, 0x200)), 210),
			multiply_and_scale(sin_fast(angle_with_offset(
				track_angle, 0x300)), 36));
		tmprow = LEGACY_S16_WRAP_ADD(
			multiply_and_scale(cos_fast(angle_with_offset(
				track_angle, 0x200)), 210),
			multiply_and_scale(cos_fast(angle_with_offset(
				track_angle, 0x300)), 36));

		init_carstate_from_simd(
			&state.opponentstate,
			&simd_opponent,
			1,
			track_coordinate_to_world(
				trackcenterpos2[startcol2], tmpcol),
			LEGACY_S32_SHL((legacy_s32)hillHeightConsts[hillFlag], 6U),
			track_coordinate_to_world(
				trackcenterpos[startrow2], tmprow),
			LEGACY_S16_WRAP_NEGATE(track_angle));

		if (gameconfig.game_opponenttype && arg != -2) {
			route_point = (legacy_u8)state.opponentstate.field_CE;
			route_table_offset = LEGACY_U16_WRAP_MUL(
				state.opponentstate.car_trackdata3_index, 2U);
			route_track_index = LEGACY_READ_S16_LE(
				(const legacy_u8 far*)trackdata3 + route_table_offset);
			sub_18D60(
				route_track_index,
				&state.opponentstate.car_vec_unk3,
				(legacy_s16)route_point,
				(legacy_s16*)&state.field_3F9);
			state.opponentstate.field_CE = LEGACY_S8_FROM_BITS(
				(legacy_u8)(route_point + 1U));
		}

		state.field_42A = 0;
	}
}

void restore_gamestate(legacy_u16 frame)
{
	legacy_u16 curframe;

	if (frame == 0 && elapsed_time1 == 0)
		init_game_state(0);

	curframe = LEGACY_U16_DIV_OR_ZERO(frame, word_45A00);
	if (curframe == RST_CVX_NUM)
		curframe = LEGACY_U16_WRAP_SUB(curframe, 1U);

	/* Find the newest valid checkpoint preceding the requested frame. */
	if (frame >= state.game_frame) {
		while (1) {
			if (LEGACY_U16_WRAP_MUL(curframe, word_45A00) <=
				state.game_frame)
				return;
			if (((struct GAMESTATE far*)cvxptr)[curframe].field_3F4 != 0)
				break;
			curframe = LEGACY_U16_WRAP_SUB(curframe, 1U);
		}
	}

	state = ((struct GAMESTATE far*)cvxptr)[curframe];
	init_kevinrandom(state.kevinseed);
	elapsed_time2 = state.game_frame;
}
