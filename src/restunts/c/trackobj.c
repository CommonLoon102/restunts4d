#include "restunts.h"
#include "legacy.h"
#include "math.h"
#include "trackdata_layout.h"

#define SURFACE_GRASS 4
#define SURFACE_WATER 5
#define TRACK_ARC_CENTER_OFFSET 1024
#define TRACK_ARC_CENTER_RADIUS 1536
#define TRACK_ARC_SEGMENT_COUNT 18U
#define TRACK_ARC_LAST_SEGMENT 17
#define TRACK_ARC_ANGLE_SCALE_SHIFT 8U
#define TERRAIN_TILE_NONE 0U
#define TERRAIN_WATER_TILE 1U
#define TERRAIN_WATER_SLOPE_FIRST 2U
#define TERRAIN_WATER_SLOPE_2 2U
#define TERRAIN_WATER_SLOPE_3 3U
#define TERRAIN_WATER_SLOPE_4 4U
#define TERRAIN_WATER_SLOPE_LAST 5U
#define TERRAIN_RAISED_TILE 6U
#define TERRAIN_RAISED_HEIGHT_INDEX 1U
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
#define BARN_HALF_WIDTH 150
#define BARN_HEIGHT 425
#define BARN_REAR_WALL_INDEX 161
#define BARN_FORWARD_WALL_INDEX 162
#define BARN_LEFT_WALL_INDEX 164
#define BARN_RIGHT_WALL_INDEX 163
#define GAS_STATION_LEFT_X -200
#define GAS_STATION_RIGHT_X 260
#define GAS_STATION_HALF_LENGTH 80
#define GAS_STATION_HEIGHT 230
#define GAS_STATION_REAR_WALL_INDEX 165
#define GAS_STATION_FORWARD_WALL_INDEX 168
#define GAS_STATION_LEFT_WALL_INDEX 166
#define GAS_STATION_RIGHT_WALL_INDEX 167
#define JOES_HALF_WIDTH 180
#define JOES_HALF_LENGTH 100
#define JOES_HEIGHT 248
#define JOES_REAR_WALL_INDEX 169
#define JOES_FORWARD_WALL_INDEX 172
#define JOES_LEFT_WALL_INDEX 171
#define JOES_RIGHT_WALL_INDEX 170
#define OFFICE_HALF_WIDTH 200
#define OFFICE_HEIGHT 550
#define OFFICE_REAR_WALL_INDEX 173
#define OFFICE_FORWARD_WALL_INDEX 174
#define OFFICE_LEFT_WALL_INDEX 175
#define OFFICE_RIGHT_WALL_INDEX 176
#define WINDMILL_HALF_WIDTH 114
#define WINDMILL_HEIGHT 495
#define WINDMILL_REAR_WALL_INDEX 180
#define WINDMILL_FORWARD_WALL_INDEX 178
#define WINDMILL_LEFT_WALL_INDEX 177
#define WINDMILL_RIGHT_WALL_INDEX 179
#define SHIP_LEFT_X -170
#define SHIP_RIGHT_X 260
#define SHIP_HALF_LENGTH 110
#define SHIP_HEIGHT 230
#define SHIP_REAR_WALL_INDEX 181
#define SHIP_FORWARD_WALL_INDEX 184
#define SHIP_LEFT_WALL_INDEX 183
#define SHIP_RIGHT_WALL_INDEX 182
#define MULTI_TILE_ROW_EDGE_FLAG 1U
#define MULTI_TILE_COLUMN_EDGE_FLAG 2U
#define TRACK_TILE_EMPTY 0U
#define TRACK_SURFACE_TYPE_OFFSET 1U
#define TRACK_SURFACE_TYPE_MINIMUM 1
#define PHYSICAL_MODEL_NONE -1
#define PHYSICAL_MODEL_MINIMUM 0
#define PHYSICAL_MODEL_START_FINISH 0
#define PHYSICAL_MODEL_ROAD 1
#define PHYSICAL_MODEL_SHARP_CORNER 2
#define PHYSICAL_MODEL_LARGE_CORNER 3
#define PHYSICAL_MODEL_CHICANE_RIGHT_LEFT 4
#define PHYSICAL_MODEL_CHICANE_LEFT_RIGHT 5
#define PHYSICAL_MODEL_SHARP_SPLIT_A 6
#define PHYSICAL_MODEL_SHARP_SPLIT_B 7
#define PHYSICAL_MODEL_LARGE_SPLIT_A 8
#define PHYSICAL_MODEL_LARGE_SPLIT_B 9
#define PHYSICAL_MODEL_HIGHWAY_ENTRANCE 10
#define PHYSICAL_MODEL_HIGHWAY 11
#define PHYSICAL_MODEL_CROSSROAD 12
#define PHYSICAL_MODEL_RAMP 16
#define PHYSICAL_MODEL_SOLID_RAMP 17
#define PHYSICAL_MODEL_ELEVATED_ROAD 18
#define PHYSICAL_MODEL_ELEVATED_SPAN 19
#define PHYSICAL_MODEL_SOLID_ROAD 20
#define PHYSICAL_MODEL_ELEVATED_CORNER 21
#define PHYSICAL_MODEL_OVERPASS 22
#define PHYSICAL_MODEL_BANKED_ENTRANCE_B 23
#define PHYSICAL_MODEL_BANKED_ENTRANCE_A 24
#define PHYSICAL_MODEL_BANKED_ROAD 25
#define PHYSICAL_MODEL_BANKED_CORNER 26
#define PHYSICAL_MODEL_LOOP 27
#define PHYSICAL_MODEL_TUNNEL 28
#define PHYSICAL_MODEL_PIPE_ENTRANCE 29
#define PHYSICAL_MODEL_PIPE 30
#define PHYSICAL_MODEL_HALF_PIPE 31
#define PHYSICAL_MODEL_CORKSCREW_UP_DOWN_A 32
#define PHYSICAL_MODEL_CORKSCREW_UP_DOWN_B 33
#define PHYSICAL_MODEL_SLALOM 34
#define PHYSICAL_MODEL_CORKSCREW_LEFT_RIGHT 35
#define PHYSICAL_MODEL_BARN 65
#define PHYSICAL_MODEL_GAS_STATION 66
#define PHYSICAL_MODEL_JOES 67
#define PHYSICAL_MODEL_OFFICE 68
#define PHYSICAL_MODEL_WINDMILL 69
#define PHYSICAL_MODEL_SHIP 70
#define TRACK_COLLISION_ENABLED 1
#define TERRAIN_ORIENTED_LAST_TILE 18U
#define TERRAIN_ORIENTATION_MASK 3U
#define TERRAIN_ORIENTATION_DEFAULT_INDEX 0U
#define TERRAIN_ORIENTATION_THREE_QUARTER_INDEX 1U
#define TERRAIN_ORIENTATION_HALF_INDEX 2U
#define TERRAIN_SLOPE_DOWN_LAST_TILE 14U
#define TERRAIN_FLAT_PLANE_INDEX 3
#define TERRAIN_SLOPE_DOWN_PLANE_INDEX 4
#define TERRAIN_SLOPE_UP_PLANE_INDEX 5
#define TERRAIN_UPPER_HEIGHT 450
#define PLANE_ORIENTATION_COUNT 4
#define PLANE_QUARTER_TURN_OFFSET 3
#define PLANE_HALF_TURN_OFFSET 2
#define PLANE_THREE_QUARTER_TURN_OFFSET 1
#define GRASS_HEIGHT_HASH_SHIFT 8U
#define GRASS_HEIGHT_VARIATION_MASK 1U
#define NON_GRASS_HEIGHT_OFFSET 2

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
	if ((multi_tile & MULTI_TILE_ROW_EDGE_FLAG) != 0)
		elem_zCenter = (legacy_s16)terrainpos[row_index];
	if ((multi_tile & MULTI_TILE_COLUMN_EDGE_FLAG) != 0)
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
		ANGLE_QUARTER_MASK) * TRACK_ARC_SEGMENT_COUNT) >>
		TRACK_ARC_ANGLE_SCALE_SHIFT);
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
	byte_4392C = TRACK_COLLISION_ENABLED;
	wall_orientation_modifier = 0;
	element_orientation = 0;
	terrainHeight = 0;
	terrain_tile = TERRAIN_TILE_NONE;

	track_column = LEGACY_S16_SAR(world_position->x,
		TRACK_TILE_POSITION_SHIFT);
	track_row = LEGACY_S16_SAR(world_position->z,
		TRACK_TILE_POSITION_SHIFT);
	physical_model = PHYSICAL_MODEL_NONE;
	if (track_column >= 0 && track_column <= TRACK_GRID_LAST_INDEX &&
		track_row >= 0 && track_row <= TRACK_GRID_LAST_INDEX) {

	elem_xCenter = (legacy_s16)trackcenterpos2[track_column];
	elem_zCenter = (legacy_s16)terraincenterpos[track_row];
	terrain_tile = td15_terr_map_main[
		trackrows[track_row] + track_column];
	if (terrain_tile == TERRAIN_WATER_TILE) {
		current_surf_type = SURFACE_WATER;
	} else if (terrain_tile >= TERRAIN_WATER_SLOPE_FIRST &&
		terrain_tile <= TERRAIN_WATER_SLOPE_LAST) {
		switch (terrain_tile) {
		case TERRAIN_WATER_SLOPE_2:
			terrain_angle = TERRAIN_SLOPE_2_ANGLE;
			break;
		case TERRAIN_WATER_SLOPE_3:
			terrain_angle = TERRAIN_SLOPE_3_ANGLE;
			break;
		case TERRAIN_WATER_SLOPE_4:
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
	} else if (terrain_tile == TERRAIN_RAISED_TILE) {
		terrainHeight = (legacy_s16)hillHeightConsts[TERRAIN_RAISED_HEIGHT_INDEX];
	}

	track_tile = td14_elem_map_main[
		terrainrows[track_row] + track_column];
	do {
	if (track_tile == TRACK_TILE_EMPTY)
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
	if (track_tile != TRACK_TILE_EMPTY && terrain_tile >= HILL_TERRAIN_FIRST &&
		terrain_tile < HILL_TERRAIN_END)
		track_tile = subst_hillroad_track(terrain_tile, track_tile);

	track_object = &trkObjectList[track_tile];
	physical_model = (legacy_s8)track_object->ss_physicalModel;
	element_orientation = (legacy_s16)track_object->ss_rotY;
	track_rotate_local(&position, element_orientation);
	track_rotate_local(&next_position, element_orientation);
	surface_type = (legacy_s8)((legacy_u8)track_object->ss_surfaceType +
		TRACK_SURFACE_TYPE_OFFSET);
	if (surface_type < TRACK_SURFACE_TYPE_MINIMUM)
		surface_type = TRACK_SURFACE_TYPE_MINIMUM;
	absolute_x = absolute_word(position.x);
	absolute_z = absolute_word(position.z);
	if (physical_model < PHYSICAL_MODEL_MINIMUM ||
		physical_model > TRACK_PHYSICAL_MODEL_MAXIMUM)
		break;

	switch (physical_model) {
	case PHYSICAL_MODEL_START_FINISH:
		if (state.game_inputmode == 0 && position.x > 0) {
			if (position.z < START_FINISH_FAR_Z)
				planindex = START_FINISH_FAR_PLANE_INDEX;
			else if (position.z < START_FINISH_NEAR_Z)
				planindex = START_FINISH_NEAR_PLANE_INDEX;
		}
		/* fall through */
	case PHYSICAL_MODEL_ROAD:
		if (absolute_x < ROAD_HALF_WIDTH)
			current_surf_type = (legacy_u8)surface_type;
		break;

	case PHYSICAL_MODEL_CROSSROAD:
		if (absolute_x < ROAD_HALF_WIDTH || absolute_z < ROAD_HALF_WIDTH)
			current_surf_type = (legacy_u8)surface_type;
		break;

	case PHYSICAL_MODEL_CHICANE_LEFT_RIGHT:
		position.x = LEGACY_S16_WRAP_NEGATE(position.x);
		/* fall through */
	case PHYSICAL_MODEL_CHICANE_RIGHT_LEFT:
		current_surf_type = (legacy_u8)surface_type;
		if (position.x > 0) {
			position.z = LEGACY_S16_WRAP_NEGATE(position.z);
			position.x = LEGACY_S16_WRAP_NEGATE(position.x);
		}
		/* fall through */
	case PHYSICAL_MODEL_LARGE_CORNER:
		if (track_radius_in_band(
				LEGACY_S16_WRAP_ADD(position.x, TRACK_ARC_CENTER_OFFSET),
				LEGACY_S16_WRAP_ADD(position.z, TRACK_ARC_CENTER_OFFSET),
				LARGE_CORNER_INNER_RADIUS, LARGE_CORNER_OUTER_RADIUS))
			current_surf_type = (legacy_u8)surface_type;
		break;

	case PHYSICAL_MODEL_SHARP_SPLIT_A:
		if (absolute_x < ROAD_HALF_WIDTH) {
			current_surf_type = (legacy_u8)surface_type;
			break;
		}
		/* fall through */
	case PHYSICAL_MODEL_SHARP_CORNER:
		if (track_radius_in_band(
				LEGACY_S16_WRAP_ADD(position.x,
					SHARP_CORNER_CENTER_OFFSET),
				LEGACY_S16_WRAP_ADD(position.z,
					SHARP_CORNER_CENTER_OFFSET),
				SHARP_CORNER_INNER_RADIUS, SHARP_CORNER_OUTER_RADIUS))
			current_surf_type = (legacy_u8)surface_type;
		break;

	case PHYSICAL_MODEL_SHARP_SPLIT_B:
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

	case PHYSICAL_MODEL_LARGE_SPLIT_A:
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

	case PHYSICAL_MODEL_LARGE_SPLIT_B:
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

	case PHYSICAL_MODEL_HIGHWAY_ENTRANCE:
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

	case PHYSICAL_MODEL_HIGHWAY:
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

	case PHYSICAL_MODEL_RAMP:
		if (position.z > 0)
			byte_4392C = 0;
		else if (next_position.z >= 0)
			wallindex = RAMP_ENTRY_WALL_INDEX;
		/* fall through */

	case PHYSICAL_MODEL_SOLID_RAMP:
		if (physical_model == PHYSICAL_MODEL_SOLID_RAMP &&
			next_position.z >= ELEVATED_ROAD_END_Z)
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

	case PHYSICAL_MODEL_ELEVATED_ROAD:
	case PHYSICAL_MODEL_ELEVATED_SPAN:
	case PHYSICAL_MODEL_SOLID_ROAD:
	case PHYSICAL_MODEL_OVERPASS:
		if (physical_model == PHYSICAL_MODEL_OVERPASS) {
			if (LEGACY_S16_WRAP_SUB(world_position->y,
				terrainHeight) <= ELEVATED_DECK_CLEARANCE) {
				if (absolute_z <= ROAD_HALF_WIDTH)
					current_surf_type = (legacy_u8)surface_type;
				break;
			}
			byte_4392C = 0;
		} else if (physical_model != PHYSICAL_MODEL_SOLID_ROAD) {
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

	case PHYSICAL_MODEL_ELEVATED_CORNER:
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

	case PHYSICAL_MODEL_BANKED_ENTRANCE_A:
		value = BANKED_ENTRANCE_A_PLAN_BASE;
		value2 = 0;
		terrain_angle = BANKED_ENTRANCE_A_ANGLE;
		/* fall through */

	case PHYSICAL_MODEL_BANKED_ENTRANCE_B:
		if (physical_model == PHYSICAL_MODEL_BANKED_ENTRANCE_B) {
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

	case PHYSICAL_MODEL_BANKED_ROAD:
		if (absolute_x <= ROAD_HALF_WIDTH) {
			current_surf_type = (legacy_u8)surface_type;
			planindex = BANKED_ROAD_PLANE_INDEX;
			if (next_position.x >= ROAD_HALF_WIDTH) {
				wall_orientation_modifier = ANGLE_HALF_TURN;
				wallindex = ELEVATED_RIGHT_WALL_INDEX;
			}
		}
		break;

	case PHYSICAL_MODEL_BANKED_CORNER:
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

	case PHYSICAL_MODEL_LOOP:
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

	case PHYSICAL_MODEL_TUNNEL:
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

	case PHYSICAL_MODEL_PIPE_ENTRANCE:
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

	case PHYSICAL_MODEL_HALF_PIPE:
		value = PIPE_HALF;
		/* fall through */

	case PHYSICAL_MODEL_PIPE:
		if (physical_model == PHYSICAL_MODEL_PIPE)
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

	case PHYSICAL_MODEL_CORKSCREW_LEFT_RIGHT:
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

	case PHYSICAL_MODEL_CORKSCREW_UP_DOWN_A:
		value = LEGACY_S16_WRAP_NEGATE(position.x);
		value2 = CORK_UD_A_PLAN_BASE;
		value3 = CORK_UD_A_OUTER_WALL_BASE;
		terrain_angle = CORK_UD_A_INNER_WALL_BASE;
		/* fall through */

	case PHYSICAL_MODEL_CORKSCREW_UP_DOWN_B:
		if (physical_model == PHYSICAL_MODEL_CORKSCREW_UP_DOWN_B) {
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

	case PHYSICAL_MODEL_SLALOM:
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

	case PHYSICAL_MODEL_BARN:
		if (absolute_x <= BARN_HALF_WIDTH && absolute_z <= BARN_HALF_WIDTH)
			track_object_building_wall(&next_position, BARN_HEIGHT,
				-BARN_HALF_WIDTH, BARN_HALF_WIDTH,
				-BARN_HALF_WIDTH, BARN_HALF_WIDTH,
				BARN_REAR_WALL_INDEX, BARN_FORWARD_WALL_INDEX,
				BARN_LEFT_WALL_INDEX, BARN_RIGHT_WALL_INDEX);
		break;

	case PHYSICAL_MODEL_GAS_STATION:
		if (position.x >= GAS_STATION_LEFT_X &&
			position.x <= GAS_STATION_RIGHT_X &&
			absolute_z <= GAS_STATION_HALF_LENGTH)
			track_object_building_wall(&next_position, GAS_STATION_HEIGHT,
				GAS_STATION_LEFT_X, GAS_STATION_RIGHT_X,
				-GAS_STATION_HALF_LENGTH, GAS_STATION_HALF_LENGTH,
				GAS_STATION_REAR_WALL_INDEX, GAS_STATION_FORWARD_WALL_INDEX,
				GAS_STATION_LEFT_WALL_INDEX, GAS_STATION_RIGHT_WALL_INDEX);
		break;

	case PHYSICAL_MODEL_JOES:
		if (absolute_x <= JOES_HALF_WIDTH && absolute_z <= JOES_HALF_LENGTH)
			track_object_building_wall(&next_position, JOES_HEIGHT,
				-JOES_HALF_WIDTH, JOES_HALF_WIDTH,
				-JOES_HALF_LENGTH, JOES_HALF_LENGTH,
				JOES_REAR_WALL_INDEX, JOES_FORWARD_WALL_INDEX,
				JOES_LEFT_WALL_INDEX, JOES_RIGHT_WALL_INDEX);
		break;

	case PHYSICAL_MODEL_OFFICE:
		if (absolute_x <= OFFICE_HALF_WIDTH && absolute_z <= OFFICE_HALF_WIDTH)
			track_object_building_wall(&next_position, OFFICE_HEIGHT,
				-OFFICE_HALF_WIDTH, OFFICE_HALF_WIDTH,
				-OFFICE_HALF_WIDTH, OFFICE_HALF_WIDTH,
				OFFICE_REAR_WALL_INDEX, OFFICE_FORWARD_WALL_INDEX,
				OFFICE_LEFT_WALL_INDEX, OFFICE_RIGHT_WALL_INDEX);
		break;

	case PHYSICAL_MODEL_WINDMILL:
		if (absolute_x <= WINDMILL_HALF_WIDTH &&
			absolute_z <= WINDMILL_HALF_WIDTH)
			track_object_building_wall(&next_position, WINDMILL_HEIGHT,
				-WINDMILL_HALF_WIDTH, WINDMILL_HALF_WIDTH,
				-WINDMILL_HALF_WIDTH, WINDMILL_HALF_WIDTH,
				WINDMILL_REAR_WALL_INDEX, WINDMILL_FORWARD_WALL_INDEX,
				WINDMILL_LEFT_WALL_INDEX, WINDMILL_RIGHT_WALL_INDEX);
		break;

	case PHYSICAL_MODEL_SHIP:
		if (position.x >= SHIP_LEFT_X && position.x <= SHIP_RIGHT_X &&
			absolute_z <= SHIP_HALF_LENGTH)
			track_object_building_wall(&next_position, SHIP_HEIGHT,
				SHIP_LEFT_X, SHIP_RIGHT_X,
				-SHIP_HALF_LENGTH, SHIP_HALF_LENGTH,
				SHIP_REAR_WALL_INDEX, SHIP_FORWARD_WALL_INDEX,
				SHIP_LEFT_WALL_INDEX, SHIP_RIGHT_WALL_INDEX);
		break;
	}
	} while (0);

	if (terrain_tile >= HILL_TERRAIN_FIRST) {
		position.x = LEGACY_S16_WRAP_SUB(world_position->x,
			(legacy_s16)trackcenterpos2[track_column]);
		position.z = LEGACY_S16_WRAP_SUB(world_position->z,
			(legacy_s16)terraincenterpos[track_row]);
		if (terrain_tile <= TERRAIN_ORIENTED_LAST_TILE) {
			switch ((terrain_tile - HILL_TERRAIN_FIRST) &
				TERRAIN_ORIENTATION_MASK) {
			case TERRAIN_ORIENTATION_DEFAULT_INDEX:
				element_orientation = 0;
				break;
			case TERRAIN_ORIENTATION_THREE_QUARTER_INDEX:
				element_orientation = ANGLE_THREE_QUARTER_TURN;
				track_rotate_local(&position, ANGLE_THREE_QUARTER_TURN);
				break;
			case TERRAIN_ORIENTATION_HALF_INDEX:
				element_orientation = ANGLE_HALF_TURN;
				track_rotate_local(&position, ANGLE_HALF_TURN);
				break;
			default:
				element_orientation = ANGLE_QUARTER_TURN;
				track_rotate_local(&position, ANGLE_QUARTER_TURN);
				break;
			}
		}
		if (terrain_tile < HILL_TERRAIN_END) {
			if (planindex == NO_PLANE_INDEX)
				planindex = TERRAIN_FLAT_PLANE_INDEX;
		} else {
			value = LEGACY_S16_WRAP_ADD(
				multiply_and_scale(sin_fast(
					(legacy_u16)TERRAIN_SLOPE_5_ANGLE),
					position.z),
				multiply_and_scale(cos_fast(
					(legacy_u16)TERRAIN_SLOPE_5_ANGLE),
					position.x));
			if (terrain_tile <= TERRAIN_SLOPE_DOWN_LAST_TILE) {
				if (value < 0)
					planindex = TERRAIN_SLOPE_DOWN_PLANE_INDEX;
			} else if (terrain_tile <= TERRAIN_ORIENTED_LAST_TILE) {
				if (value > 0)
					planindex = TERRAIN_SLOPE_UP_PLANE_INDEX;
				else
					terrainHeight = TERRAIN_UPPER_HEIGHT;
			}
		}
	}
	}

	if (planindex > NO_PLANE_INDEX) {
		planindex = LEGACY_S16_WRAP_MUL(planindex, PLANE_ORIENTATION_COUNT);
		switch ((legacy_u16)element_orientation) {
		case ANGLE_QUARTER_TURN:
			planindex = LEGACY_S16_WRAP_ADD(planindex,
				PLANE_QUARTER_TURN_OFFSET);
			break;
		case ANGLE_HALF_TURN:
			planindex = LEGACY_S16_WRAP_ADD(planindex,
				PLANE_HALF_TURN_OFFSET);
			break;
		case ANGLE_THREE_QUARTER_TURN:
			planindex = LEGACY_S16_WRAP_ADD(planindex,
				PLANE_THREE_QUARTER_TURN_OFFSET);
			break;
		}
	}
	current_planptr = &planptr[planindex];
	if (current_surf_type == SURFACE_GRASS) {
		value = LEGACY_S16_FROM_BITS(
			(legacy_u16)(world_position->z ^ world_position->x));
		terrainHeight = LEGACY_S16_WRAP_ADD(terrainHeight,
			(legacy_s16)(LEGACY_U16_SAR(value, GRASS_HEIGHT_HASH_SHIFT) &
				GRASS_HEIGHT_VARIATION_MASK));
	} else {
		terrainHeight = LEGACY_S16_WRAP_ADD(terrainHeight,
			NON_GRASS_HEIGHT_OFFSET);
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
