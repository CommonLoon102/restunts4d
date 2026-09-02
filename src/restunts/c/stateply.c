#include "externs.h"
#include "legacy.h"
#include "math.h"
#include "residue.h"

extern legacy_s32 pState_lvec1_x;
extern legacy_s32 pState_lvec1_y;
extern legacy_s32 pState_lvec1_z;
extern legacy_s16 pState_minusRotate_z_1;
extern legacy_s16 pState_minusRotate_z_2;
extern legacy_s16 pState_minusRotate_y_1;
extern legacy_s16 pState_minusRotate_y_2;
extern legacy_s16 pState_minusRotate_x_1;
extern legacy_s16 pState_minusRotate_x_2;
extern struct MATRIX mat_unk;
extern struct VECTOR vec_unk2;
extern legacy_s16 planindex;
extern legacy_s16 planindex_copy;
extern legacy_s16 pState_f36Mminf40sar2;
extern struct VECTOR vec_planerotopresult;
extern legacy_s8 current_surf_type;
extern legacy_s16 nextPosAndNormalIP;
extern legacy_s16 wallindex;
extern legacy_s16 elRdWallRelated;
extern legacy_s16 wallHeight;
extern legacy_s16 wallStartX;
extern legacy_s16 wallStartZ;
extern legacy_s16 wallOrientation;
extern struct PLANE far* planptr;
extern struct PLANE far* current_planptr;
extern legacy_s16 elem_xCenter;
extern legacy_s16 elem_zCenter;
extern legacy_s16 terrainHeight;
extern legacy_s8 byte_4392C;

extern struct POINT2D unk_3BD62[2];
extern struct POINT2D unk_3BD5A[2];
extern struct POINT2D unk_3BD6A[2];
extern legacy_s16 word_3BD72[4];
extern legacy_s16 audio_opponent_engine_channel;
extern legacy_s16 audio_player_engine_channel;
extern struct TRACKOBJECT trkObjectList[215];
extern struct VECTOR unk_3E640[];
extern struct VECTOR unk_3E646[];
extern struct VECTOR unk_3E676[];
extern struct VECTOR unk_3E682[];
extern struct VECTOR unk_3E68E[];
extern struct VECTOR unk_3E69A[];

extern void update_crash_state(legacy_s16, legacy_s16);
extern void build_track_object(struct VECTOR*, struct VECTOR*);
extern void audio_unk3(legacy_u8, legacy_s16);

#include "physics_internal.h"

enum PLAYER_PHYSICS_FLOW {
	PLAYER_FLOW_loc_15142,
	PLAYER_FLOW_loc_15163,
	PLAYER_FLOW_loc_15192,
	PLAYER_FLOW_loc_151A2,
	PLAYER_FLOW_loc_151BA,
	PLAYER_FLOW_loc_151DB,
	PLAYER_FLOW_loc_15240,
	PLAYER_FLOW_loc_15257,
	PLAYER_FLOW_loc_15264,
	PLAYER_FLOW_loc_15270,
	PLAYER_FLOW_loc_1527C,
	PLAYER_FLOW_loc_15338,
	PLAYER_FLOW_loc_15347,
	PLAYER_FLOW_loc_1537C,
	PLAYER_FLOW_loc_15381,
	PLAYER_FLOW_loc_15398,
	PLAYER_FLOW_loc_153AE,
	PLAYER_FLOW_loc_1540C,
	PLAYER_FLOW_loc_1543A,
	PLAYER_FLOW_loc_1544A,
	PLAYER_FLOW_loc_1545D,
	PLAYER_FLOW_loc_1546E,
	PLAYER_FLOW_loc_154CA,
	PLAYER_FLOW_loc_154F8,
	PLAYER_FLOW_loc_154FA,
	PLAYER_FLOW_loc_15513,
	PLAYER_FLOW_loc_15530,
	PLAYER_FLOW_loc_1553F,
	PLAYER_FLOW_loc_15599,
	PLAYER_FLOW_loc_155A1,
	PLAYER_FLOW_loc_15642,
	PLAYER_FLOW_loc_156A3,
	PLAYER_FLOW_loc_156D6,
	PLAYER_FLOW_loc_156ED,
	PLAYER_FLOW_loc_15703,
	PLAYER_FLOW_loc_1570A,
	PLAYER_FLOW_loc_15879,
	PLAYER_FLOW_loc_15882,
	PLAYER_FLOW_loc_158DA,
	PLAYER_FLOW_loc_15950,
	PLAYER_FLOW_loc_1595A,
	PLAYER_FLOW_loc_15964,
	PLAYER_FLOW_loc_1596E,
	PLAYER_FLOW_loc_1599E,
	PLAYER_FLOW_loc_159AD,
	PLAYER_FLOW_loc_15A30,
	PLAYER_FLOW_loc_15C04,
	PLAYER_FLOW_loc_15C75,
	PLAYER_FLOW_loc_15CDF,
	PLAYER_FLOW_loc_15CE8,
	PLAYER_FLOW_loc_15CF7,
	PLAYER_FLOW_loc_15D1A,
	PLAYER_FLOW_loc_15D2B,
	PLAYER_FLOW_loc_15D39,
	PLAYER_FLOW_loc_15D43,
	PLAYER_FLOW_loc_15D94,
	PLAYER_FLOW_loc_15DB6,
	PLAYER_FLOW_loc_15DC8,
	PLAYER_FLOW_loc_15DD1,
	PLAYER_FLOW_loc_15DDB,
	PLAYER_FLOW_loc_15E85,
	PLAYER_FLOW_code_update_globalPos,
	PLAYER_FLOW_code_update_rotCoords,
	PLAYER_FLOW_loc_15FDE,
	PLAYER_FLOW_loc_15FEF,
	PLAYER_FLOW_loc_15FFE,
	PLAYER_FLOW_loc_1600F,
	PLAYER_FLOW_loc_1601B,
	PLAYER_FLOW_loc_1602C,
	PLAYER_FLOW_loc_1603A,
	PLAYER_FLOW_loc_1604B,
	PLAYER_FLOW_loc_16057,
	PLAYER_FLOW_loc_160A7,
	PLAYER_FLOW_loc_1611C,
	PLAYER_FLOW_loc_1613E,
	PLAYER_FLOW_loc_16141,
	PLAYER_FLOW_loc_16146,
	PLAYER_FLOW_loc_1614C,
	PLAYER_FLOW_loc_16169,
	PLAYER_FLOW_loc_161AB,
	PLAYER_FLOW_loc_161DE,
	PLAYER_FLOW_loc_161FC,
	PLAYER_FLOW_loc_161FF,
	PLAYER_FLOW_loc_16204,
	PLAYER_FLOW_loc_1620A,
	PLAYER_FLOW_loc_16236,
	PLAYER_FLOW_loc_1624A,
	PLAYER_FLOW_loc_1624E,
	PLAYER_FLOW_loc_1625F,
	PLAYER_FLOW_loc_16288,
	PLAYER_FLOW_loc_162EE,
	PLAYER_FLOW_loc_162F9,
	PLAYER_FLOW_loc_16309,
	PLAYER_FLOW_loc_1632C,
	PLAYER_FLOW_loc_16336,
	PLAYER_FLOW_loc_1641E,
	PLAYER_FLOW_loc_16425,
	PLAYER_FLOW_loc_16428,
	PLAYER_FLOW_loc_1644C,
	PLAYER_FLOW_loc_164B2,
	PLAYER_FLOW_loc_1653E,
	PLAYER_FLOW_loc_16550,
	PLAYER_FLOW_loc_16566,
	PLAYER_FLOW_loc_16578,
	PLAYER_FLOW_loc_165AF,
	PLAYER_FLOW_loc_165B9,
	PLAYER_FLOW_loc_165C0,
	PLAYER_FLOW_loc_165C8,
	PLAYER_FLOW_loc_165EA,
	PLAYER_FLOW_loc_165F0,
	PLAYER_FLOW_loc_16648,
	PLAYER_FLOW_loc_16650,
	PLAYER_FLOW_loc_16670,
	PLAYER_FLOW_loc_1667A,
	PLAYER_FLOW_loc_16710,
	PLAYER_FLOW_loc_1671F,
	PLAYER_FLOW_loc_1672C,
	PLAYER_FLOW_loc_16836,
	PLAYER_FLOW_loc_16840,
	PLAYER_FLOW_loc_16892,
};

