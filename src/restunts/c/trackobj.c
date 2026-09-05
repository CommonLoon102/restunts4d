#include "restunts.h"
#include "legacy.h"
#include "math.h"

#define SURFACE_GRASS 4
#define SURFACE_WATER 5
#define TRACK_GRID_LAST_INDEX 29
#define TRACK_TILE_COORDINATE_SHIFT 10U
#define TRACK_ARC_CENTER_OFFSET 1024
#define TRACK_ARC_CENTER_RADIUS 1536
#define TRACK_ARC_SEGMENT_COUNT 18U
#define TRACK_ARC_LAST_SEGMENT 17
#define TERRAIN_SLOPE_2_ANGLE 128
#define TERRAIN_SLOPE_3_ANGLE -640
#define TERRAIN_SLOPE_4_ANGLE -384
#define TERRAIN_SLOPE_5_ANGLE -128
#define TRACK_TILE_CONTINUATION_SOUTHEAST 253U
#define TRACK_TILE_CONTINUATION_SOUTH 254U
#define TRACK_TILE_CONTINUATION_EAST 255U
#define HILL_TERRAIN_FIRST 7U
#define HILL_TERRAIN_END 11U
#define TRACK_PHYSICAL_MODEL_MAXIMUM 74
#define ROAD_HALF_WIDTH 120
#define START_FINISH_FAR_Z -380
#define START_FINISH_NEAR_Z -300
#define START_FINISH_FAR_PLANE_INDEX 131
#define START_FINISH_NEAR_PLANE_INDEX 132
#define LARGE_CORNER_INNER_RADIUS 1416
#define LARGE_CORNER_OUTER_RADIUS 1656
#define SHARP_CORNER_CENTER_OFFSET 512
#define SHARP_CORNER_INNER_RADIUS 392
#define SHARP_CORNER_OUTER_RADIUS 632
#define SPLIT_STRAIGHT_LANE_INNER_X 392
#define SPLIT_STRAIGHT_LANE_OUTER_X 632
#define HIGHWAY_PLANE_INDEX 1
#define HIGHWAY_MERGE_END_Z 334
#define HIGHWAY_OUTER_HALF_WIDTH 360
#define HIGHWAY_FAR_LEFT_WALL_INDEX 188
#define HIGHWAY_FAR_RIGHT_WALL_INDEX 186
#define HIGHWAY_NEAR_LEFT_WALL_INDEX 189
#define HIGHWAY_NEAR_RIGHT_WALL_INDEX 187
#define RAMP_PLANE_INDEX 3
#define RAMP_ENTRY_WALL_INDEX 102
#define ELEVATED_DECK_CLEARANCE 390
#define ELEVATED_PLANE_INDEX 2
#define ELEVATED_ROAD_END_Z 476
#define ELEVATED_FORWARD_WALL_INDEX 103
#define ELEVATED_REAR_WALL_INDEX 104
#define ELEVATED_SIDE_WALL_HEIGHT 42
#define ELEVATED_LEFT_WALL_INDEX 100
#define ELEVATED_RIGHT_WALL_INDEX 101
#define ELEVATED_CORNER_OUTER_OFFSET 150
#define ELEVATED_CORNER_INNER_OFFSET 108
#define ELEVATED_CORNER_INNER_WALL_BASE 105
#define ELEVATED_CORNER_OUTER_WALL_BASE 123
#define BANKED_ENTRANCE_A_PLAN_BASE 35
#define BANKED_ENTRANCE_A_ANGLE -672
#define BANKED_ENTRANCE_B_PLAN_BASE 25
#define BANKED_ENTRANCE_B_ANGLE 160
#define BANKED_ENTRANCE_END_Z 334
#define BANKED_ENTRANCE_INNER_Z 168
#define BANKED_ROAD_PLANE_INDEX 6
#define BANKED_CORNER_INNER_OFFSET 120
#define BANKED_CORNER_OUTER_OFFSET 126
#define BANKED_CORNER_WALL_THRESHOLD 102
#define BANKED_CORNER_PLAN_BASE 7
#define BANKED_CORNER_WALL_BASE 123
#define LOOP_REAR_PLAN_BASE 51
#define LOOP_FRONT_PLAN_BASE 45
#define LOOP_SURFACE_END_PADDING 100
#define LOOP_SURFACE_LAST_INDEX 5U
#define LOOP_UPPER_HEIGHT_THRESHOLD 524
#define LOOP_LANE_SEPARATION 400
#define LOOP_LOW_CLEARANCE 100
#define LOOP_LOW_CLEARANCE_LAST_SEGMENT 1U
#define TUNNEL_HEIGHT 144
#define TUNNEL_OUTER_HALF_WIDTH 270
#define TUNNEL_ROOF_PLANE_INDEX 133
#define TUNNEL_END_Z 512
#define TUNNEL_REAR_WALL_INDEX 154
#define TUNNEL_FORWARD_WALL_INDEX 153
#define TUNNEL_RIGHT_INNER_WALL_INDEX 152
#define TUNNEL_RIGHT_OUTER_WALL_INDEX 150
#define TUNNEL_LEFT_INNER_WALL_INDEX 151
#define TUNNEL_LEFT_OUTER_WALL_INDEX 149
#define PIPE_HALF_WIDTH 164
#define PIPE_WALL_HEIGHT 151
#define PIPE_ENTRANCE_WALL_INNER_X 115
#define PIPE_ENTRANCE_LEFT_WALL_INDEX 160
#define PIPE_ENTRANCE_RIGHT_WALL_INDEX 159
#define PIPE_ENTRANCE_MAX_HEIGHT 171
#define PIPE_ENTRANCE_CENTER_HALF_WIDTH 31
#define PIPE_ENTRANCE_CENTER_PLAN_INDEX 70
#define PIPE_ENTRANCE_SIDE_SPLIT_X 84
#define PIPE_ENTRANCE_LEFT_OUTER_PLAN_BASE 73
#define PIPE_ENTRANCE_LEFT_INNER_PLAN_BASE 71
#define PIPE_ENTRANCE_RIGHT_OUTER_PLAN_BASE 77
#define PIPE_ENTRANCE_RIGHT_INNER_PLAN_BASE 75
#define PIPE_ENTRANCE_OUTER_CENTER_X 100
#define PIPE_ENTRANCE_INNER_CENTER_X 57
#define PIPE_ENTRANCE_OUTER_ANGLE 5
#define PIPE_ENTRANCE_INNER_ANGLE 8
#define PIPE_LEFT_WALL_INDEX 156
#define PIPE_RIGHT_WALL_INDEX 155
#define PIPE_MAX_HEIGHT 265
#define PIPE_SURFACE_HALF_WIDTH 130
#define HALF_PIPE_FLOOR_HALF_WIDTH 84
#define HALF_PIPE_FLOOR_HALF_LENGTH 75
#define HALF_PIPE_FLOOR_PLAN_INDEX 69
#define HALF_PIPE_REAR_WALL_INDEX 157
#define HALF_PIPE_FORWARD_WALL_INDEX 158
#define PIPE_SIDE_HEIGHT_SPLIT 88
#define PIPE_UPPER_LEFT_SIDE_PLAN_INDEX 60
#define PIPE_UPPER_RIGHT_SIDE_PLAN_INDEX 66
#define PIPE_UPPER_CENTER_PLAN_INDEX 63
#define PIPE_UPPER_LEFT_OUTER_PLAN_INDEX 61
#define PIPE_UPPER_LEFT_INNER_PLAN_INDEX 62
#define PIPE_UPPER_RIGHT_OUTER_PLAN_INDEX 65
#define PIPE_UPPER_RIGHT_INNER_PLAN_INDEX 64
#define PIPE_LOWER_CENTER_PLAN_INDEX 57
#define PIPE_LOWER_LEFT_OUTER_PLAN_INDEX 59
#define PIPE_LOWER_LEFT_INNER_PLAN_INDEX 58
#define PIPE_LOWER_RIGHT_OUTER_PLAN_INDEX 67
#define PIPE_LOWER_RIGHT_INNER_PLAN_INDEX 68
#define PIPE_FULL 0
#define PIPE_HALF 1
#define PIPE_LOWER_HALF 0
#define PIPE_UPPER_HALF 1
#define NO_PLANE_INDEX 0
#define CORKSCREW_INACTIVE 0
#define CORKSCREW_ACTIVE 1
#define CORK_LR_HALF_WIDTH 150
#define CORK_LR_MAX_HEIGHT 265
#define CORK_LR_UPPER_HEIGHT 151
#define CORK_LR_SIDE_HEIGHT_SPLIT 88
#define CORK_LR_LOWER_HALF 0
#define CORK_LR_UPPER_HALF 1
#define CORK_LR_NO_SEGMENT 0
#define CORK_LR_CENTER_HALF_WIDTH 31
#define CORK_LR_SIDE_SPLIT_X 84
#define CORK_LR_LOWER_LEFT_SIDE_SEGMENT 3
#define CORK_LR_LOWER_RIGHT_SIDE_SEGMENT 9
#define CORK_LR_UPPER_CENTER_SEGMENT 6
#define CORK_LR_UPPER_LEFT_OUTER_SEGMENT 4
#define CORK_LR_LOWER_LEFT_OUTER_SEGMENT 2
#define CORK_LR_UPPER_LEFT_INNER_SEGMENT 5
#define CORK_LR_LOWER_LEFT_INNER_SEGMENT 1
#define CORK_LR_UPPER_RIGHT_OUTER_SEGMENT 8
#define CORK_LR_LOWER_RIGHT_OUTER_SEGMENT 10
#define CORK_LR_UPPER_RIGHT_INNER_SEGMENT 7
#define CORK_LR_LOWER_RIGHT_INNER_SEGMENT 11
#define CORK_LR_PLAN_BASE 57
#define CORK_LR_END_Z 512
#define CORK_LR_WALL_INDEX 185
#define CORK_LR_WALL_HEIGHT 117
#define CORK_UD_A_PLAN_BASE 79
#define CORK_UD_A_OUTER_WALL_BASE 50
#define CORK_UD_A_INNER_WALL_BASE 75
#define CORK_UD_B_PLAN_BASE 105
#define CORK_UD_B_OUTER_WALL_BASE 0
#define CORK_UD_B_INNER_WALL_BASE 25
#define CORK_UD_LOW_HEIGHT 100
#define CORK_UD_UPPER_HEIGHT 350
#define CORK_UD_LOWER_INNER_RADIUS 392
#define CORK_UD_LOWER_OUTER_RADIUS 632
#define CORK_UD_INNER_RADIUS 332
#define CORK_UD_OUTER_RADIUS 692
#define CORK_UD_CENTER_RADIUS 512
#define CORK_UD_WALL_RADIUS_OFFSET 90
#define CORK_UD_WALL_HEIGHT 42
#define CORK_UD_WALL_INDEX_OFFSET 24
#define CORK_UD_UPPER_PLAN_OFFSET 25
#define CORK_UD_ARC_SEGMENT_COUNT 24U
#define CORK_UD_ANGLE_SCALE_SHIFT 10U
#define CORK_UD_FIRST_ARC_PLAN_OFFSET 1
#define ELEVATED_WALL_VERTICAL_OFFSET -12
#define SLALOM_POLE_INNER_X 23
#define SLALOM_POLE_OUTER_X 97
#define SLALOM_POLE_NEAR_Z 241
#define SLALOM_POLE_FAR_Z 271
#define SLALOM_POLE_WALL_HEIGHT 42
#define SLALOM_NEGATIVE_Z_FAR_WALL_INDEX 145
#define SLALOM_NEGATIVE_Z_NEAR_WALL_INDEX 146
#define SLALOM_NEGATIVE_Z_INNER_WALL_INDEX 148
#define SLALOM_NEGATIVE_Z_OUTER_WALL_INDEX 147
#define SLALOM_POSITIVE_Z_FAR_WALL_INDEX 141
#define SLALOM_POSITIVE_Z_NEAR_WALL_INDEX 142
#define SLALOM_POSITIVE_Z_INNER_WALL_INDEX 143
#define SLALOM_POSITIVE_Z_OUTER_WALL_INDEX 144

