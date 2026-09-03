#ifndef RESTUNTS_PHYSICS_INTERNAL_H
#define RESTUNTS_PHYSICS_INTERNAL_H

#include "externs.h"

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
