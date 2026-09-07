#ifndef RESTUNTS_SKYBOX_H
#define RESTUNTS_SKYBOX_H

#include "math.h"

struct SHAPE2D;

/* The four horizon band images and the colours drawn around them. heights
 * is indexed the same way as the skyboxes[] resource array. */
struct SKYBOX {
	legacy_u16 heights[4];
	legacy_u16 minimum_height;
	legacy_u16 maximum_height;
	legacy_s16 sky_color;
	legacy_s16 ground_color;
	legacy_s16 water_color;
};
extern struct SKYBOX skybox;
extern legacy_s8 skybox_resources_loaded;
extern legacy_s8 far *skybox_res_ofs;
extern legacy_s8 loaded_skybox_index;
extern struct SHAPE2D far *skyboxes[];

void load_skybox(legacy_s8 skybox_index);

void unload_skybox(void);

void skybox_render_level_rect(struct RECTANGLE *rect, legacy_s16 angle, legacy_s16 horizon);

legacy_s16 skybox_render(legacy_s16 view_index, struct RECTANGLE *clip, legacy_s16 direction,
						 struct MATRIX *rotation, legacy_s16 roll, legacy_s16 angle,
						 legacy_s16 camera_y);

#endif