extern legacy_s16 planindex;
extern legacy_s16 wallindex;
extern legacy_s16 wallHeight;
extern legacy_s16 elRdWallRelated;
extern legacy_u8 corkFlag;
extern legacy_u8 current_surf_type;
extern legacy_u8 byte_4392C;
extern legacy_s16 terrainHeight;
extern legacy_s16 elem_xCenter;
extern legacy_s16 elem_zCenter;
extern legacy_s16 wallOrientation;
extern legacy_s16 wallStartX;
extern legacy_s16 wallStartZ;
extern struct PLANE far* planptr;
extern struct PLANE far* current_planptr;
extern struct TRACK_WALL far* wallptr;
extern struct TRACKOBJECT trkObjectList[];
extern legacy_u8 subst_hillroad_track(legacy_u8 terrain,
	legacy_u8 track);

extern legacy_s16 loopSurface_ZBounds0[];
extern legacy_s16 loopSurface_ZBounds1[];
extern legacy_s16 loopSurface_maxZ;
extern legacy_s16 loopSurface_XBounds0[];
extern legacy_s16 loopSurface_XBounds1[];
extern legacy_s16 loopBase_ZBounds0[];
extern legacy_s16 loopBase_ZBounds1[];
extern legacy_s16 loopBae_InnXBounds0[];
extern legacy_s16 loopBase_InnXBounds1[];
extern legacy_s16 loopBase_OutXBounds0[];
extern legacy_s16 loopBase_OutXBounds1[];
extern legacy_s16 bkRdEntr_triang_zAdjust[];
extern legacy_s16 corkLR_negZBound[];
extern legacy_s16 corkLR_posZBound[];
extern legacy_s16 highEntrZBounds0[];
extern legacy_s16 highEntrZBounds1[];
extern legacy_s16 highEntrXInnBounds0[];
extern legacy_s16 highEntrXInnBounds1[];
extern legacy_s16 highEntrXOutBounds0[];
extern legacy_s16 highEntrXOutBounds1[];

static legacy_s16 track_interpolate(legacy_s16 position,
	legacy_s16 position0, legacy_s16 position1,
	legacy_s16 value0, legacy_s16 value1)
{
	legacy_s32 numerator;
	legacy_s32 quotient;
	legacy_s16 denominator;

	numerator = (legacy_s32)LEGACY_S16_WRAP_SUB(position, position0) *
		(legacy_s32)LEGACY_S16_WRAP_SUB(value1, value0);
	denominator = LEGACY_S16_WRAP_SUB(position1, position0);
	quotient = LEGACY_S32_DIV_OR_ZERO(numerator, (legacy_s32)denominator);
	return LEGACY_S16_WRAP_ADD(value0,
		LEGACY_S16_FROM_BITS((legacy_u16)quotient));
}

static void track_rotate_local(struct VECTOR* position,
	legacy_s16 orientation)
{
	legacy_s16 old_x;

	old_x = position->x;
	switch ((legacy_u16)orientation) {
	case ANGLE_QUARTER_TURN:
		position->x = LEGACY_S16_WRAP_NEGATE(position->z);
		position->z = old_x;
		break;
	case ANGLE_HALF_TURN:
		position->x = LEGACY_S16_WRAP_NEGATE(position->x);
		position->z = LEGACY_S16_WRAP_NEGATE(position->z);
		break;
	case ANGLE_THREE_QUARTER_TURN:
		position->x = position->z;
		position->z = LEGACY_S16_WRAP_NEGATE(old_x);
		break;
	}
}

/* A multi-tile element keeps its origin on the shared tile edge, so the
   flags decide whether the centre comes from the tile border or its middle. */
static void track_object_tile_center(legacy_u8 track_tile,
	legacy_s16 row_index, legacy_s16 column_index)
{
	legacy_u8 multi_tile;

	multi_tile = (legacy_u8)trkObjectList[track_tile].ss_multiTileFlag;
	if ((multi_tile & 1U) != 0)
		elem_zCenter = (legacy_s16)terrainpos[row_index];
	if ((multi_tile & 2U) != 0)
		elem_xCenter = (legacy_s16)trackpos2[column_index];
}

/* Rectangular buildings share one wall test: the wall the car meets is the
   side of the footprint its next position crosses. The two x tests and the
   two z tests are mutually exclusive, so their order does not matter. */
