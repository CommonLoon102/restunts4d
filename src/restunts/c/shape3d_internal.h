#ifndef RESTUNTS_SHAPE3D_INTERNAL_H
#define RESTUNTS_SHAPE3D_INTERNAL_H

#include "shape3d.h"

#define DRAW_LINE_FIXED_ROUNDING 32768UL
#define DRAW_LINE_DEGENERATE_STEP 4956U
#define DRAW_LINE_MIN_MAJOR_LENGTH 2U
#define DRAW_LINE_MODE_MASK LEGACY_U8_MAX
#define DRAW_LINE_CLIP_SHIFT LEGACY_BYTE_BITS

#define DRAW_LINE_CLIP_RIGHT 1U
#define DRAW_LINE_CLIP_LEFT 2U
#define DRAW_LINE_CLIP_TOP 4U
#define DRAW_LINE_CLIP_BOTTOM 8U
#define DRAW_LINE_CLIP_MASK 15U

enum DRAW_LINE_MODE {
	DRAW_LINE_MODE_HORIZONTAL_REVERSED = 0,
	DRAW_LINE_MODE_HORIZONTAL = 1,
	DRAW_LINE_MODE_VERTICAL = 2,
	DRAW_LINE_MODE_DIAGONAL_LEFT = 3,
	DRAW_LINE_MODE_DIAGONAL_RIGHT = 4,
	DRAW_LINE_MODE_Y_MAJOR_LEFT = 5,
	DRAW_LINE_MODE_Y_MAJOR_RIGHT = 6,
	DRAW_LINE_MODE_X_MAJOR_LEFT = 7,
	DRAW_LINE_MODE_X_MAJOR_RIGHT = 8,
	DRAW_LINE_MODE_POINT = 9,
	DRAW_LINE_MODE_UNSET = LEGACY_U8_MAX
};

#define DRAW_LINE_SUBDIVIDE_MIN (-16000)
#define DRAW_LINE_SUBDIVIDE_MAX 16000

enum DRAW_LINE_BUFFER_INDEX {
	DRAW_LINE_START_X_FRACTION_INDEX = 0,
	DRAW_LINE_START_X_INDEX = 1,
	DRAW_LINE_START_Y_FRACTION_INDEX = 2,
	DRAW_LINE_START_Y_INDEX = 3,
	DRAW_LINE_END_X_INDEX = 4,
	DRAW_LINE_END_Y_INDEX = 5,
	DRAW_LINE_STEP_INDEX = 6,
	DRAW_LINE_PIXEL_COUNT_INDEX = 7,
	DRAW_LINE_COLOR_INDEX = 8,
	DRAW_LINE_MODE_AND_CLIP_INDEX = 9,
	DRAW_LINE_START_LEFT_CLIP_COUNT_INDEX = 10,
	DRAW_LINE_END_LEFT_CLIP_COUNT_INDEX = 11,
	DRAW_LINE_START_RIGHT_CLIP_COUNT_INDEX = 12,
	DRAW_LINE_END_RIGHT_CLIP_COUNT_INDEX = 13
};

#define DRAW_LINE_WORD_COUNT 14

extern struct SHAPE3D game3dshapes[130];
extern legacy_s8 car_shape_resource_name[];
extern legacy_s8 far* carresptr;
extern legacy_s8 far* car2resptr;
extern struct VECTOR player_front_wheel_centers[2];
extern struct VECTOR player_base_wheel_vertices[];
extern struct VECTOR opponent_front_wheel_centers[2];
extern struct VECTOR opponent_base_wheel_vertices[];
extern legacy_s16 player_wheel_vertex_state[];
extern legacy_s16 opponent_wheel_vertex_state[];
extern legacy_s16 neutral_wheel_suspension[];
extern void (*spritefunc)(legacy_s16*, legacy_s16*, legacy_u16,
	legacy_u16, legacy_u16);
extern void (*imagefunc)(legacy_u16, legacy_u16, legacy_u16,
	legacy_u16, legacy_u16);
extern legacy_u8* sphere_radius_rows[];

#endif