void update_player_state(struct CARSTATE* arg_pState, struct SIMD* arg_pSimd, struct CARSTATE* arg_oState, struct SIMD* arg_oSimd, legacy_s16 arg_MplayerFlag) {
	struct MATRIX var_MmatFromAngleZ;
	legacy_s16 var_pSpeed2Scaled;
	struct VECTOR vec_FC;
	struct VECTOR vec_1C6;
	legacy_s16 var_140someWhlData[4];
	struct VECTORLONG* var_DEptrTo1C0;
	struct VECTORLONG* var_146ptrTo176;
	legacy_s16 pState_f40_sar2;
	legacy_s8 var_EC;
	legacy_s16 var_F0;
	struct VECTOR vec_E4;
	struct VECTORLONG vecl_1C0[4];
	struct VECTORLONG vecl_176[4];
	legacy_s16 var_wheelIndex;
	legacy_s8 var_2;
	struct VECTOR vec_182, vec_1E4, vec_C, vec_1C, vec_17C, var_122;
	struct VECTOR var_11ApStateWorldCrds[2], vec_18EoStateWorldCrds[2];
	struct MATRIX mat_134;
	legacy_s8 var_136;
	legacy_s16 var_F4, var_F2, var_EE, var_138;
	legacy_u16 var_190;
	struct MATRIX* var_EA;
	legacy_s16 si;
	legacy_s16 var_16[4];
	struct PLANE far* var_6;
	struct VECTOR vec_1DE[4];
	legacy_s16 var_E;
	legacy_s8 var_11C;
	struct VECTOR var_DC[32];
	enum PLAYER_PHYSICS_FLOW physics_flow;

	//return ported_update_player_state_(arg_pState, arg_pSimd, arg_oState, arg_oSimd, arg_MplayerFlag);

	/*
	 * Seed the four collision-plane results from the explicit model of the
	 * original overlapping stack window. Each successful wheel lookup below
	 * replaces its corresponding entry.
	 */
	var_16[0] = legacy_execution_residue.grip_stack_words[0];
	var_16[1] = legacy_execution_residue.grip_stack_words[1];
	var_16[2] = legacy_execution_residue.grip_stack_words[2];
	var_16[3] = legacy_execution_residue.grip_stack_words[3];

	/* Initialize the working position and rotation from the current car pose. */
	pState_lvec1_x = arg_pState->car_posWorld1.lx;
	pState_lvec1_y = arg_pState->car_posWorld1.ly;
	pState_lvec1_z = arg_pState->car_posWorld1.lz;
	arg_pState->car_posWorld2.lx = arg_pState->car_posWorld1.lx;
	arg_pState->car_posWorld2.ly = arg_pState->car_posWorld1.ly;
	arg_pState->car_posWorld2.lz = arg_pState->car_posWorld1.lz;
	pState_minusRotate_z_1 = arg_pState->car_rotate.z;
	pState_minusRotate_z_2 = arg_pState->car_rotate.z;
	pState_minusRotate_x_1 = arg_pState->car_rotate.y;
	pState_minusRotate_x_2 = arg_pState->car_rotate.y;
	pState_minusRotate_y_1 = arg_pState->car_rotate.x;
	pState_minusRotate_y_2 = arg_pState->car_rotate.x;

	/*
	 * While the car has surface contact, offset the first two wheel-plane
	 * angles by one quarter of the front-wheel angle.
	 */
	if (arg_pState->car_sumSurfAllWheels != 0) {
		pState_f40_sar2 = LEGACY_S16_SAR2(
			arg_pState->car_40MfrontWhlAngle);
	} else {
		pState_f40_sar2 = 0;
	}

	/* Convert speed to per-tick travel, accounting for the simulation rate. */
	if (framespersec == 0xA) {
		var_pSpeed2Scaled = scale_speed_to_travel(
			arg_pState->car_speed2, 0x1E00U);
	} else {
		var_pSpeed2Scaled = scale_speed_to_travel(
			arg_pState->car_speed2, 0x3C00U);
	}

	/*
	 * At zero per-tick travel the original skips initialization of its four
	 * wheel-angle locals. Restore the retained words from the stack window
	 * appropriate to the opponent or player invocation.
	 */
	if (var_pSpeed2Scaled == 0) {
		if (arg_MplayerFlag != 0) {
			var_140someWhlData[0] =
				legacy_execution_residue.wheel_angle_stack_words[0];
			var_140someWhlData[1] =
				legacy_execution_residue.wheel_angle_stack_words[1];
			var_140someWhlData[2] =
				legacy_execution_residue.wheel_angle_stack_words[2];
			var_140someWhlData[3] =
				legacy_execution_residue.wheel_angle_stack_words[3];
		} else {
			var_140someWhlData[0] =
				legacy_execution_residue.wheel_plane_angles[0];
			var_140someWhlData[1] =
				legacy_execution_residue.wheel_plane_angles[1];
			var_140someWhlData[2] =
				legacy_execution_residue.wheel_plane_angles[2];
			var_140someWhlData[3] =
				legacy_execution_residue.wheel_plane_angles[3];
		}
	}

	/*
	 * On the first stopped frame of a player crash, the retained words come
	 * from rear-opponent wheel coordinates rather than ordinary wheel angles.
	 * Reconstruct those words from explicit game state.
	 */
	if (arg_MplayerFlag == 0 && var_pSpeed2Scaled == 0 &&
		arg_pState->car_lastspeed != 0 && arg_pState->car_crashBmpFlag != 0) {
		/*
		 * On the zero-speed crash transition, the original wheel-angle locals
		 * reuse opponent wheel-coordinate words left at the same stack addresses.
		 */
		mat_unk = *mat_rot_zxy(
			LEGACY_S16_WRAP_NEGATE(state.opponentstate.car_rotate.z),
			LEGACY_S16_WRAP_NEGATE(state.opponentstate.car_rotate.y),
			LEGACY_S16_WRAP_NEGATE(state.opponentstate.car_rotate.x),
			0
		);
		/*
		if (opponent has wheel contact &&
			opponent speed <= 30 mph &&
			opponent is upside down)
		{
			wheel_y_adjustment = 192;
		}
		*/
		var_F0 = 0;
		if (state.opponentstate.car_sumSurfAllWheels != 0 &&
			state.opponentstate.car_speed2 <= 0x1E00) {
			vec_1C6.x = 0;
			vec_1C6.y = 0x7530;
			vec_1C6.z = 0;
			mat_mul_vector(&vec_1C6, &mat_unk, &vec_FC);
			if (vec_FC.y < 0) {
				var_F0 = 0xC0;
			}
		}

		/* Prepare the optional auxiliary wheel rotation once for both wheels. */
		if ((state.opponentstate.car_angle_z & 0x3FF) != 0) {
			var_MmatFromAngleZ = *mat_rot_zxy(0, 0,
				LEGACY_S16_WRAP_NEGATE(
					state.opponentstate.car_angle_z), 0);
		}

		/*
		 * Rebuild wheel 2 in car-local coordinates, including suspension travel
		 * and the low-speed inverted-car adjustment.
		 */
		vec_1C6 = simd_opponent.wheel_coords[2];
		vec_1C6.y = LEGACY_S16_WRAP_ADD(LEGACY_S16_WRAP_NEGATE(
			LEGACY_S16_WRAP_ADD(
				state.opponentstate.car_rc2[2], 0x180)), var_F0);
		if ((state.opponentstate.car_angle_z & 0x3FF) != 0) {
			mat_mul_vector(&vec_1C6, &var_MmatFromAngleZ, &vec_FC);
			vec_1C6 = vec_FC;
		}

		/*
		 * Rotate wheel 2 into world axes and recover the high word of its world
		 * Z coordinate, the first aliased stack word.
		 */
		mat_mul_vector(&vec_1C6, &mat_unk, &vec_FC);
		var_140someWhlData[0] = (legacy_u16)(
			(legacy_u32)LEGACY_S32_WRAP_ADD_S16(
				state.opponentstate.car_posWorld1.lz, vec_FC.z) >> 16
		);
		legacy_execution_residue.wheel_plane_angles[0] =
			var_140someWhlData[0];

		/* Rebuild wheel 3 using the same local-coordinate adjustments. */
		vec_1C6 = simd_opponent.wheel_coords[3];
		vec_1C6.y = LEGACY_S16_WRAP_ADD(LEGACY_S16_WRAP_NEGATE(
			LEGACY_S16_WRAP_ADD(
				state.opponentstate.car_rc2[3], 0x180)), var_F0);
		if ((state.opponentstate.car_angle_z & 0x3FF) != 0) {
			mat_mul_vector(&vec_1C6, &var_MmatFromAngleZ, &vec_FC);
			vec_1C6 = vec_FC;
		}

		/*
		 * Rotate wheel 3 into world axes and recover the low word of its world
		 * X coordinate, the second aliased stack word.
		 */
		mat_mul_vector(&vec_1C6, &mat_unk, &vec_FC);
		var_140someWhlData[1] = (legacy_u16)LEGACY_S32_WRAP_ADD_S16(
			state.opponentstate.car_posWorld1.lx, vec_FC.x);

		/* Commit the second word and recover world X's high word as the third. */
		legacy_execution_residue.wheel_plane_angles[1] =
			var_140someWhlData[1];
		var_140someWhlData[2] = (legacy_u16)(
			(legacy_u32)LEGACY_S32_WRAP_ADD_S16(
				state.opponentstate.car_posWorld1.lx, vec_FC.x) >> 16
		);

		/* Commit the third word and recover world Y's low word as the fourth. */
		legacy_execution_residue.wheel_plane_angles[2] =
			var_140someWhlData[2];
		var_140someWhlData[3] = (legacy_u16)LEGACY_S32_WRAP_ADD_S16(
			state.opponentstate.car_posWorld1.ly, vec_FC.y);
		legacy_execution_residue.wheel_plane_angles[3] =
			var_140someWhlData[3];
	}

	mat_unk = *mat_rot_zxy(
		LEGACY_S16_WRAP_NEGATE(pState_minusRotate_z_1),
		LEGACY_S16_WRAP_NEGATE(pState_minusRotate_x_1),
		LEGACY_S16_WRAP_NEGATE(pState_minusRotate_y_1), 0);
	if (pState_minusRotate_x_1 != 0 || pState_minusRotate_z_1 != 0) {
		vec_1C6.x = 0;
		vec_1C6.y = 0;
		vec_1C6.z = 0x82;
		mat_mul_vector(&vec_1C6, &mat_unk, &vec_FC);
		arg_pState->car_pseudoGravity = LEGACY_S16_WRAP_NEGATE(vec_FC.y);
	} else {
		arg_pState->car_pseudoGravity = 0;
	}

	if ((arg_pState->car_angle_z & 0x3FF) != 0) {
		var_EC = 1;
		var_MmatFromAngleZ = *mat_rot_zxy(0, 0,
			LEGACY_S16_WRAP_NEGATE(arg_pState->car_angle_z), 0);
	} else {
		var_EC = 0;
	}

	vec_1C6.x = 0;
	vec_1C6.y = 0x7530;
	vec_1C6.z = 0;
	mat_mul_vector(&vec_1C6, &mat_unk, &vec_FC);
	if (arg_pState->car_sumSurfAllWheels == 0 || vec_FC.y >= 0) {
		var_F0 = 0;
	} else if (arg_pState->car_speed2 <= 0x1E00) {
		var_F0 = -192;
	} else {
		var_F0 = 192;
		vec_1C6.y = -192;
		mat_mul_vector(&vec_1C6, &mat_unk, &vec_E4);
	}
	vec_unk2.x = 0;
	vec_unk2.y = 0;
	planindex_copy = -1;
	var_DEptrTo1C0 = vecl_1C0;
	var_146ptrTo176 = vecl_176;
	for (var_wheelIndex = 0; var_wheelIndex < 4; var_wheelIndex++) {
	vec_1C6 = arg_pSimd->wheel_coords[var_wheelIndex];
	vec_1C6.y = LEGACY_S16_WRAP_NEGATE(LEGACY_S16_WRAP_ADD(
		arg_pState->car_rc2[var_wheelIndex], 0x180));
	if (var_F0 < 0)
		vec_1C6.y = LEGACY_S16_WRAP_SUB(vec_1C6.y, var_F0);
	if (var_EC != 0) {
		mat_mul_vector(&vec_1C6, &var_MmatFromAngleZ, &vec_FC);
		vec_1C6 = vec_FC;
	}
	mat_mul_vector(&vec_1C6, &mat_unk, &vec_FC);
	var_DEptrTo1C0->lx = LEGACY_S32_WRAP_ADD_S16(
		pState_lvec1_x, vec_FC.x);
	var_DEptrTo1C0->ly = LEGACY_S32_WRAP_ADD_S16(
		pState_lvec1_y, vec_FC.y);
	var_DEptrTo1C0->lz = LEGACY_S32_WRAP_ADD_S16(
		pState_lvec1_z, vec_FC.z);

	var_146ptrTo176->lx = var_DEptrTo1C0->lx;
	var_146ptrTo176->ly = var_DEptrTo1C0->ly;
	var_146ptrTo176->lz = var_DEptrTo1C0->lz;
	if (var_pSpeed2Scaled != 0) {
		vec_unk2.z = var_pSpeed2Scaled;
		pState_f36Mminf40sar2 = arg_pState->car_36MwhlAngle;
		if (pState_f40_sar2 != 0 && var_wheelIndex < 2) {
			pState_f36Mminf40sar2 = LEGACY_S16_WRAP_SUB(
				arg_pState->car_36MwhlAngle, pState_f40_sar2);
		}
		var_140someWhlData[var_wheelIndex] = pState_f36Mminf40sar2;
		legacy_execution_residue.wheel_plane_angles[var_wheelIndex] =
			pState_f36Mminf40sar2;
		if (arg_MplayerFlag != 0) {
			legacy_execution_residue.wheel_angle_stack_words[
				var_wheelIndex] = pState_f36Mminf40sar2;
		}
		plane_rotate_op();
		var_DEptrTo1C0->lx = LEGACY_S32_WRAP_ADD_S16(
			var_DEptrTo1C0->lx, vec_planerotopresult.x);
		var_DEptrTo1C0->ly = LEGACY_S32_WRAP_ADD_S16(
			var_DEptrTo1C0->ly, vec_planerotopresult.y);
		var_DEptrTo1C0->lz = LEGACY_S32_WRAP_ADD_S16(
			var_DEptrTo1C0->lz, vec_planerotopresult.z);
	}
	var_DEptrTo1C0++;
	var_146ptrTo176++;
	}
	var_2 = 0;
	physics_flow = PLAYER_FLOW_loc_15142;
	for (;;) {
	switch (physics_flow) {
case PLAYER_FLOW_loc_15142:
	var_2 = LEGACY_S8_WRAP_ADD(var_2, 1);
	if (var_2 != 5)
		{ physics_flow = PLAYER_FLOW_loc_151A2; continue; }
	arg_pState->car_36MwhlAngle = 0x200;
	update_crash_state(1, arg_MplayerFlag);

case PLAYER_FLOW_loc_15163:
	if (arg_pState->car_surfaceWhl[0] != 5)
		{ physics_flow = PLAYER_FLOW_loc_15192; continue; }
	if (arg_pState->car_surfaceWhl[1] != 5)
		{ physics_flow = PLAYER_FLOW_loc_15192; continue; }
	if (arg_pState->car_surfaceWhl[2] != 5)
		{ physics_flow = PLAYER_FLOW_loc_15192; continue; }
	if (arg_pState->car_surfaceWhl[3] != 5)
		{ physics_flow = PLAYER_FLOW_loc_15192; continue; }
	update_crash_state(2, arg_MplayerFlag);

case PLAYER_FLOW_loc_15192:
	var_DEptrTo1C0 = vecl_1C0;
	var_wheelIndex = 0;
	{ physics_flow = PLAYER_FLOW_loc_15DD1; continue; }

case PLAYER_FLOW_loc_151A2:
	var_DEptrTo1C0 = vecl_1C0;
	var_146ptrTo176 = vecl_176;
	var_wheelIndex = 0;
	{ physics_flow = PLAYER_FLOW_loc_15D39; continue; }

case PLAYER_FLOW_loc_151BA:
	build_track_object(&vec_1C6, &arg_pState->car_whlWorldCrds1[var_wheelIndex]);

case PLAYER_FLOW_loc_151DB:
	arg_pState->car_surfaceWhl[var_wheelIndex] = current_surf_type;
	vec_1C6.x = physics_position_word(var_DEptrTo1C0->lx);
	vec_1C6.y = physics_position_word(var_DEptrTo1C0->ly);
	vec_1C6.z = physics_position_word(var_DEptrTo1C0->lz);

	if (state.game_inputmode != 2)
		{ physics_flow = PLAYER_FLOW_loc_15240; continue; }
	nextPosAndNormalIP = vec_1C6.y;
	{ physics_flow = PLAYER_FLOW_loc_15257; continue; }

case PLAYER_FLOW_loc_15240:
	nextPosAndNormalIP = plane_origin_op(planindex, vec_1C6.x, vec_1C6.y, vec_1C6.z);

case PLAYER_FLOW_loc_15257:
	if (wallindex != -1)
		{ physics_flow = PLAYER_FLOW_loc_15264; continue; }
	{ physics_flow = PLAYER_FLOW_loc_15950; continue; }

case PLAYER_FLOW_loc_15264:
	if (nextPosAndNormalIP > elRdWallRelated)
		{ physics_flow = PLAYER_FLOW_loc_15270; continue; }
	{ physics_flow = PLAYER_FLOW_loc_15950; continue; }

case PLAYER_FLOW_loc_15270:
	if (nextPosAndNormalIP < wallHeight)
		{ physics_flow = PLAYER_FLOW_loc_1527C; continue; }
	{ physics_flow = PLAYER_FLOW_loc_15950; continue; }

case PLAYER_FLOW_loc_1527C:
	vec_182.x = LEGACY_S16_WRAP_SUB(
		arg_pState->car_whlWorldCrds1[var_wheelIndex].x, wallStartX);
	vec_182.y = 0;
	vec_182.z = LEGACY_S16_WRAP_SUB(
		arg_pState->car_whlWorldCrds1[var_wheelIndex].z, wallStartZ);
	vec_1E4.x = LEGACY_S16_WRAP_SUB(
		physics_position_word(var_DEptrTo1C0->lx), wallStartX);
	vec_1E4.y = 0;
	vec_1E4.z = LEGACY_S16_WRAP_SUB(
		physics_position_word(var_DEptrTo1C0->lz), wallStartZ);

	mat_rot_y(&mat_134, LEGACY_S16_WRAP_SUB(
		LEGACY_S16_WRAP_NEGATE(wallOrientation), 0x100));
	if (arg_MplayerFlag == 0) {
		legacy_execution_residue.wheel_angle_stack_words[0] = mat_134.vals[4];
		legacy_execution_residue.wheel_angle_stack_words[1] = mat_134.vals[5];
		legacy_execution_residue.wheel_angle_stack_words[2] = mat_134.vals[6];
		legacy_execution_residue.wheel_angle_stack_words[3] = mat_134.vals[7];
	}
	mat_mul_vector(&vec_182, &mat_134, &vec_C);
	mat_mul_vector(&vec_1E4, &mat_134, &vec_1C);
	if (vec_1C.z <= 0)
		{ physics_flow = PLAYER_FLOW_loc_15338; continue; }
	if (vec_C.z <= 0)
		{ physics_flow = PLAYER_FLOW_loc_15338; continue; }
	{ physics_flow = PLAYER_FLOW_loc_15950; continue; }

case PLAYER_FLOW_loc_15338:
	if (vec_1C.z >= 0)
		{ physics_flow = PLAYER_FLOW_loc_15347; continue; }
	if (vec_C.z >= 0)
		{ physics_flow = PLAYER_FLOW_loc_15347; continue; }
	{ physics_flow = PLAYER_FLOW_loc_15950; continue; }

case PLAYER_FLOW_loc_15347:
	if (vec_1C.z <= vec_C.z)
		{ physics_flow = PLAYER_FLOW_loc_1537C; continue; }
	var_136 = 1;
	vec_FC = vec_1C;
	vec_1C = vec_C;
	vec_C = vec_FC;
	{ physics_flow = PLAYER_FLOW_loc_15381; continue; }

case PLAYER_FLOW_loc_1537C:
	var_136 = 0;
case PLAYER_FLOW_loc_15381:
	if (vec_1C.z != 0)
		{ physics_flow = PLAYER_FLOW_loc_15398; continue; }
	var_F4 = var_pSpeed2Scaled;
	var_F2 = 0;
	{ physics_flow = PLAYER_FLOW_loc_1540C; continue; }

case PLAYER_FLOW_loc_15398:
	if (vec_C.z != 0)
		{ physics_flow = PLAYER_FLOW_loc_153AE; continue; }
	var_F4 = 0;
	var_F2 = var_pSpeed2Scaled;
	{ physics_flow = PLAYER_FLOW_loc_1540C; continue; }

case PLAYER_FLOW_loc_153AE:
	vector_op_unk(&vec_1C, &vec_C, &vec_FC, 0);
	vec_17C.x = LEGACY_S16_SHL(
		LEGACY_S16_WRAP_SUB(vec_1C.x, vec_FC.x), 6U);
	vec_17C.y = LEGACY_S16_SHL(
		LEGACY_S16_WRAP_SUB(vec_1C.y, vec_FC.y), 6U);
	vec_17C.z = LEGACY_S16_SHL(
		LEGACY_S16_WRAP_SUB(vec_1C.z, vec_FC.z), 6U);
	var_F2 = polarRadius3D(&vec_17C);
	var_F4 = LEGACY_S16_WRAP_SUB(var_pSpeed2Scaled, var_F2);

case PLAYER_FLOW_loc_1540C:
	var_EE = LEGACY_S16_FROM_BITS((legacy_u16)LEGACY_S16_WRAP_SUB(
		LEGACY_S16_WRAP_NEGATE(pState_minusRotate_y_1),
		wallOrientation) & 0x3FFU);
	vec_FC.z = var_F2;
	vec_FC.y = 0;
	if (var_EE < 0x100)
		{ physics_flow = PLAYER_FLOW_loc_1543A; continue; }
	if (var_EE <= 0x300)
		{ physics_flow = PLAYER_FLOW_loc_1544A; continue; }

case PLAYER_FLOW_loc_1543A:
	var_EE = wallOrientation;
	vec_FC.x = 768;//0x300;
	{ physics_flow = PLAYER_FLOW_loc_1545D; continue; }

case PLAYER_FLOW_loc_1544A:
	var_EE = LEGACY_S16_FROM_BITS((legacy_u16)
		LEGACY_S16_WRAP_ADD(wallOrientation, 0x200) & 0x3FFU);
	vec_FC.x = -768;//0xFD00; // TODO: a negative number

case PLAYER_FLOW_loc_1545D:
	if (var_136 == 0)
		{ physics_flow = PLAYER_FLOW_loc_1546E; continue; }
	vec_FC.x = LEGACY_S16_WRAP_NEGATE(vec_FC.x);

case PLAYER_FLOW_loc_1546E:
	var_EA = mat_rot_zxy(
		LEGACY_S16_WRAP_NEGATE(pState_minusRotate_z_1),
		LEGACY_S16_WRAP_NEGATE(pState_minusRotate_x_1), var_EE, 0);
	mat_mul_vector(&vec_FC, var_EA, &vec_1C);
	si = LEGACY_S16_FROM_BITS((legacy_u16)LEGACY_S16_WRAP_SUB(
		LEGACY_S16_WRAP_NEGATE(pState_minusRotate_y_1), var_EE) &
		0x3FFU);
	var_138 = 0;
	if (si <= 0x100)
		{ physics_flow = PLAYER_FLOW_loc_154CA; continue; }
		si = LEGACY_S16_WRAP_SUB(0x400, si);
	var_138 = 1;

case PLAYER_FLOW_loc_154CA:
	var_190 = LEGACY_U16_SHL(
		(legacy_u8)LEGACY_S16_WRAP_NEGATE(
			LEGACY_S16_WRAP_SUB(LEGACY_S16_SAR(
				LEGACY_S16_WRAP_MUL(si, 0x46), 8U), 0x64)), 8U);
	if (arg_pState->car_speed2 <= var_190)
		{ physics_flow = PLAYER_FLOW_loc_15513; continue; }
	if (var_138 == 0)
		{ physics_flow = PLAYER_FLOW_loc_154F8; continue; }
	var_138 = LEGACY_S16_SHL(LEGACY_S16_WRAP_NEGATE(si), 1U);
	{ physics_flow = PLAYER_FLOW_loc_154FA; continue; }

case PLAYER_FLOW_loc_154F8:
	var_138 = LEGACY_S16_SHL(si, 1U);
case PLAYER_FLOW_loc_154FA:
	arg_pState->car_36MwhlAngle = var_138;
	update_crash_state(1, arg_MplayerFlag);

case PLAYER_FLOW_loc_15513:
	arg_pState->field_CF |= 0x10;
	var_DEptrTo1C0 = vecl_1C0;
	var_146ptrTo176 = vecl_176;
	si = 0;
	{ physics_flow = PLAYER_FLOW_loc_15599; continue; }

case PLAYER_FLOW_loc_15530:
	vec_C.x = 0;
	vec_C.y = 0;
	vec_C.z = 0;

case PLAYER_FLOW_loc_1553F:
	var_DEptrTo1C0->lx = LEGACY_S32_WRAP_ADD_S16(
		LEGACY_S32_WRAP_ADD_S16(var_146ptrTo176->lx, vec_C.x),
		vec_1C.x);
	var_DEptrTo1C0->ly = LEGACY_S32_WRAP_ADD_S16(
		LEGACY_S32_WRAP_ADD_S16(var_146ptrTo176->ly, vec_C.y),
		vec_1C.y);
	var_DEptrTo1C0->lz = LEGACY_S32_WRAP_ADD_S16(
		LEGACY_S32_WRAP_ADD_S16(var_146ptrTo176->lz, vec_C.z),
		vec_1C.z);
	var_DEptrTo1C0++;
	var_146ptrTo176++;
	si++;

case PLAYER_FLOW_loc_15599:
	if (si < 4)
		{ physics_flow = PLAYER_FLOW_loc_155A1; continue; }
	{ physics_flow = PLAYER_FLOW_loc_15142; continue; }

case PLAYER_FLOW_loc_155A1:
	if (var_F4 == 0)
		{ physics_flow = PLAYER_FLOW_loc_15530; continue; }
	vec_C.x = scale_position_delta(var_DEptrTo1C0->lx,
		var_146ptrTo176->lx, var_F4, var_pSpeed2Scaled);
	vec_C.y = scale_position_delta(var_DEptrTo1C0->ly,
		var_146ptrTo176->ly, var_F4, var_pSpeed2Scaled);
	vec_C.z = scale_position_delta(var_DEptrTo1C0->lz,
		var_146ptrTo176->lz, var_F4, var_pSpeed2Scaled);
	{ physics_flow = PLAYER_FLOW_loc_1553F; continue; }

case PLAYER_FLOW_loc_15642:
	arg_pState->car_rc1[var_wheelIndex] = LEGACY_S16_WRAP_ADD(
		arg_pState->car_rc1[var_wheelIndex],
		word_3BD72[var_wheelIndex]);
	var_DEptrTo1C0->ly = LEGACY_S32_WRAP_SUB_S16(
		var_DEptrTo1C0->ly, arg_pState->car_rc1[var_wheelIndex]);
	if (framespersec != 0xA)
		{ physics_flow = PLAYER_FLOW_loc_156A3; continue; }
	arg_pState->car_rc1[var_wheelIndex] = LEGACY_S16_WRAP_ADD(
		arg_pState->car_rc1[var_wheelIndex],
		word_3BD72[var_wheelIndex]);
	var_DEptrTo1C0->ly = LEGACY_S32_WRAP_SUB_S16(
		var_DEptrTo1C0->ly, arg_pState->car_rc1[var_wheelIndex]);

case PLAYER_FLOW_loc_156A3:
	vec_1C6.y = physics_position_word(var_DEptrTo1C0->ly);
	if (state.game_inputmode == 2) {
		nextPosAndNormalIP = vec_1C6.y;
	} else {
		nextPosAndNormalIP = plane_origin_op(planindex, vec_1C6.x, vec_1C6.y, vec_1C6.z);
	}

case PLAYER_FLOW_loc_156D6:
	if (nextPosAndNormalIP <= 0xC)
		{ physics_flow = PLAYER_FLOW_loc_156ED; continue; }
	arg_pState->car_surfaceWhl[var_wheelIndex] = 0;

case PLAYER_FLOW_loc_156ED:
	var_16[var_wheelIndex] = nextPosAndNormalIP;
	if (nextPosAndNormalIP != 0)
		{ physics_flow = PLAYER_FLOW_loc_15703; continue; }
	{ physics_flow = PLAYER_FLOW_loc_15CE8; continue; }

case PLAYER_FLOW_loc_15703:
	if (nextPosAndNormalIP < 0)
		{ physics_flow = PLAYER_FLOW_loc_1570A; continue; }
	{ physics_flow = PLAYER_FLOW_loc_15D2B; continue; }

case PLAYER_FLOW_loc_1570A:
	var_6 = &planptr[planindex];
	var_122.x = LEGACY_S16_WRAP_ADD(
		var_6->plane_origin.x, elem_xCenter);
	var_122.y = LEGACY_S16_WRAP_ADD(
		var_6->plane_origin.y, terrainHeight);
	var_122.z = LEGACY_S16_WRAP_ADD(
		var_6->plane_origin.z, elem_zCenter);

	vec_182.x = LEGACY_S16_WRAP_SUB(
		physics_position_word(var_146ptrTo176->lx), var_122.x);
	vec_182.y = LEGACY_S16_WRAP_SUB(
		physics_position_word(var_146ptrTo176->ly), var_122.y);
	vec_182.z = LEGACY_S16_WRAP_SUB(
		physics_position_word(var_146ptrTo176->lz), var_122.z);

	vec_1E4.x = LEGACY_S16_WRAP_SUB(
		physics_position_word(var_DEptrTo1C0->lx), var_122.x);
	vec_1E4.y = LEGACY_S16_WRAP_SUB(
		physics_position_word(var_DEptrTo1C0->ly), var_122.y);
	vec_1E4.z = LEGACY_S16_WRAP_SUB(
		physics_position_word(var_DEptrTo1C0->lz), var_122.z);

	mat_134 = var_6->plane_rotation;
	if (arg_MplayerFlag == 0) {
		legacy_execution_residue.wheel_angle_stack_words[0] = mat_134.vals[4];
		legacy_execution_residue.wheel_angle_stack_words[1] = mat_134.vals[5];
		legacy_execution_residue.wheel_angle_stack_words[2] = mat_134.vals[6];
		legacy_execution_residue.wheel_angle_stack_words[3] = mat_134.vals[7];
	}
	mat_invert(&mat_134, &var_MmatFromAngleZ);
	mat_mul_vector(&vec_182, &var_MmatFromAngleZ, &vec_C);

	mat_mul_vector(&vec_1E4, &var_MmatFromAngleZ, &vec_1C);
	var_136 = 0;
	if (byte_4392C != 0)
		{ physics_flow = PLAYER_FLOW_loc_15879; continue; }
	if (vec_C.y >= -12)//0xFFF4)
		{ physics_flow = PLAYER_FLOW_loc_15879; continue; }
	if (vec_1C.y >= -12)//0xFFF4)
		{ physics_flow = PLAYER_FLOW_loc_15879; continue; }
	if (vec_1C.y <= -24)//0xFFE8)
		{ physics_flow = PLAYER_FLOW_loc_158DA; continue; }
	update_crash_state(5, arg_MplayerFlag);
	var_136 = 1;

case PLAYER_FLOW_loc_15879:
	if (vec_1C.y == 0)
		{ physics_flow = PLAYER_FLOW_loc_15882; continue; }
	{ physics_flow = PLAYER_FLOW_loc_1599E; continue; }

case PLAYER_FLOW_loc_15882:
	vec_unk2.x = 0;
	vec_unk2.y = 0;
	vec_unk2.z = 0x40;
	planindex_copy = planindex;
	pState_f36Mminf40sar2 = var_140someWhlData[var_wheelIndex];
	plane_rotate_op();
	var_DEptrTo1C0->lx = LEGACY_S32_WRAP_SUB_S16(
		var_DEptrTo1C0->lx, vec_planerotopresult.x);
	var_DEptrTo1C0->ly = LEGACY_S32_WRAP_SUB_S16(
		var_DEptrTo1C0->ly, vec_planerotopresult.y);
	var_DEptrTo1C0->lz = LEGACY_S32_WRAP_SUB_S16(
		var_DEptrTo1C0->lz, vec_planerotopresult.z);
	{ physics_flow = PLAYER_FLOW_loc_15CDF; continue; }

case PLAYER_FLOW_loc_158DA:
	planindex = 0;
	current_planptr = planptr;
	byte_4392C = 1;
	vec_1C6.x = physics_position_word(var_DEptrTo1C0->lx);
	vec_1C6.y = physics_position_word(var_DEptrTo1C0->ly);
	vec_1C6.z = physics_position_word(var_DEptrTo1C0->lz);

	nextPosAndNormalIP = plane_origin_op(0, vec_1C6.x, vec_1C6.y, vec_1C6.z);

case PLAYER_FLOW_loc_15950:
	if (nextPosAndNormalIP > 0)
		{ physics_flow = PLAYER_FLOW_loc_1595A; continue; }
	{ physics_flow = PLAYER_FLOW_loc_156ED; continue; }

case PLAYER_FLOW_loc_1595A:
	if (var_F0 > 0)
		{ physics_flow = PLAYER_FLOW_loc_15964; continue; }
	{ physics_flow = PLAYER_FLOW_loc_15642; continue; }

case PLAYER_FLOW_loc_15964:
	if (nextPosAndNormalIP < 0x18)
		{ physics_flow = PLAYER_FLOW_loc_1596E; continue; }
	{ physics_flow = PLAYER_FLOW_loc_15642; continue; }

case PLAYER_FLOW_loc_1596E:
	var_DEptrTo1C0->lx = LEGACY_S32_WRAP_ADD_S16(
		var_DEptrTo1C0->lx, vec_E4.x);
	var_DEptrTo1C0->ly = LEGACY_S32_WRAP_ADD_S16(
		var_DEptrTo1C0->ly, vec_E4.y);
	var_DEptrTo1C0->lz = LEGACY_S32_WRAP_ADD_S16(
		var_DEptrTo1C0->lz, vec_E4.z);
	{ physics_flow = PLAYER_FLOW_loc_156ED; continue; }

case PLAYER_FLOW_loc_1599E:
	if (vec_C.y <= 0)
		{ physics_flow = PLAYER_FLOW_loc_159AD; continue; }
	if (vec_1C.y >= 0)
		{ physics_flow = PLAYER_FLOW_loc_159AD; continue; }
	{ physics_flow = PLAYER_FLOW_loc_15A30; continue; }

case PLAYER_FLOW_loc_159AD:
	vec_unk2.x = 0;
	vec_unk2.y = 0;
	vec_unk2.z = var_pSpeed2Scaled;
	planindex_copy = planindex;
	pState_f36Mminf40sar2 = var_140someWhlData[var_wheelIndex];
	plane_rotate_op();
	var_DEptrTo1C0->lx = LEGACY_S32_WRAP_ADD_S16(
		var_146ptrTo176->lx, vec_planerotopresult.x);
	var_DEptrTo1C0->ly = LEGACY_S32_WRAP_ADD_S16(
		var_146ptrTo176->ly, vec_planerotopresult.y);
	var_DEptrTo1C0->lz = LEGACY_S32_WRAP_ADD_S16(
		var_146ptrTo176->lz, vec_planerotopresult.z);
	{ physics_flow = PLAYER_FLOW_loc_15C04; continue; }

case PLAYER_FLOW_loc_15A30:
	var_EE = vec_C.z;
	vec_C.z = LEGACY_S16_WRAP_NEGATE(vec_C.y);
	vec_C.y = var_EE;

	var_EE = vec_1C.z;
	vec_1C.z = LEGACY_S16_WRAP_NEGATE(vec_1C.y);
	vec_1C.y = var_EE;
	vector_op_unk(&vec_1C, &vec_C, &vec_FC, 0);
	vec_17C.x = LEGACY_S16_SHL(
		LEGACY_S16_WRAP_SUB(vec_1C.x, vec_FC.x), 6U);
	vec_17C.y = LEGACY_S16_SHL(
		LEGACY_S16_WRAP_SUB(vec_1C.y, vec_FC.y), 6U);
	vec_17C.z = LEGACY_S16_SHL(
		LEGACY_S16_WRAP_SUB(vec_1C.z, vec_FC.z), 6U);

	var_EE = polarRadius3D(&vec_17C);
	var_F4 = LEGACY_S16_WRAP_ADD(
		arg_pState->car_rc1[var_wheelIndex], var_pSpeed2Scaled);
	var_F2 = LEGACY_S16_WRAP_SUB(var_F4, var_EE);
	vec_C.x = scale_position_delta(var_DEptrTo1C0->lx,
		var_146ptrTo176->lx, var_F2, var_F4);
	vec_C.y = scale_position_delta(var_DEptrTo1C0->ly,
		var_146ptrTo176->ly, var_F2, var_F4);
	vec_C.z = scale_position_delta(var_DEptrTo1C0->lz,
		var_146ptrTo176->lz, var_F2, var_F4);

	vec_unk2.x = 0;
	vec_unk2.y = 0;
	vec_unk2.z = var_EE;
	planindex_copy = planindex;
	pState_f36Mminf40sar2 = var_140someWhlData[var_wheelIndex];
	plane_rotate_op();
	var_DEptrTo1C0->lx = LEGACY_S32_WRAP_ADD_S16(
		LEGACY_S32_WRAP_ADD_S16(
			var_146ptrTo176->lx, vec_C.x),
		vec_planerotopresult.x);
	var_DEptrTo1C0->ly = LEGACY_S32_WRAP_ADD_S16(
		LEGACY_S32_WRAP_ADD_S16(
			var_146ptrTo176->ly, vec_C.y),
		vec_planerotopresult.y);
	var_DEptrTo1C0->lz = LEGACY_S32_WRAP_ADD_S16(
		LEGACY_S32_WRAP_ADD_S16(
			var_146ptrTo176->lz, vec_C.z),
		vec_planerotopresult.z);

case PLAYER_FLOW_loc_15C04:
	vec_1C6.x = physics_position_word(var_DEptrTo1C0->lx);
	vec_1C6.y = physics_position_word(var_DEptrTo1C0->ly);
	vec_1C6.z = physics_position_word(var_DEptrTo1C0->lz);

	nextPosAndNormalIP = plane_origin_op(planindex, vec_1C6.x, vec_1C6.y, vec_1C6.z);
	if (nextPosAndNormalIP >= 0)
		{ physics_flow = PLAYER_FLOW_loc_15CDF; continue; }
	if (var_136 == 0)
		{ physics_flow = PLAYER_FLOW_loc_15C75; continue; }
	nextPosAndNormalIP = LEGACY_S16_WRAP_ADD(
		LEGACY_S16_WRAP_NEGATE(nextPosAndNormalIP), 6);

case PLAYER_FLOW_loc_15C75:
	vec_1C6.z = 0;
	vec_1C6.x = 0;
	vec_1C6.y = LEGACY_S16_SHL(
		LEGACY_S16_WRAP_NEGATE(nextPosAndNormalIP), 6U);
	mat_mul_vector2(&vec_1C6, &planptr[planindex].plane_rotation, &vec_FC);

	var_DEptrTo1C0->lx = LEGACY_S32_WRAP_ADD_S16(
		var_DEptrTo1C0->lx, vec_FC.x);
	var_DEptrTo1C0->ly = LEGACY_S32_WRAP_ADD_S16(
		var_DEptrTo1C0->ly, vec_FC.y);
	var_DEptrTo1C0->lz = LEGACY_S32_WRAP_ADD_S16(
		var_DEptrTo1C0->lz, vec_FC.z);

case PLAYER_FLOW_loc_15CDF:
case PLAYER_FLOW_loc_15CE8:
	if (arg_pState->car_rc1[var_wheelIndex] <= 0xFA)
		{ physics_flow = PLAYER_FLOW_loc_15CF7; continue; }
	arg_pState->field_CF |= 0x20;


case PLAYER_FLOW_loc_15CF7:
	if (arg_pState->car_rc1[var_wheelIndex] <= 0x5AEB)
		{ physics_flow = PLAYER_FLOW_loc_15D1A; continue; }
	update_crash_state(1, arg_MplayerFlag);

case PLAYER_FLOW_loc_15D1A:
	arg_pState->car_rc1[var_wheelIndex] = 0;

case PLAYER_FLOW_loc_15D2B:
	var_DEptrTo1C0++;
	var_146ptrTo176++;
	var_wheelIndex++;

case PLAYER_FLOW_loc_15D39:
	if (var_wheelIndex < 4)
		{ physics_flow = PLAYER_FLOW_loc_15D43; continue; }
	{ physics_flow = PLAYER_FLOW_loc_15163; continue; }

case PLAYER_FLOW_loc_15D43:
	vec_1C6.x = physics_position_word(var_DEptrTo1C0->lx);
	vec_1C6.y = physics_position_word(var_DEptrTo1C0->ly);
	vec_1C6.z = physics_position_word(var_DEptrTo1C0->lz);

	if (state.game_inputmode == 2)
		{ physics_flow = PLAYER_FLOW_loc_15D94; continue; }
	{ physics_flow = PLAYER_FLOW_loc_151BA; continue; }

case PLAYER_FLOW_loc_15D94:
	wallindex = -1;
	current_surf_type = 1; //tarmac;
	planindex = 0;
	current_planptr = planptr;
	{ physics_flow = PLAYER_FLOW_loc_151DB; continue; }

case PLAYER_FLOW_loc_15DB6:
	var_DEptrTo1C0->ly = LEGACY_S32_WRAP_ADD_S16(
		var_DEptrTo1C0->ly, LEGACY_S16_WRAP_ADD(var_EE, 0x180));

case PLAYER_FLOW_loc_15DC8:
	var_DEptrTo1C0++;
	var_wheelIndex++;

case PLAYER_FLOW_loc_15DD1:
	if (var_wheelIndex < 4)
		{ physics_flow = PLAYER_FLOW_loc_15DDB; continue; }
	{ physics_flow = PLAYER_FLOW_code_update_globalPos; continue; }

case PLAYER_FLOW_loc_15DDB:
	arg_pState->car_whlWorldCrds1[var_wheelIndex].x =
		physics_position_word(var_DEptrTo1C0->lx);
	arg_pState->car_whlWorldCrds1[var_wheelIndex].y =
		physics_position_word(var_DEptrTo1C0->ly);
	arg_pState->car_whlWorldCrds1[var_wheelIndex].z =
		physics_position_word(var_DEptrTo1C0->lz);


	var_EE = carState_rc_op(arg_pState, var_16[var_wheelIndex], var_wheelIndex);
	if (pState_minusRotate_z_1 != 0)
		{ physics_flow = PLAYER_FLOW_loc_15E85; continue; }
	if (pState_minusRotate_x_1 != 0)
		{ physics_flow = PLAYER_FLOW_loc_15E85; continue; }
	{ physics_flow = PLAYER_FLOW_loc_15DB6; continue; }

case PLAYER_FLOW_loc_15E85:
	vec_1C6.z = 0;
	vec_1C6.x = 0;
	vec_1C6.y = LEGACY_S16_WRAP_ADD(var_EE, 0x180);
	mat_mul_vector(&vec_1C6, &mat_unk, &vec_182);
	var_DEptrTo1C0->lx = LEGACY_S32_WRAP_ADD_S16(
		var_DEptrTo1C0->lx, vec_182.x);
	var_DEptrTo1C0->ly = LEGACY_S32_WRAP_ADD_S16(
		var_DEptrTo1C0->ly, vec_182.y);
	var_DEptrTo1C0->lz = LEGACY_S32_WRAP_ADD_S16(
		var_DEptrTo1C0->lz, vec_182.z);
	{ physics_flow = PLAYER_FLOW_loc_15DC8; continue; }

case PLAYER_FLOW_code_update_globalPos:
	pState_lvec1_x = LEGACY_S32_SAR(LEGACY_S32_WRAP_ADD(
		LEGACY_S32_WRAP_ADD(vecl_1C0[0].lx, vecl_1C0[1].lx),
		LEGACY_S32_WRAP_ADD(vecl_1C0[2].lx, vecl_1C0[3].lx)), 2U);
	pState_lvec1_y = LEGACY_S32_SAR(LEGACY_S32_WRAP_ADD(
		LEGACY_S32_WRAP_ADD(vecl_1C0[0].ly, vecl_1C0[1].ly),
		LEGACY_S32_WRAP_ADD(vecl_1C0[2].ly, vecl_1C0[3].ly)), 2U);
	pState_lvec1_z = LEGACY_S32_SAR(LEGACY_S32_WRAP_ADD(
		LEGACY_S32_WRAP_ADD(vecl_1C0[0].lz, vecl_1C0[1].lz),
		LEGACY_S32_WRAP_ADD(vecl_1C0[2].lz, vecl_1C0[3].lz)), 2U);

	var_DEptrTo1C0 = vecl_1C0;
	var_wheelIndex = 0;

case PLAYER_FLOW_code_update_rotCoords:
	vec_1DE[var_wheelIndex].x = physics_difference_word(
		var_DEptrTo1C0->lx, pState_lvec1_x);
	vec_1DE[var_wheelIndex].y = physics_difference_word(
		var_DEptrTo1C0->ly, pState_lvec1_y);
	vec_1DE[var_wheelIndex].z = physics_difference_word(
		var_DEptrTo1C0->lz, pState_lvec1_z);
	var_DEptrTo1C0++;
	var_wheelIndex++;
	if (var_wheelIndex < 4)
		{ physics_flow = PLAYER_FLOW_code_update_rotCoords; continue; }
	if (pState_lvec1_y >= 0)
		{ physics_flow = PLAYER_FLOW_loc_15FDE; continue; }
	pState_lvec1_y = 0;

case PLAYER_FLOW_loc_15FDE:
	if (pState_lvec1_x <= 0x1DF100)
		{ physics_flow = PLAYER_FLOW_loc_15FFE; continue; }


case PLAYER_FLOW_loc_15FEF:
	pState_lvec1_x = 0x1DF0FF;
	{ physics_flow = PLAYER_FLOW_loc_1601B; continue; }

case PLAYER_FLOW_loc_15FFE:
	if (pState_lvec1_x >= 0xF00)
		{ physics_flow = PLAYER_FLOW_loc_1601B; continue; }


case PLAYER_FLOW_loc_1600F:
	pState_lvec1_x = 0xF00;
case PLAYER_FLOW_loc_1601B:
	if (pState_lvec1_z <= 0x1DF100)
		{ physics_flow = PLAYER_FLOW_loc_1603A; continue; }



case PLAYER_FLOW_loc_1602C:
	pState_lvec1_z = 0x1DF0FF;
	{ physics_flow = PLAYER_FLOW_loc_16057; continue; }

case PLAYER_FLOW_loc_1603A:
	if (pState_lvec1_z >= 0xF00)
		{ physics_flow = PLAYER_FLOW_loc_16057; continue; }


case PLAYER_FLOW_loc_1604B:
	pState_lvec1_z = 0xF00;

case PLAYER_FLOW_loc_16057:
	var_EE = wheel_pair_delta(vec_1DE[3].x, vec_1DE[2].x,
		vec_1DE[0].x, vec_1DE[1].x);
	var_F2 = wheel_pair_delta(vec_1DE[3].z, vec_1DE[2].z,
		vec_1DE[0].z, vec_1DE[1].z);
	pState_minusRotate_y_1 = LEGACY_S16_FROM_BITS((legacy_u16)
		polarAngle(var_EE, LEGACY_S16_WRAP_NEGATE(var_F2)) & 0x3FFU);
	mat_rot_y(&var_MmatFromAngleZ, pState_minusRotate_y_1);
	var_wheelIndex = 0;

case PLAYER_FLOW_loc_160A7:
	vec_FC = vec_1DE[var_wheelIndex];
	mat_mul_vector(&vec_FC, &var_MmatFromAngleZ, &vec_1DE[var_wheelIndex]);
	var_wheelIndex++;
	if (var_wheelIndex < 4)
		{ physics_flow = PLAYER_FLOW_loc_160A7; continue; }

	var_F2 = wheel_pair_delta(vec_1DE[3].z, vec_1DE[2].z,
		vec_1DE[0].z, vec_1DE[1].z);
	var_F4 = wheel_pair_delta(vec_1DE[3].y, vec_1DE[2].y,
		vec_1DE[0].y, vec_1DE[1].y);
	//var_F2 = vec_1CC.z + vec_1D2.z - vec_1DE.z - vec_1D8.z;
	//var_F4 = vec_1CC.y + vec_1D2.y - vec_1DE.y - vec_1D8.y;
	if (var_F4 != 0)
		{ physics_flow = PLAYER_FLOW_loc_1611C; continue; }
	if (var_F2 < 0)
		{ physics_flow = PLAYER_FLOW_loc_16146; continue; }

case PLAYER_FLOW_loc_1611C:
	pState_minusRotate_x_1 = LEGACY_S16_WRAP_SUB(
		polarAngle(LEGACY_S16_WRAP_NEGATE(var_F2), var_F4), 0x100);
	if (pState_minusRotate_x_1 >= 0)
		{ physics_flow = PLAYER_FLOW_loc_1613E; continue; }
	{ physics_flow = PLAYER_FLOW_loc_16141; continue; }

case PLAYER_FLOW_loc_1613E:
	if (pState_minusRotate_x_1 >= 2)
		{ physics_flow = PLAYER_FLOW_loc_1614C; continue; }
	{ physics_flow = PLAYER_FLOW_loc_16146; continue; }
case PLAYER_FLOW_loc_16141:
	if (LEGACY_S16_WRAP_NEGATE(pState_minusRotate_x_1) >= 2)
		{ physics_flow = PLAYER_FLOW_loc_1614C; continue; }

case PLAYER_FLOW_loc_16146:
	pState_minusRotate_x_1 = 0;
case PLAYER_FLOW_loc_1614C:
	if (pState_minusRotate_x_1 == 0)
		{ physics_flow = PLAYER_FLOW_loc_161AB; continue; }
	mat_rot_x(&var_MmatFromAngleZ, pState_minusRotate_x_1);
	var_wheelIndex = 0;

case PLAYER_FLOW_loc_16169:
	vec_FC = vec_1DE[var_wheelIndex];
	mat_mul_vector(&vec_FC, &var_MmatFromAngleZ, &vec_1DE[var_wheelIndex]);
	var_wheelIndex++;
	if (var_wheelIndex < 4)
		{ physics_flow = PLAYER_FLOW_loc_16169; continue; }

case PLAYER_FLOW_loc_161AB:
	var_F2 = wheel_pair_delta(vec_1DE[1].x, vec_1DE[2].x,
		vec_1DE[0].x, vec_1DE[3].x);
	var_F4 = wheel_pair_delta(vec_1DE[1].y, vec_1DE[2].y,
		vec_1DE[0].y, vec_1DE[3].y);

	//var_F2 = vec_1DE[3].x + vec_1DE[2].x - vec_1DE[0].x - vec_1DE[1].x;
	//var_F4 = vec_1DE[3].y + vec_1DE[2].y - vec_1DE[0].y - vec_1DE[1].y;

	//var_F2 = vec_1D8.x + vec_1D2.x - vec_1DE.x - vec_1CC.x;
	//var_F4 = vec_1D8.y + vec_1D2.y - vec_1DE.y - vec_1CC.y;
	if (var_F4 != 0)
		{ physics_flow = PLAYER_FLOW_loc_161DE; continue; }
	if (var_F2 > 0)
		{ physics_flow = PLAYER_FLOW_loc_16204; continue; }

case PLAYER_FLOW_loc_161DE:
	pState_minusRotate_z_1 = LEGACY_S16_WRAP_SUB(
		polarAngle(var_F2, var_F4), 0x100);
	if (pState_minusRotate_z_1 >= 0)
		{ physics_flow = PLAYER_FLOW_loc_161FC; continue; }
	{ physics_flow = PLAYER_FLOW_loc_161FF; continue; }

case PLAYER_FLOW_loc_161FC:
	if (pState_minusRotate_z_1 >= 2)
		{ physics_flow = PLAYER_FLOW_loc_1620A; continue; }
	{ physics_flow = PLAYER_FLOW_loc_16204; continue; }
case PLAYER_FLOW_loc_161FF:
	if (LEGACY_S16_WRAP_NEGATE(pState_minusRotate_z_1) >= 2)
		{ physics_flow = PLAYER_FLOW_loc_1620A; continue; }
case PLAYER_FLOW_loc_16204:
	pState_minusRotate_z_1 = 0;
case PLAYER_FLOW_loc_1620A:
	arg_pState->car_sumSurfFrontWheels = LEGACY_S8_WRAP_ADD(
		arg_pState->car_surfaceWhl[0], arg_pState->car_surfaceWhl[1]);
	arg_pState->car_sumSurfRearWheels = LEGACY_S8_WRAP_ADD(
		arg_pState->car_surfaceWhl[2], arg_pState->car_surfaceWhl[3]);
	if (state.game_inputmode != 2)
		{ physics_flow = PLAYER_FLOW_loc_16236; continue; }
	{ physics_flow = PLAYER_FLOW_loc_16840; continue; }

case PLAYER_FLOW_loc_16236:
#ifndef RESTUNTS_HEADLESS
	if (is_in_replay != 0)
		{ physics_flow = PLAYER_FLOW_loc_1625F; continue; }
	if (arg_MplayerFlag == 0)
		{ physics_flow = PLAYER_FLOW_loc_1624A; continue; }
	audio_unk3(arg_pState->field_CF, audio_opponent_engine_channel);
	{ physics_flow = PLAYER_FLOW_loc_1624E; continue; }

case PLAYER_FLOW_loc_1624A:
	audio_unk3(arg_pState->field_CF, audio_player_engine_channel);
case PLAYER_FLOW_loc_1624E:
	//audio_unk3(arg_pState->field_CF, );
#endif

case PLAYER_FLOW_loc_1625F:
	var_EA = mat_rot_zxy(
		LEGACY_S16_WRAP_NEGATE(pState_minusRotate_z_1),
		LEGACY_S16_WRAP_NEGATE(pState_minusRotate_x_1),
		LEGACY_S16_WRAP_NEGATE(pState_minusRotate_y_1), 0);
	var_wheelIndex = 0;
	{ physics_flow = PLAYER_FLOW_loc_1632C; continue; }

case PLAYER_FLOW_loc_16288:
	var_E = planindex;
	vec_1C6 = arg_pState->car_whlWorldCrds2[var_wheelIndex];
	build_track_object(&vec_1C6, &vec_17C);
	if (var_E != planindex)
		{ physics_flow = PLAYER_FLOW_loc_16309; continue; }
	var_138 = plane_origin_op(planindex, vec_1C6.x, vec_1C6.y, vec_1C6.z);
	if (game_replay_mode == 1)
		{ physics_flow = PLAYER_FLOW_loc_16309; continue; }
	if (si >= 0)
		{ physics_flow = PLAYER_FLOW_loc_162EE; continue; }
	if (var_138 > 0)
		{ physics_flow = PLAYER_FLOW_loc_162F9; continue; }

case PLAYER_FLOW_loc_162EE:
	if (si <= 0)
		{ physics_flow = PLAYER_FLOW_loc_16309; continue; }
	if (var_138 >= 0)
		{ physics_flow = PLAYER_FLOW_loc_16309; continue; }

case PLAYER_FLOW_loc_162F9:
	update_crash_state(5, arg_MplayerFlag);

case PLAYER_FLOW_loc_16309:
	arg_pState->car_whlWorldCrds2[var_wheelIndex] = vec_17C;
	var_wheelIndex++;

case PLAYER_FLOW_loc_1632C:
	if (var_wheelIndex < 4)
		{ physics_flow = PLAYER_FLOW_loc_16336; continue; }
	{ physics_flow = PLAYER_FLOW_loc_16428; continue; }

case PLAYER_FLOW_loc_16336:
	vec_1C6 = arg_pSimd->wheel_coords[var_wheelIndex];
	vec_1C6.y = LEGACY_S16_SHL(
		arg_pSimd->collide_points[0].py, 6U);
	mat_mul_vector(&vec_1C6, var_EA, &vec_FC);

	vec_1C6.x = physics_position_word(
		LEGACY_S32_WRAP_ADD_S16(pState_lvec1_x, vec_FC.x));
	vec_1C6.y = physics_position_word(
		LEGACY_S32_WRAP_ADD_S16(pState_lvec1_y, vec_FC.y));
	vec_1C6.z = physics_position_word(
		LEGACY_S32_WRAP_ADD_S16(pState_lvec1_z, vec_FC.z));

	vec_17C = vec_1C6;
	build_track_object(&vec_1C6, &arg_pState->car_whlWorldCrds2[var_wheelIndex]);
	si = plane_origin_op(planindex, vec_1C6.x, vec_1C6.y, vec_1C6.z);
	if (planindex < 4)
		{ physics_flow = PLAYER_FLOW_loc_1641E; continue; }
	{ physics_flow = PLAYER_FLOW_loc_16288; continue; }

case PLAYER_FLOW_loc_1641E:
	if (si <= 0)
		{ physics_flow = PLAYER_FLOW_loc_16425; continue; }
	{ physics_flow = PLAYER_FLOW_loc_16309; continue; }

case PLAYER_FLOW_loc_16425:
	{ physics_flow = PLAYER_FLOW_loc_162F9; continue; }
case PLAYER_FLOW_loc_16428:
	var_11C = LEGACY_S8_WRAP_ADD(
		arg_pState->car_sumSurfFrontWheels,
		arg_pState->car_sumSurfRearWheels);
	if (arg_MplayerFlag != 0)
		{ physics_flow = PLAYER_FLOW_loc_1644C; continue; }
	if (var_11C != 0)
		{ physics_flow = PLAYER_FLOW_loc_1644C; continue; }
	if (arg_pState->car_sumSurfAllWheels == 0)
		{ physics_flow = PLAYER_FLOW_loc_1644C; continue; }
	state.game_jumpCount = LEGACY_S16_WRAP_ADD(state.game_jumpCount, 1);

case PLAYER_FLOW_loc_1644C:
	arg_pState->car_sumSurfAllWheels = var_11C;
	var_11ApStateWorldCrds[0].x = physics_position_word(pState_lvec1_x);
	var_11ApStateWorldCrds[0].y = physics_position_word(pState_lvec1_y);
	var_11ApStateWorldCrds[0].z = physics_position_word(pState_lvec1_z);

	var_11ApStateWorldCrds[1].x = pState_minusRotate_z_1;
	var_11ApStateWorldCrds[1].y = pState_minusRotate_x_1;
	var_11ApStateWorldCrds[1].z = pState_minusRotate_y_1;
	if (gameconfig.game_opponenttype != 0)
		{ physics_flow = PLAYER_FLOW_loc_164B2; continue; }
	{ physics_flow = PLAYER_FLOW_loc_16578; continue; }

case PLAYER_FLOW_loc_164B2:
	vec_18EoStateWorldCrds[0].x = physics_position_word(
		arg_oState->car_posWorld1.lx);
	vec_18EoStateWorldCrds[0].y = physics_position_word(
		arg_oState->car_posWorld1.ly);
	vec_18EoStateWorldCrds[0].z = physics_position_word(
		arg_oState->car_posWorld1.lz);

	vec_18EoStateWorldCrds[1].x = arg_oState->car_rotate.z;
	vec_18EoStateWorldCrds[1].y = arg_oState->car_rotate.y;
	vec_18EoStateWorldCrds[1].z = arg_oState->car_rotate.x;
	if (car_car_coll_detect_maybe(arg_pSimd->collide_points, var_11ApStateWorldCrds, arg_oSimd->collide_points, vec_18EoStateWorldCrds) == 0)
		{ physics_flow = PLAYER_FLOW_loc_16578; continue; }
	if (arg_pState->field_C8 == 0)
		{ physics_flow = PLAYER_FLOW_loc_1653E; continue; }
	{ physics_flow = PLAYER_FLOW_loc_16892; continue; }

case PLAYER_FLOW_loc_1653E:
	if (car_car_speed_adjust_maybe(arg_pState, arg_oState) != 0)
		{ physics_flow = PLAYER_FLOW_loc_16550; continue; }
	{ physics_flow = PLAYER_FLOW_loc_16892; continue; }

case PLAYER_FLOW_loc_16550:
	update_crash_state(1, arg_MplayerFlag);
	update_crash_state(1, arg_MplayerFlag ^ 1);
	return;

case PLAYER_FLOW_loc_16566:
	return;

case PLAYER_FLOW_loc_16578:
	vec_FC.x = LEGACY_S16_SAR(var_11ApStateWorldCrds[0].x, 10U);
	vec_FC.z = LEGACY_S16_WRAP_NEGATE(LEGACY_S16_WRAP_SUB(
		LEGACY_S16_SAR(var_11ApStateWorldCrds[0].z, 10U), 0x1D));
	vec_18EoStateWorldCrds[1].x = 0;
	vec_18EoStateWorldCrds[1].y = 0;
	vec_18EoStateWorldCrds[1].z = 0;
	if (vec_FC.x >= 0)
		{ physics_flow = PLAYER_FLOW_loc_165AF; continue; }
	{ physics_flow = PLAYER_FLOW_loc_16840; continue; }

case PLAYER_FLOW_loc_165AF:
	if (vec_FC.x < 0x1E)
		{ physics_flow = PLAYER_FLOW_loc_165B9; continue; }
	{ physics_flow = PLAYER_FLOW_loc_16840; continue; }

case PLAYER_FLOW_loc_165B9:
	if (vec_FC.z >= 0)
		{ physics_flow = PLAYER_FLOW_loc_165C0; continue; }
	{ physics_flow = PLAYER_FLOW_loc_16840; continue; }

case PLAYER_FLOW_loc_165C0:
	if (vec_FC.z < 0x1E)
		{ physics_flow = PLAYER_FLOW_loc_165C8; continue; }
	{ physics_flow = PLAYER_FLOW_loc_16840; continue; }

case PLAYER_FLOW_loc_165C8:
	var_EC = bto_auxiliary1(vec_FC.x, vec_FC.z, var_DC);
	if (var_EC == 0)
		{ physics_flow = PLAYER_FLOW_loc_16650; continue; }
	si = 0;
	{ physics_flow = PLAYER_FLOW_loc_165F0; continue; }

case PLAYER_FLOW_loc_165EA:
	// NOTE: var_144 is unused
	// var_144 += 6;
	si++;

case PLAYER_FLOW_loc_165F0:
	if (var_EC <= si)
		{ physics_flow = PLAYER_FLOW_loc_16650; continue; }
	vec_18EoStateWorldCrds[0].x = var_DC[si].x;
	vec_18EoStateWorldCrds[0].y = var_DC[si].y;
	vec_18EoStateWorldCrds[0].z = var_DC[si].z;
	if (car_car_coll_detect_maybe(arg_pSimd->collide_points, var_11ApStateWorldCrds, unk_3BD6A, vec_18EoStateWorldCrds) == 0)
		{ physics_flow = PLAYER_FLOW_loc_165EA; continue; }
	arg_pState->car_36MwhlAngle = LEGACY_S16_WRAP_SUB(
		arg_pState->car_36MwhlAngle, 0x200);

case PLAYER_FLOW_loc_16648:
	// crash with start/finish pole
	update_crash_state(1, arg_MplayerFlag);
	return ;

case PLAYER_FLOW_loc_16650:
	si = (legacy_s8)trackdata19[trackrows[vec_FC.z] + vec_FC.x];
	if (si != -1) //0xFF) // note: checking for 0xff elsewhere, should be signed and check for -1
		{ physics_flow = PLAYER_FLOW_loc_16670; continue; }
	{ physics_flow = PLAYER_FLOW_loc_16710; continue; }

case PLAYER_FLOW_loc_16670:
	if (state.field_3FA[si] == 0)
		{ physics_flow = PLAYER_FLOW_loc_1667A; continue; }
	{ physics_flow = PLAYER_FLOW_loc_16710; continue; }

case PLAYER_FLOW_loc_1667A:
	vec_18EoStateWorldCrds[0].x = td10_track_check_rel[si * 3 + 0];
	vec_18EoStateWorldCrds[0].y = td10_track_check_rel[si * 3 + 1];
	vec_18EoStateWorldCrds[0].z = td10_track_check_rel[si * 3 + 2];
	if (car_car_coll_detect_maybe(arg_pSimd->collide_points, var_11ApStateWorldCrds, unk_3BD5A, vec_18EoStateWorldCrds) == 0)
		{ physics_flow = PLAYER_FLOW_loc_16710; continue; }

	state.field_3FA[si] = 1;

	state_op_unk(LEGACY_S16_WRAP_ADD(si, 2),
		LEGACY_S16_WRAP_NEGATE(arg_pState->car_rotate.x),
		scale_speed_to_travel(arg_pState->car_speed2, 0x3C00U));

case PLAYER_FLOW_loc_16710:
	// following looks like collision detection against right and left start/finish poles
	if (vec_FC.x == startcol2)
		{ physics_flow = PLAYER_FLOW_loc_1671F; continue; }
	{ physics_flow = PLAYER_FLOW_loc_16840; continue; }

case PLAYER_FLOW_loc_1671F:
	if (vec_FC.z == startrow2)
		{ physics_flow = PLAYER_FLOW_loc_1672C; continue; }
	{ physics_flow = PLAYER_FLOW_loc_16840; continue; }

case PLAYER_FLOW_loc_1672C:
	vec_18EoStateWorldCrds[0].x = LEGACY_S16_WRAP_ADD(
		trackcenterpos2[startcol2], multiply_and_scale(sin_fast(
			LEGACY_S16_WRAP_ADD(track_angle, 0x100)), 0x7E));
	vec_18EoStateWorldCrds[0].y = hillHeightConsts[hillFlag];
	vec_18EoStateWorldCrds[0].z = LEGACY_S16_WRAP_ADD(
		trackcenterpos[startrow2], multiply_and_scale(cos_fast(
			LEGACY_S16_WRAP_ADD(track_angle, 0x100)), 0x7E));

	var_138 = car_car_coll_detect_maybe(arg_pSimd->collide_points, var_11ApStateWorldCrds, unk_3BD62, vec_18EoStateWorldCrds);
	if (var_138 != 0)
		{ physics_flow = PLAYER_FLOW_loc_16836; continue; }

	vec_18EoStateWorldCrds[0].x = LEGACY_S16_WRAP_ADD(
		trackcenterpos2[startcol2], multiply_and_scale(sin_fast(
			LEGACY_S16_WRAP_ADD(track_angle, 0x300)), 0x7E));
	vec_18EoStateWorldCrds[0].z = LEGACY_S16_WRAP_ADD(
		trackcenterpos[startrow2], multiply_and_scale(cos_fast(
			LEGACY_S16_WRAP_ADD(track_angle, 0x300)), 0x7E));

	var_138 = car_car_coll_detect_maybe(arg_pSimd->collide_points, var_11ApStateWorldCrds, unk_3BD62, vec_18EoStateWorldCrds);

case PLAYER_FLOW_loc_16836:
	if (var_138 == 0)
		{ physics_flow = PLAYER_FLOW_loc_16840; continue; }
	{ physics_flow = PLAYER_FLOW_loc_16648; continue; }

case PLAYER_FLOW_loc_16840:
	arg_pState->car_posWorld1.lx = pState_lvec1_x;
	arg_pState->car_posWorld1.ly = pState_lvec1_y;
	arg_pState->car_posWorld1.lz = pState_lvec1_z;
	arg_pState->car_rotate.z = pState_minusRotate_z_1;
	arg_pState->car_rotate.y = pState_minusRotate_x_1;
	arg_pState->car_rotate.x = pState_minusRotate_y_1;
	arg_pState->field_C8 = 0;

case PLAYER_FLOW_loc_16892:
	return ;

	}
	}
}