static void track_object_building_wall(const struct VECTOR* next_position,
	legacy_s16 height, legacy_s16 x_min, legacy_s16 x_max,
	legacy_s16 z_min, legacy_s16 z_max,
	legacy_s16 wall_z_min, legacy_s16 wall_z_max,
	legacy_s16 wall_x_min, legacy_s16 wall_x_max)
{
	wallHeight = height;
	if (next_position->z <= z_min)
		wallindex = wall_z_min;
	else if (next_position->z >= z_max)
		wallindex = wall_z_max;
	else if (next_position->x <= x_min)
		wallindex = wall_x_min;
	else if (next_position->x >= x_max)
		wallindex = wall_x_max;
}

/* A corner surface is the ring between two radii around the arc centre. */
static legacy_s16 track_radius_in_band(legacy_s16 x, legacy_s16 z,
	legacy_s16 low, legacy_s16 high)
{
	legacy_s16 radius;

	radius = (legacy_s16)polarRadius2D(x, z);
	return radius > low && radius < high;
}

/* Elevated and banked corners share one arc, centred on the tile corner:
   the radius is measured against the 1536-unit centre line, and the shape index
   comes from the same angle quantised into the 18 segments of the arc. */
static legacy_s16 track_arc_radius(const struct VECTOR* position)
{
	return LEGACY_S16_WRAP_SUB((legacy_s16)polarRadius2D(
		LEGACY_S16_WRAP_ADD(position->x, TRACK_ARC_CENTER_OFFSET),
		LEGACY_S16_WRAP_ADD(position->z, TRACK_ARC_CENTER_OFFSET)),
		TRACK_ARC_CENTER_RADIUS);
}

static legacy_s16 track_arc_segment(const struct VECTOR* position)
{
	legacy_s16 value;

	value = (legacy_s16)((((legacy_u16)polarAngle(
		LEGACY_S16_WRAP_ADD(position->x, TRACK_ARC_CENTER_OFFSET),
		LEGACY_S16_WRAP_ADD(position->z, TRACK_ARC_CENTER_OFFSET)) &
		ANGLE_QUARTER_MASK) * TRACK_ARC_SEGMENT_COUNT) >> 8);
	return LEGACY_S16_WRAP_NEGATE(LEGACY_S16_WRAP_SUB(value,
		TRACK_ARC_LAST_SEGMENT));
}

void build_track_object(struct VECTOR* world_position,
	struct VECTOR* next_world_position)
{
	struct VECTOR position;
	struct VECTOR next_position;
	struct TRACKOBJECT* track_object;
	struct TRACK_WALL far* wall;
	legacy_s16 wall_orientation_modifier;
	legacy_s16 element_orientation;
	legacy_s16 physical_model;
	legacy_s16 surface_type;
	legacy_s16 absolute_x;
	legacy_s16 absolute_z;
	legacy_s16 radius;
	legacy_s16 value;
	legacy_s16 value2;
	legacy_s16 value3;
	legacy_s16 effective_x;
	legacy_s16 effective_z;
	legacy_s16 angle_step;
	legacy_s16 terrain_angle;
	legacy_s16 track_column;
	legacy_s16 track_row;
	legacy_u16 index;
	legacy_u8 terrain_tile;
	legacy_u8 track_tile;

	planindex = NO_PLANE_INDEX;
	wallindex = -1;
	wallHeight = -12;
	elRdWallRelated = -1000;
	corkFlag = CORKSCREW_INACTIVE;
	current_surf_type = SURFACE_GRASS;
	byte_4392C = 1;
	wall_orientation_modifier = 0;
	element_orientation = 0;
	terrainHeight = 0;
	terrain_tile = 0;

