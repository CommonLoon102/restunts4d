#ifndef RESTUNTS_TRACK_OBJECTS_H
#define RESTUNTS_TRACK_OBJECTS_H

#include "track_types.h"

/* Decoded track objects, route setup, and roadside placement data. */

#define PENALTY_ROUTE_FINISH_REACHED (-1)
#define PENALTY_ROUTE_OUTSIDE_TRACK (-2)
#define TRACK_ROUTE_LINK_NONE (-1)

extern struct TRACKOBJECT trkObjectList[215];
extern struct PLANE far plan_memres;
extern legacy_s16 track_pieces_counter;
extern legacy_u8 roadside_sign_forward_types[];
extern legacy_u8 roadside_sign_reverse_types[];
extern legacy_u8 terrConnDataEtoW[];
extern legacy_u8 terrConnDataWtoE[];
extern legacy_u8 terrConnDataNtoS[];
extern legacy_u8 terrConnDataStoN[];
extern legacy_u8 roadside_sign_count;
extern legacy_u8 track_validation_column;
extern legacy_u8 track_validation_row;
extern legacy_u8 trackside_camera_count;

struct VECTOR *track_vector_from_legacy_offset(legacy_u16 offset);

legacy_u8 subst_hillroad_track(legacy_u8 terrain, legacy_u8 track);

void init_plantrak(void);

legacy_s16 track_object_base_x(const struct TRACKOBJECT *track_object, legacy_u8 column);

legacy_s16 track_object_base_z(const struct TRACKOBJECT *track_object, legacy_u8 row);
extern legacy_s16 track_setup(void);

#endif
