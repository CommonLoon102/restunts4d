#ifndef RESTUNTS_GAMESTATE_H
#define RESTUNTS_GAMESTATE_H

#include "math.h"

#define GAMESTATE_SERIALIZED_SIZE 1120U
#define CARSTATE_SERIALIZED_SIZE 208U
#define CARSTATE_WHEEL_COUNT 4U
#define GAMESTATE_PARTICLE_SLOT_COUNT 24U
#define GAMESTATE_CAR_VECTOR_COUNT 2U
#define GAMESTATE_PARTICLE_VELOCITY_BYTES \
	(GAMESTATE_PARTICLE_SLOT_COUNT * LEGACY_WORD_BYTES)
#define GAMESTATE_RANDOM_SEED_SIZE 6U
#define GAMESTATE_FIELD_3F7_SIZE 2U
#define GAMESTATE_FIELD_3FA_SIZE 48U

#pragma pack (push, 1)

struct CARSTATE {
	struct VECTORLONG car_posWorld1;
	struct VECTORLONG car_posWorld2;
	struct VECTOR car_rotate; /* Rotation angles, despite the vector notation. */
	legacy_s16 car_pseudoGravity;
	legacy_s16 car_steeringAngle;
	legacy_s16 car_currpm;
	legacy_s16 car_lastrpm;
	legacy_s16 car_idlerpm2;
	legacy_s16 car_speeddiff; /* Formerly called gripdiff. */
	legacy_u16 car_speed; /* Rev-coupled speed, scaled by 2^8. */
	legacy_u16 car_speed2; /* Actual car speed, scaled by 2^8. */
	legacy_u16 car_lastspeed;
	legacy_u16 car_gearratio;
	legacy_u16 car_gearratioshr8;
	legacy_s16 car_knob_x;
	legacy_s16 car_36MwhlAngle;
	legacy_s16 car_knob_y;
	legacy_s16 car_knob_x2;
	legacy_s16 car_knob_y2;
	legacy_s16 car_angle_z;
	legacy_s16 car_40MfrontWhlAngle;
	legacy_s16 field_42;
	legacy_s16 car_demandedGrip;
	legacy_s16 car_surfacegrip_sum;
	legacy_s16 field_48;
	legacy_s16 car_trackdata3_index;
	legacy_s16 car_rc1[CARSTATE_WHEEL_COUNT];
	legacy_s16 car_rc2[CARSTATE_WHEEL_COUNT];
	legacy_s16 car_rc3[CARSTATE_WHEEL_COUNT];
	legacy_s16 car_rc4[CARSTATE_WHEEL_COUNT];
	legacy_s16 car_rc5[CARSTATE_WHEEL_COUNT];
	struct VECTOR car_whlWorldCrds1[CARSTATE_WHEEL_COUNT];
	struct VECTOR car_whlWorldCrds2[CARSTATE_WHEEL_COUNT];
	struct VECTOR car_vec_unk3;
	struct VECTOR car_vec_unk4;
	struct VECTOR car_vec_unk5;
	legacy_s16 field_B6;
	legacy_s16 field_B8;
	legacy_s16 field_BA;
	legacy_s8 car_is_braking;
	legacy_s8 car_is_accelerating;
	legacy_s8 car_current_gear;
	legacy_s8 car_sumSurfFrontWheels;
	legacy_s8 car_sumSurfRearWheels;
	legacy_s8 car_sumSurfAllWheels; /* Also used as the jump flag. */
	legacy_s8 car_surfaceWhl[CARSTATE_WHEEL_COUNT];
	legacy_s8 car_engineLimiterTimer;
	legacy_s8 car_slidingFlag;
	legacy_s8 field_C8;
	legacy_s8 car_crashBmpFlag;
	legacy_s8 car_changing_gear;
	legacy_s8 car_fpsmul2;
	legacy_s8 car_transmission;
	legacy_s8 field_CD;
	legacy_s8 field_CE;
	legacy_s8 field_CF;
};

struct GAMESTATE {
	legacy_s32 game_longs1[GAMESTATE_PARTICLE_SLOT_COUNT]; /* x */
	legacy_s32 game_longs2[GAMESTATE_PARTICLE_SLOT_COUNT]; /* y */
	legacy_s32 game_longs3[GAMESTATE_PARTICLE_SLOT_COUNT]; /* z */
	struct VECTOR game_vec1[GAMESTATE_CAR_VECTOR_COUNT];
	struct VECTOR game_vec3;
	struct VECTOR game_vec4;
	legacy_s16 game_frame_in_sec;
	legacy_s16 game_frames_per_sec;
	legacy_s32 game_travDist;
	legacy_s16 game_frame;
	legacy_s16 game_total_finish; /* Finish time plus penalty. */
	legacy_s16 field_144;
	legacy_s16 game_pEndFrame;
	legacy_s16 game_oEndFrame;
	legacy_s16 game_penalty;
	legacy_u16 game_impactSpeed;
	legacy_u16 game_topSpeed;
	legacy_s16 game_jumpCount;
	struct CARSTATE playerstate;
	struct CARSTATE opponentstate;
	legacy_s16 field_2F2;
	legacy_s16 field_2F4;
	legacy_s16 game_startcol;
	legacy_s16 game_startcol2;
	legacy_s16 game_startrow;
	legacy_s16 game_startrow2;
	legacy_s16 field_2FE[GAMESTATE_PARTICLE_SLOT_COUNT];
	legacy_s16 field_32E[GAMESTATE_PARTICLE_SLOT_COUNT];
	legacy_s16 field_35E[GAMESTATE_PARTICLE_SLOT_COUNT];
	legacy_s16 field_38E[GAMESTATE_PARTICLE_SLOT_COUNT];
	legacy_s8 field_3BE[GAMESTATE_PARTICLE_VELOCITY_BYTES];
	legacy_s8 kevinseed[GAMESTATE_RANDOM_SEED_SIZE];
	legacy_s8 field_3F4;
	legacy_s8 game_inputmode; /* 0 waiting, 1 active, 2 intro. */
	legacy_s8 game_3F6autoLoadEvalFlag;
	legacy_s8 field_3F7[GAMESTATE_FIELD_3F7_SIZE];
	legacy_s8 field_3F9;
	legacy_s8 field_3FA[GAMESTATE_FIELD_3FA_SIZE];
	legacy_s8 field_42A;
	legacy_s8 field_42B[GAMESTATE_PARTICLE_SLOT_COUNT];
	legacy_s8 field_443[GAMESTATE_PARTICLE_SLOT_COUNT];
	legacy_s8 field_45B;
	legacy_s8 field_45C;
	legacy_s8 field_45D;
	legacy_s8 field_45E;
	legacy_s8 field_45F;
};

#pragma pack (pop)

typedef char legacy_carstate_must_be_208_bytes[
	(sizeof(struct CARSTATE) == CARSTATE_SERIALIZED_SIZE) ? 1 : -1];
typedef char legacy_gamestate_must_be_1120_bytes[
	(sizeof(struct GAMESTATE) == GAMESTATE_SERIALIZED_SIZE) ? 1 : -1];

legacy_u16 gamestate_serialize(legacy_u8 far* destination,
	const struct GAMESTATE* source);

#endif