	track_column = LEGACY_S16_SAR(world_position->x,
		TRACK_TILE_COORDINATE_SHIFT);
	track_row = LEGACY_S16_SAR(world_position->z,
		TRACK_TILE_COORDINATE_SHIFT);
	physical_model = -1;
	if (track_column >= 0 && track_column <= TRACK_GRID_LAST_INDEX &&
		track_row >= 0 && track_row <= TRACK_GRID_LAST_INDEX) {

	elem_xCenter = (legacy_s16)trackcenterpos2[track_column];
	elem_zCenter = (legacy_s16)terraincenterpos[track_row];
	terrain_tile = td15_terr_map_main[
		trackrows[track_row] + track_column];
	if (terrain_tile == 1U) {
		current_surf_type = SURFACE_WATER;
	} else if (terrain_tile >= 2U && terrain_tile <= 5U) {
		switch (terrain_tile) {
		case 2:
			terrain_angle = TERRAIN_SLOPE_2_ANGLE;
			break;
		case 3:
			terrain_angle = TERRAIN_SLOPE_3_ANGLE;
			break;
		case 4:
			terrain_angle = TERRAIN_SLOPE_4_ANGLE;
			break;
		default:
			terrain_angle = TERRAIN_SLOPE_5_ANGLE;
			break;
		}
		position.x = LEGACY_S16_WRAP_SUB(world_position->x,
			elem_xCenter);
		position.z = LEGACY_S16_WRAP_SUB(world_position->z,
			elem_zCenter);
		value = LEGACY_S16_WRAP_ADD(
			multiply_and_scale(sin_fast((legacy_u16)terrain_angle),
				position.z),
			multiply_and_scale(cos_fast((legacy_u16)terrain_angle),
				position.x));
		if (value < 0)
			current_surf_type = SURFACE_WATER;
	} else if (terrain_tile == 6U) {
		terrainHeight = (legacy_s16)hillHeightConsts[1];
	}

	track_tile = td14_elem_map_main[
		terrainrows[track_row] + track_column];
	do {
	if (track_tile == 0)
		break;
	if (track_tile == TRACK_TILE_CONTINUATION_SOUTHEAST) {
		track_tile = td14_elem_map_main[
			terrainrows[track_row + 1] + track_column - 1];
		track_object_tile_center(track_tile, track_row + 1, track_column);
	} else if (track_tile == TRACK_TILE_CONTINUATION_SOUTH) {
		track_tile = td14_elem_map_main[
			terrainrows[track_row + 1] + track_column];
		track_object_tile_center(track_tile, track_row + 1,
			track_column + 1);
	} else if (track_tile == TRACK_TILE_CONTINUATION_EAST) {
		track_tile = td14_elem_map_main[
			terrainrows[track_row] + track_column - 1];
		track_object_tile_center(track_tile, track_row, track_column);
	} else {
		track_object_tile_center(track_tile, track_row, track_column + 1);
	}

	position.x = LEGACY_S16_WRAP_SUB(world_position->x, elem_xCenter);
	position.z = LEGACY_S16_WRAP_SUB(world_position->z, elem_zCenter);
	next_position.x = LEGACY_S16_WRAP_SUB(
		next_world_position->x, elem_xCenter);
	next_position.z = LEGACY_S16_WRAP_SUB(
		next_world_position->z, elem_zCenter);
	if (track_tile != 0 && terrain_tile >= HILL_TERRAIN_FIRST &&
		terrain_tile < HILL_TERRAIN_END)
		track_tile = subst_hillroad_track(terrain_tile, track_tile);

	track_object = &trkObjectList[track_tile];
	physical_model = (legacy_s8)track_object->ss_physicalModel;
	element_orientation = (legacy_s16)track_object->ss_rotY;
	track_rotate_local(&position, element_orientation);
	track_rotate_local(&next_position, element_orientation);
	surface_type = (legacy_s8)((legacy_u8)track_object->ss_surfaceType + 1U);
	if (surface_type < 1)
		surface_type = 1;
	absolute_x = absolute_word(position.x);
	absolute_z = absolute_word(position.z);
	if (physical_model < 0 || physical_model > TRACK_PHYSICAL_MODEL_MAXIMUM)
		break;

	switch (physical_model) {
	case 0: /* Start/finish line. */
		if (state.game_inputmode == 0 && position.x > 0) {
			if (position.z < START_FINISH_FAR_Z)
				planindex = START_FINISH_FAR_PLANE_INDEX;
			else if (position.z < START_FINISH_NEAR_Z)
				planindex = START_FINISH_NEAR_PLANE_INDEX;
		}
		/* fall through */
	case 1: /* Road. */
		if (absolute_x < ROAD_HALF_WIDTH)
			current_surf_type = (legacy_u8)surface_type;
		break;

	case 12: /* Crossroad. */
		if (absolute_x < ROAD_HALF_WIDTH || absolute_z < ROAD_HALF_WIDTH)
			current_surf_type = (legacy_u8)surface_type;
		break;

	case 5: /* Left/right chicane. */
		position.x = LEGACY_S16_WRAP_NEGATE(position.x);
		/* fall through */
	case 4: /* Right/left chicane. */
		current_surf_type = (legacy_u8)surface_type;
		if (position.x > 0) {
			position.z = LEGACY_S16_WRAP_NEGATE(position.z);
			position.x = LEGACY_S16_WRAP_NEGATE(position.x);
		}
		/* fall through */
	case 3: /* Large corner. */
		if (track_radius_in_band(
				LEGACY_S16_WRAP_ADD(position.x, TRACK_ARC_CENTER_OFFSET),
				LEGACY_S16_WRAP_ADD(position.z, TRACK_ARC_CENTER_OFFSET),
				LARGE_CORNER_INNER_RADIUS, LARGE_CORNER_OUTER_RADIUS))
			current_surf_type = (legacy_u8)surface_type;
		break;

	case 6: /* Sharp split A. */
		if (absolute_x < ROAD_HALF_WIDTH) {
			current_surf_type = (legacy_u8)surface_type;
			break;
		}
		/* fall through */
	case 2: /* Sharp corner. */
		if (track_radius_in_band(
				LEGACY_S16_WRAP_ADD(position.x,
					SHARP_CORNER_CENTER_OFFSET),
				LEGACY_S16_WRAP_ADD(position.z,
					SHARP_CORNER_CENTER_OFFSET),
				SHARP_CORNER_INNER_RADIUS, SHARP_CORNER_OUTER_RADIUS))
			current_surf_type = (legacy_u8)surface_type;
		break;

	case 7: /* Sharp split B. */
		if (absolute_x < ROAD_HALF_WIDTH) {
			current_surf_type = (legacy_u8)surface_type;
			break;
		}
		if (track_radius_in_band(
				LEGACY_S16_WRAP_SUB(SHARP_CORNER_CENTER_OFFSET,
					position.x),
				LEGACY_S16_WRAP_ADD(position.z,
					SHARP_CORNER_CENTER_OFFSET),
				SHARP_CORNER_INNER_RADIUS, SHARP_CORNER_OUTER_RADIUS))
			current_surf_type = (legacy_u8)surface_type;
		break;

	case 8: /* Large split A. */
		if (position.x >= SPLIT_STRAIGHT_LANE_INNER_X &&
			position.x <= SPLIT_STRAIGHT_LANE_OUTER_X) {
			current_surf_type = (legacy_u8)surface_type;
			break;
		}
		if (track_radius_in_band(
				LEGACY_S16_WRAP_ADD(position.x, TRACK_ARC_CENTER_OFFSET),
				LEGACY_S16_WRAP_ADD(position.z, TRACK_ARC_CENTER_OFFSET),
				LARGE_CORNER_INNER_RADIUS, LARGE_CORNER_OUTER_RADIUS))
			current_surf_type = (legacy_u8)surface_type;
		break;

	case 9: /* Large split B. */
		if (position.x >= -SPLIT_STRAIGHT_LANE_OUTER_X &&
			position.x <= -SPLIT_STRAIGHT_LANE_INNER_X) {
			current_surf_type = (legacy_u8)surface_type;
			break;
		}
		if (track_radius_in_band(
				LEGACY_S16_WRAP_SUB(TRACK_ARC_CENTER_OFFSET, position.x),
				LEGACY_S16_WRAP_ADD(position.z, TRACK_ARC_CENTER_OFFSET),
				LARGE_CORNER_INNER_RADIUS, LARGE_CORNER_OUTER_RADIUS))
			current_surf_type = (legacy_u8)surface_type;
		break;

	case 10: /* Highway entrance. */
		value = absolute_word(position.x);
		index = 0;
		while (highEntrZBounds1[index] < position.z)
			index++;
		value2 = highEntrXInnBounds0[index];
		if (highEntrXInnBounds1[index] != value2) {
			value2 = track_interpolate(position.z,
				highEntrZBounds0[index], highEntrZBounds1[index],
				highEntrXInnBounds0[index],
				highEntrXInnBounds1[index]);
		}
		value3 = highEntrXOutBounds0[index];
		if (highEntrXOutBounds1[index] != value3) {
			value3 = track_interpolate(position.z,
				highEntrZBounds0[index], highEntrZBounds1[index],
				highEntrXOutBounds0[index],
				highEntrXOutBounds1[index]);
		}
		if (value > value2 && value < value3) {
			current_surf_type = (legacy_u8)surface_type;
			break;
		}
		if (position.z >= 0 && value <= ROAD_HALF_WIDTH) {
			planindex = HIGHWAY_PLANE_INDEX;
			if (position.z >= HIGHWAY_MERGE_END_Z) {
				if (next_position.x <= -ROAD_HALF_WIDTH)
					wallindex = HIGHWAY_FAR_LEFT_WALL_INDEX;
				else if (next_position.x >= ROAD_HALF_WIDTH)
					wallindex = HIGHWAY_FAR_RIGHT_WALL_INDEX;
			} else {
				wallindex = next_position.x < 0 ?
					HIGHWAY_NEAR_LEFT_WALL_INDEX :
					HIGHWAY_NEAR_RIGHT_WALL_INDEX;
			}
		}
		break;

	case 11: /* Highway. */
		if (absolute_x <= HIGHWAY_OUTER_HALF_WIDTH) {
			if (absolute_x > ROAD_HALF_WIDTH) {
				current_surf_type = (legacy_u8)surface_type;
			} else {
				planindex = HIGHWAY_PLANE_INDEX;
				if (next_position.x <= -ROAD_HALF_WIDTH)
					wallindex = HIGHWAY_FAR_LEFT_WALL_INDEX;
				else if (next_position.x >= ROAD_HALF_WIDTH)
					wallindex = HIGHWAY_FAR_RIGHT_WALL_INDEX;
			}
		}
		break;

	case 16: /* Ramp. */
		if (position.z > 0)
			byte_4392C = 0;
		else if (next_position.z >= 0)
			wallindex = RAMP_ENTRY_WALL_INDEX;
		/* fall through */

	case 17: /* Solid ramp. */
		if (physical_model == 17 && next_position.z >= ELEVATED_ROAD_END_Z)
			wallindex = ELEVATED_FORWARD_WALL_INDEX;

		if (absolute_word(next_position.x) < ROAD_HALF_WIDTH) {
			planindex = RAMP_PLANE_INDEX;
			current_surf_type = (legacy_u8)surface_type;
			if (wallindex < 0 && position.z >= 0 &&
				absolute_x >= ROAD_HALF_WIDTH) {
				wallHeight = ELEVATED_SIDE_WALL_HEIGHT;
				elRdWallRelated = ELEVATED_WALL_VERTICAL_OFFSET;
				wallindex = position.x < 0 ? ELEVATED_LEFT_WALL_INDEX :
					ELEVATED_RIGHT_WALL_INDEX;
			}
		} else if (byte_4392C != 0 && absolute_x <= ROAD_HALF_WIDTH) {
			planindex = RAMP_PLANE_INDEX;
			if (wallindex < 0) {
				wall_orientation_modifier = ANGLE_HALF_TURN;
				wallindex = position.x < 0 ? ELEVATED_LEFT_WALL_INDEX :
					ELEVATED_RIGHT_WALL_INDEX;
			}
		}
		break;

	case 18: /* Elevated road. */
	case 19: /* Elevated span. */
	case 20: /* Solid road. */
	case 22: /* Overpass. */
		if (physical_model == 22) {
			if (LEGACY_S16_WRAP_SUB(world_position->y,
				terrainHeight) <= ELEVATED_DECK_CLEARANCE) {
				if (absolute_z <= ROAD_HALF_WIDTH)
					current_surf_type = (legacy_u8)surface_type;
				break;
			}
			byte_4392C = 0;
		} else if (physical_model != 20) {
			if (LEGACY_S16_WRAP_SUB(world_position->y,
				terrainHeight) <= ELEVATED_DECK_CLEARANCE)
				break;
			byte_4392C = 0;
		}
		if (absolute_word(next_position.x) <= ROAD_HALF_WIDTH) {
			planindex = ELEVATED_PLANE_INDEX;
			current_surf_type = (legacy_u8)surface_type;
			if (byte_4392C != 0) {
				if (next_position.z >= ELEVATED_ROAD_END_Z)
					wallindex = ELEVATED_FORWARD_WALL_INDEX;
				else if (next_position.z <= -ELEVATED_ROAD_END_Z)
					wallindex = ELEVATED_REAR_WALL_INDEX;
			}
			if (absolute_x >= ROAD_HALF_WIDTH) {
				wallHeight = ELEVATED_SIDE_WALL_HEIGHT;
				wallindex = position.x < 0 ? ELEVATED_LEFT_WALL_INDEX :
					ELEVATED_RIGHT_WALL_INDEX;
			}
		} else if (byte_4392C != 0 && absolute_x <= ROAD_HALF_WIDTH) {
			planindex = ELEVATED_PLANE_INDEX;
			wallHeight = ELEVATED_SIDE_WALL_HEIGHT;
			wall_orientation_modifier = ANGLE_HALF_TURN;
			wallindex = next_position.x < 0 ? ELEVATED_LEFT_WALL_INDEX :
				ELEVATED_RIGHT_WALL_INDEX;
		}
		break;

	case 21: /* Elevated corner. */
		if (LEGACY_S16_WRAP_SUB(world_position->y,
			terrainHeight) <= ELEVATED_DECK_CLEARANCE)
			break;
		radius = track_arc_radius(&position);
		if (radius <= -ELEVATED_CORNER_OUTER_OFFSET ||
			radius >= ELEVATED_CORNER_OUTER_OFFSET)
			break;
		current_surf_type = (legacy_u8)surface_type;
		planindex = ELEVATED_PLANE_INDEX;
		byte_4392C = 0;
		if (radius >= -ELEVATED_CORNER_INNER_OFFSET &&
			radius <= ELEVATED_CORNER_INNER_OFFSET)
			break;
		value = track_arc_segment(&position);
		wallHeight = ELEVATED_SIDE_WALL_HEIGHT;
		elRdWallRelated = ELEVATED_WALL_VERTICAL_OFFSET;
		wallindex = LEGACY_S16_WRAP_ADD(value,
			radius < 0 ? ELEVATED_CORNER_INNER_WALL_BASE :
				ELEVATED_CORNER_OUTER_WALL_BASE);
		break;

	case 24: /* Banked-road entrance A. */
		value = BANKED_ENTRANCE_A_PLAN_BASE;
		value2 = 0;
		terrain_angle = BANKED_ENTRANCE_A_ANGLE;
		/* fall through */

	case 23: /* Banked-road entrance B. */
		if (physical_model == 23) {
			value = BANKED_ENTRANCE_B_PLAN_BASE;
			value2 = 1;
			terrain_angle = BANKED_ENTRANCE_B_ANGLE;
		}
		if (absolute_x > ROAD_HALF_WIDTH)
			break;
		if (value2 == 0 && next_position.x <= -ROAD_HALF_WIDTH) {
			wall_orientation_modifier = ANGLE_HALF_TURN;
			wallindex = ELEVATED_LEFT_WALL_INDEX;
		} else if (value2 != 0 && next_position.x >= ROAD_HALF_WIDTH) {
			wall_orientation_modifier = ANGLE_HALF_TURN;
			wallindex = ELEVATED_RIGHT_WALL_INDEX;
		}
		current_surf_type = (legacy_u8)surface_type;
		if (position.z < -BANKED_ENTRANCE_END_Z) {
			planindex = value;
			break;
		}
		if (position.z >= BANKED_ENTRANCE_END_Z) {
			planindex = LEGACY_S16_WRAP_ADD(value, 9);
			break;
		}
		if (position.z < -BANKED_ENTRANCE_INNER_Z) {
			planindex = LEGACY_S16_WRAP_ADD(value, 1);
			index = 0;
		} else if (position.z < 0) {
			planindex = LEGACY_S16_WRAP_ADD(value, 3);
			index = 1;
		} else if (position.z < BANKED_ENTRANCE_INNER_Z) {
			planindex = LEGACY_S16_WRAP_ADD(value, 5);
			index = 2;
		} else {
			planindex = LEGACY_S16_WRAP_ADD(value, 7);
			index = 3;
		}
		value3 = LEGACY_S16_WRAP_SUB(position.z,
			bkRdEntr_triang_zAdjust[index]);
		value3 = LEGACY_S16_WRAP_ADD(
			multiply_and_scale(sin_fast((legacy_u16)terrain_angle),
				value3),
			multiply_and_scale(cos_fast((legacy_u16)terrain_angle),
				position.x));
		if (value3 > 0)
			planindex = LEGACY_S16_WRAP_ADD(planindex, 1);
		break;

	case 25: /* Banked road. */
		if (absolute_x <= ROAD_HALF_WIDTH) {
			current_surf_type = (legacy_u8)surface_type;
			planindex = BANKED_ROAD_PLANE_INDEX;
			if (next_position.x >= ROAD_HALF_WIDTH) {
				wall_orientation_modifier = ANGLE_HALF_TURN;
				wallindex = ELEVATED_RIGHT_WALL_INDEX;
			}
		}
		break;

	case 26: /* Banked corner. */
		radius = track_arc_radius(&position);
		if (radius <= -BANKED_CORNER_INNER_OFFSET ||
			radius >= BANKED_CORNER_OUTER_OFFSET)
			break;
		value = track_arc_segment(&position);
		planindex = LEGACY_S16_WRAP_ADD(value, BANKED_CORNER_PLAN_BASE);
		current_surf_type = (legacy_u8)surface_type;
		if (radius > BANKED_CORNER_WALL_THRESHOLD) {
			wall_orientation_modifier = ANGLE_HALF_TURN;
			wallindex = LEGACY_S16_WRAP_ADD(value, BANKED_CORNER_WALL_BASE);
			byte_4392C = 0;
		}
		break;

	case 27: /* Loop. */
		if (position.z < 0) {
			value = LOOP_REAR_PLAN_BASE;
			effective_x = LEGACY_S16_WRAP_NEGATE(position.x);
			effective_z = LEGACY_S16_WRAP_NEGATE(position.z);
		} else {
			value = LOOP_FRONT_PLAN_BASE;
			effective_x = position.x;
			effective_z = position.z;
		}
		if (effective_z <=
			LEGACY_S16_WRAP_ADD(loopSurface_maxZ, LOOP_SURFACE_END_PADDING)) {
			if (effective_z > LEGACY_S16_WRAP_SUB(loopSurface_maxZ, 1))
				value2 = LEGACY_S16_WRAP_SUB(loopSurface_maxZ, 1);
			else
				value2 = effective_z;
			index = 0;
			while (loopSurface_ZBounds1[index] < value2)
				index++;
			if (LEGACY_S16_WRAP_SUB(world_position->y,
				terrainHeight) > LOOP_UPPER_HEIGHT_THRESHOLD) {
				index = (legacy_u16)(LOOP_SURFACE_LAST_INDEX - index);
				if (effective_x < loopSurface_XBounds0[index] ||
					effective_x > LEGACY_S16_WRAP_ADD(
						loopSurface_XBounds1[index], LOOP_LANE_SEPARATION))
					break;
				if (effective_x <= loopSurface_XBounds1[index] ||
					effective_x >= LEGACY_S16_WRAP_ADD(
						loopSurface_XBounds0[index], LOOP_LANE_SEPARATION)) {
					value3 = track_interpolate(value2,
						loopSurface_ZBounds0[index],
						loopSurface_ZBounds1[index],
						loopSurface_XBounds0[index],
						loopSurface_XBounds1[index]);
					if (effective_x <= value3 || effective_x >=
						LEGACY_S16_WRAP_ADD(value3, LOOP_LANE_SEPARATION))
						break;
				}
				planindex = LEGACY_S16_WRAP_ADD(
					value, (legacy_s16)index);
				current_surf_type = (legacy_u8)surface_type;
				byte_4392C = 0;
				break;
			}
			if (!((index > LOOP_LOW_CLEARANCE_LAST_SEGMENT &&
				LEGACY_S16_WRAP_SUB(world_position->y,
					terrainHeight) < LOOP_LOW_CLEARANCE) ||
				effective_x < loopSurface_XBounds0[index] ||
				effective_x > LEGACY_S16_WRAP_ADD(
					loopSurface_XBounds1[index], LOOP_LANE_SEPARATION))) {
				if (effective_x > loopSurface_XBounds1[index] &&
					effective_x < LEGACY_S16_WRAP_ADD(
						loopSurface_XBounds0[index], LOOP_LANE_SEPARATION)) {
					planindex = LEGACY_S16_WRAP_ADD(
						value, (legacy_s16)index);
					current_surf_type = (legacy_u8)surface_type;
					byte_4392C = 0;
					break;
				}
				if (loopSurface_XBounds0[index] !=
					loopSurface_XBounds1[index]) {
					value3 = track_interpolate(value2,
						loopSurface_ZBounds0[index],
						loopSurface_ZBounds1[index],
						loopSurface_XBounds0[index],
						loopSurface_XBounds1[index]);
					if (effective_x > value3 && effective_x <
						LEGACY_S16_WRAP_ADD(value3, LOOP_LANE_SEPARATION)) {
						planindex = LEGACY_S16_WRAP_ADD(
							value, (legacy_s16)index);
						current_surf_type = (legacy_u8)surface_type;
						byte_4392C = 0;
						break;
					}
				}
			}
		}

		index = 0;
		while (loopBase_ZBounds1[index] < effective_z)
			index++;
		value2 = track_interpolate(effective_z,
			loopBase_ZBounds0[index], loopBase_ZBounds1[index],
			loopBae_InnXBounds0[index], loopBase_InnXBounds1[index]);
		value3 = track_interpolate(effective_z,
			loopBase_ZBounds0[index], loopBase_ZBounds1[index],
			loopBase_OutXBounds0[index], loopBase_OutXBounds1[index]);
		if (effective_x >= value2 && effective_x <= value3)
			current_surf_type = (legacy_u8)surface_type;
		break;

	case 28: /* Tunnel. */
		if (LEGACY_S16_WRAP_SUB(world_position->y,
			terrainHeight) >= TUNNEL_HEIGHT ||
			LEGACY_S16_WRAP_SUB(next_world_position->y,
				terrainHeight) >= TUNNEL_HEIGHT) {
			if (absolute_x < TUNNEL_OUTER_HALF_WIDTH) {
				current_surf_type = (legacy_u8)surface_type;
				planindex = TUNNEL_ROOF_PLANE_INDEX;
			}
			break;
		}
		if (absolute_x < ROAD_HALF_WIDTH)
			current_surf_type = (legacy_u8)surface_type;
		if (position.x >= ROAD_HALF_WIDTH &&
			position.x <= TUNNEL_OUTER_HALF_WIDTH) {
			wallHeight = TUNNEL_HEIGHT;
			if (next_position.z <= -TUNNEL_END_Z)
				wallindex = TUNNEL_REAR_WALL_INDEX;
			else if (next_position.z >= TUNNEL_END_Z)
				wallindex = TUNNEL_FORWARD_WALL_INDEX;
			else if (next_position.x <= ROAD_HALF_WIDTH)
				wallindex = TUNNEL_RIGHT_INNER_WALL_INDEX;
			else if (next_position.x >= TUNNEL_OUTER_HALF_WIDTH)
				wallindex = TUNNEL_RIGHT_OUTER_WALL_INDEX;
		} else if (position.x <= -ROAD_HALF_WIDTH &&
			position.x >= -TUNNEL_OUTER_HALF_WIDTH) {
			wallHeight = TUNNEL_HEIGHT;
			if (next_position.z <= -TUNNEL_END_Z)
				wallindex = TUNNEL_REAR_WALL_INDEX;
			else if (next_position.z >= TUNNEL_END_Z)
				wallindex = TUNNEL_FORWARD_WALL_INDEX;
			else if (next_position.x >= -ROAD_HALF_WIDTH)
				wallindex = TUNNEL_LEFT_INNER_WALL_INDEX;
			else if (next_position.x <= -TUNNEL_OUTER_HALF_WIDTH)
				wallindex = TUNNEL_LEFT_OUTER_WALL_INDEX;
		}
		break;

	case 29: /* Pipe entrance. */
		if (absolute_word(next_position.x) >= PIPE_ENTRANCE_WALL_INNER_X &&
			absolute_x <= PIPE_HALF_WIDTH) {
			wallHeight = PIPE_WALL_HEIGHT;
			wallindex = next_position.x <= 0 ? PIPE_ENTRANCE_LEFT_WALL_INDEX :
				PIPE_ENTRANCE_RIGHT_WALL_INDEX;
			break;
		}
		if (absolute_x >= PIPE_ENTRANCE_WALL_INNER_X ||
			LEGACY_S16_WRAP_SUB(world_position->y,
				terrainHeight) >= PIPE_ENTRANCE_MAX_HEIGHT)
			break;
		current_surf_type = (legacy_u8)surface_type;
		if (absolute_x < PIPE_ENTRANCE_CENTER_HALF_WIDTH) {
			planindex = PIPE_ENTRANCE_CENTER_PLAN_INDEX;
			break;
		}
		if (position.x < -PIPE_ENTRANCE_SIDE_SPLIT_X) {
			planindex = PIPE_ENTRANCE_LEFT_OUTER_PLAN_BASE;
			value = -PIPE_ENTRANCE_OUTER_CENTER_X;
			terrain_angle = -PIPE_ENTRANCE_OUTER_ANGLE;
		} else if (position.x < 0) {
			planindex = PIPE_ENTRANCE_LEFT_INNER_PLAN_BASE;
			value = -PIPE_ENTRANCE_INNER_CENTER_X;
			terrain_angle = -PIPE_ENTRANCE_INNER_ANGLE;
		} else if (position.x > PIPE_ENTRANCE_SIDE_SPLIT_X) {
			planindex = PIPE_ENTRANCE_RIGHT_OUTER_PLAN_BASE;
			value = PIPE_ENTRANCE_OUTER_CENTER_X;
			terrain_angle = PIPE_ENTRANCE_OUTER_ANGLE;
		} else {
			planindex = PIPE_ENTRANCE_RIGHT_INNER_PLAN_BASE;
			value = PIPE_ENTRANCE_INNER_CENTER_X;
			terrain_angle = PIPE_ENTRANCE_INNER_ANGLE;
		}
		value2 = LEGACY_S16_WRAP_ADD(
			multiply_and_scale(sin_fast((legacy_u16)terrain_angle),
				position.z),
			multiply_and_scale(cos_fast((legacy_u16)terrain_angle),
				LEGACY_S16_WRAP_SUB(position.x, value)));
		if (value2 < 0)
			planindex = LEGACY_S16_WRAP_ADD(planindex, 1);
		break;

	case 31: /* Half-pipe. */
		value = PIPE_HALF;
		/* fall through */

	case 30: /* Pipe. */
		if (physical_model == 30)
			value = PIPE_FULL;
		if (absolute_word(next_position.x) >= PIPE_HALF_WIDTH &&
			absolute_x <= PIPE_HALF_WIDTH) {
			wallHeight = PIPE_WALL_HEIGHT;
			wallindex = next_position.x <= 0 ? PIPE_LEFT_WALL_INDEX :
				PIPE_RIGHT_WALL_INDEX;
			break;
		}
		if (absolute_x >= PIPE_HALF_WIDTH ||
			LEGACY_S16_WRAP_SUB(world_position->y,
				terrainHeight) >= PIPE_MAX_HEIGHT)
			break;
		if (absolute_x < PIPE_SURFACE_HALF_WIDTH)
			current_surf_type = (legacy_u8)surface_type;
		value2 = LEGACY_S16_WRAP_SUB(world_position->y, terrainHeight) >
			PIPE_WALL_HEIGHT ? PIPE_UPPER_HALF : PIPE_LOWER_HALF;
		if (value == PIPE_HALF && value2 == PIPE_LOWER_HALF &&
			absolute_x <= HALF_PIPE_FLOOR_HALF_WIDTH &&
			absolute_z <= HALF_PIPE_FLOOR_HALF_LENGTH) {
			planindex = HALF_PIPE_FLOOR_PLAN_INDEX;
			if (next_position.z <= -HALF_PIPE_FLOOR_HALF_LENGTH)
				wallindex = HALF_PIPE_REAR_WALL_INDEX;
			else if (next_position.z >= HALF_PIPE_FLOOR_HALF_LENGTH)
				wallindex = HALF_PIPE_FORWARD_WALL_INDEX;
			break;
		}
		if (LEGACY_S16_WRAP_SUB(world_position->y,
			terrainHeight) > PIPE_SIDE_HEIGHT_SPLIT &&
			value2 == PIPE_LOWER_HALF) {
			planindex = position.x < 0 ? PIPE_UPPER_LEFT_SIDE_PLAN_INDEX :
				PIPE_UPPER_RIGHT_SIDE_PLAN_INDEX;
		} else if (absolute_x < PIPE_ENTRANCE_CENTER_HALF_WIDTH) {
			planindex = value2 != PIPE_LOWER_HALF ? PIPE_UPPER_CENTER_PLAN_INDEX :
				PIPE_LOWER_CENTER_PLAN_INDEX;
		} else if (position.x < -PIPE_ENTRANCE_SIDE_SPLIT_X) {
			planindex = value2 != PIPE_LOWER_HALF ?
				PIPE_UPPER_LEFT_OUTER_PLAN_INDEX : PIPE_LOWER_LEFT_OUTER_PLAN_INDEX;
		} else if (position.x < 0) {
			planindex = value2 != PIPE_LOWER_HALF ?
				PIPE_UPPER_LEFT_INNER_PLAN_INDEX : PIPE_LOWER_LEFT_INNER_PLAN_INDEX;
		} else if (position.x > PIPE_ENTRANCE_SIDE_SPLIT_X) {
			planindex = value2 != PIPE_LOWER_HALF ?
				PIPE_UPPER_RIGHT_OUTER_PLAN_INDEX : PIPE_LOWER_RIGHT_OUTER_PLAN_INDEX;
		} else {
			planindex = value2 != PIPE_LOWER_HALF ?
				PIPE_UPPER_RIGHT_INNER_PLAN_INDEX : PIPE_LOWER_RIGHT_INNER_PLAN_INDEX;
		}
		break;

	case 35: /* Left/right corkscrew. */
		if (absolute_x >= CORK_LR_HALF_WIDTH ||
			LEGACY_S16_WRAP_SUB(world_position->y,
				terrainHeight) >= CORK_LR_MAX_HEIGHT)
			break;
		current_surf_type = (legacy_u8)surface_type;
		value2 = LEGACY_S16_WRAP_SUB(world_position->y, terrainHeight) >
			CORK_LR_UPPER_HEIGHT ? CORK_LR_UPPER_HALF : CORK_LR_LOWER_HALF;
		value3 = CORK_LR_NO_SEGMENT;
		if (LEGACY_S16_WRAP_SUB(world_position->y,
			terrainHeight) > CORK_LR_SIDE_HEIGHT_SPLIT &&
			value2 == CORK_LR_LOWER_HALF) {
			value3 = position.x < 0 ? CORK_LR_LOWER_LEFT_SIDE_SEGMENT :
				CORK_LR_LOWER_RIGHT_SIDE_SEGMENT;
		} else if (absolute_x < CORK_LR_CENTER_HALF_WIDTH) {
			if (value2 != CORK_LR_LOWER_HALF)
				value3 = CORK_LR_UPPER_CENTER_SEGMENT;
		} else if (position.x < -CORK_LR_SIDE_SPLIT_X) {
			value3 = value2 != CORK_LR_LOWER_HALF ?
				CORK_LR_UPPER_LEFT_OUTER_SEGMENT :
				CORK_LR_LOWER_LEFT_OUTER_SEGMENT;
		} else if (position.x < 0) {
			value3 = value2 != CORK_LR_LOWER_HALF ?
				CORK_LR_UPPER_LEFT_INNER_SEGMENT :
				CORK_LR_LOWER_LEFT_INNER_SEGMENT;
		} else if (position.x > CORK_LR_SIDE_SPLIT_X) {
			value3 = value2 != CORK_LR_LOWER_HALF ?
				CORK_LR_UPPER_RIGHT_OUTER_SEGMENT :
				CORK_LR_LOWER_RIGHT_OUTER_SEGMENT;
		} else {
			value3 = value2 != CORK_LR_LOWER_HALF ?
				CORK_LR_UPPER_RIGHT_INNER_SEGMENT :
				CORK_LR_LOWER_RIGHT_INNER_SEGMENT;
		}
		if (value3 != CORK_LR_NO_SEGMENT &&
			position.z > corkLR_negZBound[value3] &&
			position.z < corkLR_posZBound[value3])
			planindex = LEGACY_S16_WRAP_ADD(value3, CORK_LR_PLAN_BASE);
		if (planindex == NO_PLANE_INDEX && absolute_z < CORK_LR_END_Z) {
			wallindex = CORK_LR_WALL_INDEX;
			corkFlag = CORKSCREW_ACTIVE;
			wallHeight = CORK_LR_WALL_HEIGHT;
		}
		break;

	case 32: /* Up/down corkscrew A. */
		value = LEGACY_S16_WRAP_NEGATE(position.x);
		value2 = CORK_UD_A_PLAN_BASE;
		value3 = CORK_UD_A_OUTER_WALL_BASE;
		terrain_angle = CORK_UD_A_INNER_WALL_BASE;
		/* fall through */

	case 33: /* Up/down corkscrew B. */
		if (physical_model == 33) {
			value = position.x;
			value2 = CORK_UD_B_PLAN_BASE;
			value3 = CORK_UD_B_OUTER_WALL_BASE;
			terrain_angle = CORK_UD_B_INNER_WALL_BASE;
		}
		corkFlag = CORKSCREW_ACTIVE;
		if (position.z < 0 &&
			LEGACY_S16_WRAP_SUB(world_position->y,
				terrainHeight) < CORK_UD_LOW_HEIGHT && value > 0) {
			if (value >= CORK_UD_LOWER_OUTER_RADIUS ||
				value <= CORK_UD_LOWER_INNER_RADIUS)
				break;
			current_surf_type = (legacy_u8)surface_type;
			planindex = value2;
			break;
		}
		if (position.z > 0 &&
			LEGACY_S16_WRAP_SUB(world_position->y,
				terrainHeight) > CORK_UD_UPPER_HEIGHT &&
			value < CORK_UD_OUTER_RADIUS && value > CORK_UD_INNER_RADIUS) {
			wallHeight = CORK_UD_WALL_HEIGHT;
			elRdWallRelated = ELEVATED_WALL_VERTICAL_OFFSET;
			wallindex = LEGACY_S16_WRAP_ADD(
				value > CORK_UD_CENTER_RADIUS ? value3 : terrain_angle,
				CORK_UD_WALL_INDEX_OFFSET);
			current_surf_type = (legacy_u8)surface_type;
			planindex = LEGACY_S16_WRAP_ADD(value2, CORK_UD_UPPER_PLAN_OFFSET);
			byte_4392C = 0;
			break;
		}
		radius = (legacy_s16)polarRadius2D(value, position.z);
		if (radius <= CORK_UD_INNER_RADIUS || radius >= CORK_UD_OUTER_RADIUS)
			break;
		angle_step = (legacy_s16)((((legacy_u16)
			LEGACY_S16_WRAP_NEGATE(LEGACY_S16_WRAP_SUB(
				(legacy_s16)polarAngle(value, position.z),
				ANGLE_QUARTER_TURN)) & ANGLE_MASK) *
			CORK_UD_ARC_SEGMENT_COUNT) >> CORK_UD_ANGLE_SCALE_SHIFT);
		planindex = LEGACY_S16_WRAP_ADD(
			LEGACY_S16_WRAP_ADD(value2, angle_step),
			CORK_UD_FIRST_ARC_PLAN_OFFSET);
		current_surf_type = (legacy_u8)surface_type;
		byte_4392C = 0;
		wallHeight = CORK_UD_WALL_HEIGHT;
		elRdWallRelated = ELEVATED_WALL_VERTICAL_OFFSET;
		value = LEGACY_S16_WRAP_SUB(radius, CORK_UD_CENTER_RADIUS);
		if (value > CORK_UD_WALL_RADIUS_OFFSET)
			wallindex = LEGACY_S16_WRAP_ADD(value3, angle_step);
		else if (value < -CORK_UD_WALL_RADIUS_OFFSET)
			wallindex = LEGACY_S16_WRAP_ADD(terrain_angle, angle_step);
		break;

	case 34: /* Slalom. */
		if (absolute_x < ROAD_HALF_WIDTH)
			current_surf_type = (legacy_u8)surface_type;
		if (position.x >= SLALOM_POLE_INNER_X &&
			position.x <= SLALOM_POLE_OUTER_X &&
			position.z > -SLALOM_POLE_FAR_Z &&
			position.z < -SLALOM_POLE_NEAR_Z) {
			wallHeight = SLALOM_POLE_WALL_HEIGHT;
			if (next_position.z < -SLALOM_POLE_FAR_Z)
				wallindex = SLALOM_NEGATIVE_Z_FAR_WALL_INDEX;
			else if (next_position.z > -SLALOM_POLE_NEAR_Z)
				wallindex = SLALOM_NEGATIVE_Z_NEAR_WALL_INDEX;
			else if (next_position.x < SLALOM_POLE_INNER_X)
				wallindex = SLALOM_NEGATIVE_Z_INNER_WALL_INDEX;
			else if (next_position.x > SLALOM_POLE_OUTER_X)
				wallindex = SLALOM_NEGATIVE_Z_OUTER_WALL_INDEX;
		} else if (position.x <= -SLALOM_POLE_INNER_X &&
			position.x >= -SLALOM_POLE_OUTER_X &&
			position.z < SLALOM_POLE_FAR_Z &&
			position.z > SLALOM_POLE_NEAR_Z) {
			wallHeight = SLALOM_POLE_WALL_HEIGHT;
			if (next_position.z > SLALOM_POLE_FAR_Z)
				wallindex = SLALOM_POSITIVE_Z_FAR_WALL_INDEX;
			else if (next_position.z < SLALOM_POLE_NEAR_Z)
				wallindex = SLALOM_POSITIVE_Z_NEAR_WALL_INDEX;
			else if (next_position.x > -SLALOM_POLE_INNER_X)
				wallindex = SLALOM_POSITIVE_Z_INNER_WALL_INDEX;
			else if (next_position.x < -SLALOM_POLE_OUTER_X)
				wallindex = SLALOM_POSITIVE_Z_OUTER_WALL_INDEX;
		}
		break;

	case 65: /* Barn. */
		if (absolute_x <= 0x96 && absolute_z <= 0x96)
			track_object_building_wall(&next_position, 0x1A9,
				-0x96, 0x96, -0x96, 0x96,
				0xA1, 0xA2, 0xA4, 0xA3);
		break;

	case 66: /* Gas station. */
		if (position.x >= -0xC8 && position.x <= 0x104 &&
			absolute_z <= 0x50)
			track_object_building_wall(&next_position, 0xE6,
				-0xC8, 0x104, -0x50, 0x50,
				0xA5, 0xA8, 0xA6, 0xA7);
		break;

	case 67: /* Joe's. */
		if (absolute_x <= 0xB4 && absolute_z <= 0x64)
			track_object_building_wall(&next_position, 0xF8,
				-0xB4, 0xB4, -0x64, 0x64,
				0xA9, 0xAC, 0xAB, 0xAA);
		break;

	case 68: /* Office. */
		if (absolute_x <= 0xC8 && absolute_z <= 0xC8)
			track_object_building_wall(&next_position, 0x226,
				-0xC8, 0xC8, -0xC8, 0xC8,
				0xAD, 0xAE, 0xAF, 0xB0);
		break;

	case 69: /* Windmill. */
		if (absolute_x <= 0x72 && absolute_z <= 0x72)
			track_object_building_wall(&next_position, 0x1EF,
				-0x72, 0x72, -0x72, 0x72,
				0xB4, 0xB2, 0xB1, 0xB3);
		break;

	case 70: /* Ship. */
		if (position.x >= -0xAA && position.x <= 0x104 &&
			absolute_z <= 0x6E)
			track_object_building_wall(&next_position, 0xE6,
				-0xAA, 0x104, -0x6E, 0x6E,
				0xB5, 0xB8, 0xB7, 0xB6);
		break;
	}
	} while (0);

	if (terrain_tile >= 7U) {
		position.x = LEGACY_S16_WRAP_SUB(world_position->x,
			(legacy_s16)trackcenterpos2[track_column]);
		position.z = LEGACY_S16_WRAP_SUB(world_position->z,
			(legacy_s16)terraincenterpos[track_row]);
		if (terrain_tile <= 0x12U) {
			switch ((terrain_tile - 7U) & 3U) {
			case 0:
				element_orientation = 0;
				break;
			case 1:
				element_orientation = ANGLE_THREE_QUARTER_TURN;
				track_rotate_local(&position, ANGLE_THREE_QUARTER_TURN);
				break;
			case 2:
				element_orientation = ANGLE_HALF_TURN;
				track_rotate_local(&position, ANGLE_HALF_TURN);
				break;
			default:
				element_orientation = ANGLE_QUARTER_TURN;
				track_rotate_local(&position, ANGLE_QUARTER_TURN);
				break;
			}
		}
		if (terrain_tile <= 0x0AU) {
			if (planindex == 0)
				planindex = 3;
		} else {
			value = LEGACY_S16_WRAP_ADD(
				multiply_and_scale(sin_fast((legacy_u16)-0x80),
					position.z),
				multiply_and_scale(cos_fast((legacy_u16)-0x80),
					position.x));
			if (terrain_tile <= 0x0EU) {
				if (value < 0)
					planindex = 4;
			} else if (terrain_tile <= 0x12U) {
				if (value > 0)
					planindex = 5;
				else
					terrainHeight = 0x1C2;
			}
		}
	}
	}

