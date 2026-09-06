#ifndef RESTUNTS_CAMERA_H
#define RESTUNTS_CAMERA_H

#include "legacy.h"

enum CAMERA_MODE {
	CAMERA_MODE_COCKPIT = 0,
	CAMERA_MODE_FOLLOW = 1,
	CAMERA_MODE_CUSTOM = 2,
	CAMERA_MODE_TRACKSIDE = 3
};

#define CAMERA_MODE_COUNT 4U
#define CAMERA_MODE_MASK (CAMERA_MODE_COUNT - 1U)

/* The free camera the player steers with the keypad in replay mode. */
struct CUSTOM_CAMERA {
	legacy_s16 distance;
	legacy_s16 elevation_angle;
	legacy_s16 azimuth_angle;
};
extern struct CUSTOM_CAMERA custom_camera;
extern legacy_s8 cameramode;
extern legacy_s8 followOpponentFlag;
extern legacy_s8 followOpponentFlag_copy;
extern legacy_s16 camera_track_height_offset;

#endif
