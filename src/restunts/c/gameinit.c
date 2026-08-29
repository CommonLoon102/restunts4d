#include "restunts.h"

#define RST_CVX_NUM 20

void init_carstate_from_simd(struct CARSTATE* playerstate, struct SIMD* simd,
	legacy_s8 transmission, legacy_s32 posX, legacy_s32 posY,
	legacy_s32 posZ, legacy_s16 track_angle)
{
	legacy_s16 i;
	struct VECTOR whlPos;

	playerstate->car_posWorld1.lx = posX;
	playerstate->car_posWorld2.lx = posX;
	playerstate->car_posWorld1.ly = posY + 512;
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

	whlPos.x = posX / 64;
	whlPos.y = posY / 64;
	whlPos.z = posZ / 64;

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

void restore_gamestate(legacy_u16 frame)
{
	legacy_u16 curframe;

	if (frame == 0 && elapsed_time1 == 0)
		init_game_state(0);

	curframe = LEGACY_U16_DIV_OR_ZERO(frame, word_45A00);
	if (curframe == RST_CVX_NUM)
		curframe--;

	/* Find the newest valid checkpoint preceding the requested frame. */
	if (frame >= state.game_frame) {
		while (1) {
			if (LEGACY_U16_WRAP_MUL(curframe, word_45A00) <=
				state.game_frame)
				return;
			if (((struct GAMESTATE far*)cvxptr)[curframe].field_3F4 != 0)
				break;
			curframe--;
		}
	}

	state = ((struct GAMESTATE far*)cvxptr)[curframe];
	init_kevinrandom(state.kevinseed);
	elapsed_time2 = state.game_frame;
}
