#ifndef RESTUNTS_PHYSICS_INTERNAL_H
#define RESTUNTS_PHYSICS_INTERNAL_H

#include "math.h"

struct CARSTATE;

extern legacy_s32 car_working_x;
extern legacy_s32 car_working_y;
extern legacy_s32 car_working_z;
extern legacy_s16 car_working_roll;
extern legacy_s16 car_working_yaw;
extern legacy_s16 car_working_pitch;
extern legacy_s16 nextPosAndNormalIP;

extern struct POINT2D start_finish_pole_bounds[2];
extern struct POINT2D breakable_object_bounds[2];
extern struct POINT2D track_auxiliary_obstacle_bounds[2];
extern legacy_s16 wheel_gravity_steps[4];
extern struct VECTOR scenery_collision_points[];
extern struct VECTOR elevated_road_collision_points[];
extern struct VECTOR corkscrew_lr_collision_points[];
extern struct VECTOR corkscrew_up_collision_points[];
extern struct VECTOR corkscrew_down_collision_points[];
extern struct VECTOR slalom_collision_points[];

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