	if (planindex > 0) {
		planindex = LEGACY_S16_WRAP_MUL(planindex, 4);
		switch ((legacy_u16)element_orientation) {
		case ANGLE_QUARTER_TURN:
			planindex = LEGACY_S16_WRAP_ADD(planindex, 3);
			break;
		case ANGLE_HALF_TURN:
			planindex = LEGACY_S16_WRAP_ADD(planindex, 2);
			break;
		case ANGLE_THREE_QUARTER_TURN:
			planindex = LEGACY_S16_WRAP_ADD(planindex, 1);
			break;
		}
	}
	current_planptr = &planptr[planindex];
	if (current_surf_type == SURFACE_GRASS) {
		value = LEGACY_S16_FROM_BITS(
			(legacy_u16)(world_position->z ^ world_position->x));
		terrainHeight = LEGACY_S16_WRAP_ADD(terrainHeight,
			(legacy_s16)(LEGACY_U16_SAR(value, 8U) & 1U));
	} else {
		terrainHeight = LEGACY_S16_WRAP_ADD(terrainHeight, 2);
	}

	if (wallindex < 0)
		return;
	wall = &wallptr[wallindex];
	wallOrientation = (legacy_s16)((
		LEGACY_S16_WRAP_ADD(LEGACY_S16_WRAP_ADD(
			LEGACY_S16_WRAP_NEGATE(wall->orientation),
			element_orientation), wall_orientation_modifier)) & ANGLE_MASK);
	switch ((legacy_u16)element_orientation) {
	case ANGLE_QUARTER_TURN:
		wallStartX = wall->z;
		wallStartZ = LEGACY_S16_WRAP_NEGATE(wall->x);
		break;
	case ANGLE_HALF_TURN:
		wallStartX = LEGACY_S16_WRAP_NEGATE(wall->x);
		wallStartZ = LEGACY_S16_WRAP_NEGATE(wall->z);
		break;
	case ANGLE_THREE_QUARTER_TURN:
		wallStartX = LEGACY_S16_WRAP_NEGATE(wall->z);
		wallStartZ = wall->x;
		break;
	default:
		wallStartX = wall->x;
		wallStartZ = wall->z;
		break;
	}
	wallStartX = LEGACY_S16_WRAP_ADD(wallStartX, elem_xCenter);
	wallStartZ = LEGACY_S16_WRAP_ADD(wallStartZ, elem_zCenter);
}
