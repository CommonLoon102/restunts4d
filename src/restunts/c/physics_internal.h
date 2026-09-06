#ifndef RESTUNTS_PHYSICS_INTERNAL_H
#define RESTUNTS_PHYSICS_INTERNAL_H

#include "externs.h"

extern legacy_s32 car_working_x;
extern legacy_s32 car_working_y;
extern legacy_s32 car_working_z;
extern legacy_s16 car_working_roll;
extern legacy_s16 car_initial_roll;
extern legacy_s16 car_working_yaw;
extern legacy_s16 car_initial_yaw;
extern legacy_s16 car_working_pitch;
extern legacy_s16 car_initial_pitch;
extern struct MATRIX car_to_world_rotation;
extern struct VECTOR wheel_forward_travel;
extern legacy_s16 planindex;
extern legacy_s16 planindex_copy;
extern legacy_s16 wheel_heading_offset;
extern struct VECTOR wheel_world_travel;
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
extern legacy_s8 track_wall_collision_enabled;

extern struct POINT2D start_finish_pole_bounds[2];
extern struct POINT2D breakable_object_bounds[2];
extern struct POINT2D track_auxiliary_obstacle_bounds[2];
extern legacy_s16 wheel_gravity_steps[4];
extern legacy_s16 audio_opponent_engine_channel;
extern legacy_s16 audio_player_engine_channel;
extern struct TRACKOBJECT trkObjectList[215];
extern struct VECTOR scenery_collision_points[];
extern struct VECTOR elevated_road_collision_points[];
extern struct VECTOR corkscrew_lr_collision_points[];
extern struct VECTOR corkscrew_up_collision_points[];
extern struct VECTOR corkscrew_down_collision_points[];
extern struct VECTOR slalom_collision_points[];

extern void update_crash_state(legacy_s16, legacy_s16);
extern void build_track_object(struct VECTOR*, struct VECTOR*);
extern void audio_play_car_events(legacy_u8, legacy_s16);

legacy_s16 scale_position_delta(legacy_s32 current,
	legacy_s32 previous, legacy_s16 factor, legacy_s16 divisor);
legacy_s16 scale_speed_to_travel(legacy_u16 speed,
	legacy_u16 divisor);
legacy_s16 physics_difference_word(legacy_s32 left, legacy_s32 right);
legacy_s16 wheel_pair_delta(legacy_s16 first, legacy_s16 second,
	legacy_s16 third, legacy_s16 fourth);

legacy_s16 get_track_collision_points(legacy_s16 column, legacy_s16 row,
	struct VECTOR* output);
legacy_s16 update_wheel_suspension(struct CARSTATE* carstate,
	legacy_s16 contact_delta, legacy_s16 wheel_index);
legacy_s16 resolve_car_collision_speeds(struct CARSTATE* first_state,
	struct CARSTATE* second_state);
legacy_s16 car_collision_boxes_overlap(
	struct POINT2D* first_collision_points,
	struct VECTOR* first_world_coordinates,
	struct POINT2D* second_collision_points,
	struct VECTOR* second_world_coordinates);

#endif
