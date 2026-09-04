#include "restunts.h"
#include "legacy.h"
#include "math.h"

#define SURFACE_GRASS 4
#define SURFACE_WATER 5

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
	case 0x100U:
		position->x = LEGACY_S16_WRAP_NEGATE(position->z);
		position->z = old_x;
		break;
	case 0x200U:
		position->x = LEGACY_S16_WRAP_NEGATE(position->x);
		position->z = LEGACY_S16_WRAP_NEGATE(position->z);
		break;
	case 0x300U:
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
   the radius is measured against the 0x600 centre line, and the shape index
   comes from the same angle quantised into the 18 segments of the arc. */
static legacy_s16 track_arc_radius(const struct VECTOR* position)
{
	return LEGACY_S16_WRAP_SUB((legacy_s16)polarRadius2D(
		LEGACY_S16_WRAP_ADD(position->x, 0x400),
		LEGACY_S16_WRAP_ADD(position->z, 0x400)), 0x600);
}

static legacy_s16 track_arc_segment(const struct VECTOR* position)
{
	legacy_s16 value;

	value = (legacy_s16)((((legacy_u16)polarAngle(
		LEGACY_S16_WRAP_ADD(position->x, 0x400),
		LEGACY_S16_WRAP_ADD(position->z, 0x400)) & 0x00FFU) *
		18U) >> 8);
	return LEGACY_S16_WRAP_NEGATE(LEGACY_S16_WRAP_SUB(value, 0x11));
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

	planindex = 0;
	wallindex = -1;
	wallHeight = -12;
	elRdWallRelated = -1000;
	corkFlag = 0;
	current_surf_type = SURFACE_GRASS;
	byte_4392C = 1;
	wall_orientation_modifier = 0;
	element_orientation = 0;
	terrainHeight = 0;
	terrain_tile = 0;

	track_column = LEGACY_S16_SAR(world_position->x, 10U);
	track_row = LEGACY_S16_SAR(world_position->z, 10U);
	physical_model = -1;
	if (track_column >= 0 && track_column <= 0x1D &&
		track_row >= 0 && track_row <= 0x1D) {

	elem_xCenter = (legacy_s16)trackcenterpos2[track_column];
	elem_zCenter = (legacy_s16)terraincenterpos[track_row];
	terrain_tile = td15_terr_map_main[
		trackrows[track_row] + track_column];
	if (terrain_tile == 1U) {
		current_surf_type = SURFACE_WATER;
	} else if (terrain_tile >= 2U && terrain_tile <= 5U) {
		switch (terrain_tile) {
		case 2:
			terrain_angle = 0x80;
			break;
		case 3:
			terrain_angle = -0x280;
			break;
		case 4:
			terrain_angle = -0x180;
			break;
		default:
			terrain_angle = -0x80;
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
	if (track_tile == 0xFDU) {
		track_tile = td14_elem_map_main[
			terrainrows[track_row + 1] + track_column - 1];
		track_object_tile_center(track_tile, track_row + 1, track_column);
	} else if (track_tile == 0xFEU) {
		track_tile = td14_elem_map_main[
			terrainrows[track_row + 1] + track_column];
		track_object_tile_center(track_tile, track_row + 1,
			track_column + 1);
	} else if (track_tile == 0xFFU) {
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
	if (track_tile != 0 && terrain_tile >= 7U && terrain_tile < 0x0BU)
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
	if (physical_model < 0 || physical_model > 0x4A)
		break;

	switch (physical_model) {
	case 0: /* Start/finish line. */
		if (state.game_inputmode == 0 && position.x > 0) {
			if (position.z < -0x17C)
				planindex = 0x83;
			else if (position.z < -0x12C)
				planindex = 0x84;
		}
		/* fall through */
	case 1: /* Road. */
		if (absolute_x < 0x78)
			current_surf_type = (legacy_u8)surface_type;
		break;

	case 12: /* Crossroad. */
		if (absolute_x < 0x78 || absolute_z < 0x78)
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
				LEGACY_S16_WRAP_ADD(position.x, 0x400),
				LEGACY_S16_WRAP_ADD(position.z, 0x400), 0x588, 0x678))
			current_surf_type = (legacy_u8)surface_type;
		break;

	case 6: /* Sharp split A. */
		if (absolute_x < 0x78) {
			current_surf_type = (legacy_u8)surface_type;
			break;
		}
		/* fall through */
	case 2: /* Sharp corner. */
		if (track_radius_in_band(
				LEGACY_S16_WRAP_ADD(position.x, 0x200),
				LEGACY_S16_WRAP_ADD(position.z, 0x200), 0x188, 0x278))
			current_surf_type = (legacy_u8)surface_type;
		break;

	case 7: /* Sharp split B. */
		if (absolute_x < 0x78) {
			current_surf_type = (legacy_u8)surface_type;
			break;
		}
		if (track_radius_in_band(
				LEGACY_S16_WRAP_SUB(0x200, position.x),
				LEGACY_S16_WRAP_ADD(position.z, 0x200), 0x188, 0x278))
			current_surf_type = (legacy_u8)surface_type;
		break;

	case 8: /* Large split A. */
		if (position.x >= 0x188 && position.x <= 0x278) {
			current_surf_type = (legacy_u8)surface_type;
			break;
		}
		if (track_radius_in_band(
				LEGACY_S16_WRAP_ADD(position.x, 0x400),
				LEGACY_S16_WRAP_ADD(position.z, 0x400), 0x588, 0x678))
			current_surf_type = (legacy_u8)surface_type;
		break;

	case 9: /* Large split B. */
		if (position.x >= -0x278 && position.x <= -0x188) {
			current_surf_type = (legacy_u8)surface_type;
			break;
		}
		if (track_radius_in_band(
				LEGACY_S16_WRAP_SUB(0x400, position.x),
				LEGACY_S16_WRAP_ADD(position.z, 0x400), 0x588, 0x678))
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
		if (position.z >= 0 && value <= 0x78) {
			planindex = 1;
			if (position.z >= 0x14E) {
				if (next_position.x <= -0x78)
					wallindex = 0xBC;
				else if (next_position.x >= 0x78)
					wallindex = 0xBA;
			} else {
				wallindex = next_position.x < 0 ? 0xBD : 0xBB;
			}
		}
		break;

	case 11: /* Highway. */
		if (absolute_x <= 0x168) {
			if (absolute_x > 0x78) {
				current_surf_type = (legacy_u8)surface_type;
			} else {
				planindex = 1;
				if (next_position.x <= -0x78)
					wallindex = 0xBC;
				else if (next_position.x >= 0x78)
					wallindex = 0xBA;
			}
		}
		break;

	case 16: /* Ramp. */
		if (position.z > 0)
			byte_4392C = 0;
		else if (next_position.z >= 0)
			wallindex = 0x66;
		/* fall through */

	case 17: /* Solid ramp. */
		if (physical_model == 17 && next_position.z >= 0x1DC)
			wallindex = 0x67;

		if (absolute_word(next_position.x) < 0x78) {
			planindex = 3;
			current_surf_type = (legacy_u8)surface_type;
			if (wallindex < 0 && position.z >= 0 && absolute_x >= 0x78) {
				wallHeight = 0x2A;
				elRdWallRelated = -12;
				wallindex = position.x < 0 ? 0x64 : 0x65;
			}
		} else if (byte_4392C != 0 && absolute_x <= 0x78) {
			planindex = 3;
			if (wallindex < 0) {
				wall_orientation_modifier = 0x200;
				wallindex = position.x < 0 ? 0x64 : 0x65;
			}
		}
		break;

	case 18: /* Elevated road. */
	case 19: /* Elevated span. */
	case 20: /* Solid road. */
	case 22: /* Overpass. */
		if (physical_model == 22) {
			if (LEGACY_S16_WRAP_SUB(world_position->y,
				terrainHeight) <= 0x186) {
				if (absolute_z <= 0x78)
					current_surf_type = (legacy_u8)surface_type;
				break;
			}
			byte_4392C = 0;
		} else if (physical_model != 20) {
			if (LEGACY_S16_WRAP_SUB(world_position->y,
				terrainHeight) <= 0x186)
				break;
			byte_4392C = 0;
		}
		if (absolute_word(next_position.x) <= 0x78) {
			planindex = 2;
			current_surf_type = (legacy_u8)surface_type;
			if (byte_4392C != 0) {
				if (next_position.z >= 0x1DC)
					wallindex = 0x67;
				else if (next_position.z <= -0x1DC)
					wallindex = 0x68;
			}
			if (absolute_x >= 0x78) {
				wallHeight = 0x2A;
				wallindex = position.x < 0 ? 0x64 : 0x65;
			}
		} else if (byte_4392C != 0 && absolute_x <= 0x78) {
			planindex = 2;
			wallHeight = 0x2A;
			wall_orientation_modifier = 0x200;
			wallindex = next_position.x < 0 ? 0x64 : 0x65;
		}
		break;

	case 21: /* Elevated corner. */
		if (LEGACY_S16_WRAP_SUB(world_position->y,
			terrainHeight) <= 0x186)
			break;
		radius = track_arc_radius(&position);
		if (radius <= -0x96 || radius >= 0x96)
			break;
		current_surf_type = (legacy_u8)surface_type;
		planindex = 2;
		byte_4392C = 0;
		if (radius >= -0x6C && radius <= 0x6C)
			break;
		value = track_arc_segment(&position);
		wallHeight = 0x2A;
		elRdWallRelated = -12;
		wallindex = LEGACY_S16_WRAP_ADD(value,
			radius < 0 ? 0x69 : 0x7B);
		break;

	case 24: /* Banked-road entrance A. */
		value = 0x23;
		value2 = 0;
		terrain_angle = -0x2A0;
		/* fall through */

	case 23: /* Banked-road entrance B. */
		if (physical_model == 23) {
			value = 0x19;
			value2 = 1;
			terrain_angle = 0xA0;
		}
		if (absolute_x > 0x78)
			break;
		if (value2 == 0 && next_position.x <= -0x78) {
			wall_orientation_modifier = 0x200;
			wallindex = 0x64;
		} else if (value2 != 0 && next_position.x >= 0x78) {
			wall_orientation_modifier = 0x200;
			wallindex = 0x65;
		}
		current_surf_type = (legacy_u8)surface_type;
		if (position.z < -0x14E) {
			planindex = value;
			break;
		}
		if (position.z >= 0x14E) {
			planindex = LEGACY_S16_WRAP_ADD(value, 9);
			break;
		}
		if (position.z < -0xA8) {
			planindex = LEGACY_S16_WRAP_ADD(value, 1);
			index = 0;
		} else if (position.z < 0) {
			planindex = LEGACY_S16_WRAP_ADD(value, 3);
			index = 1;
		} else if (position.z < 0xA8) {
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
		if (absolute_x <= 0x78) {
			current_surf_type = (legacy_u8)surface_type;
			planindex = 6;
			if (next_position.x >= 0x78) {
				wall_orientation_modifier = 0x200;
				wallindex = 0x65;
			}
		}
		break;

	case 26: /* Banked corner. */
		radius = track_arc_radius(&position);
		if (radius <= -0x78 || radius >= 0x7E)
			break;
		value = track_arc_segment(&position);
		planindex = LEGACY_S16_WRAP_ADD(value, 7);
		current_surf_type = (legacy_u8)surface_type;
		if (radius > 0x66) {
			wall_orientation_modifier = 0x200;
			wallindex = LEGACY_S16_WRAP_ADD(value, 0x7B);
			byte_4392C = 0;
		}
		break;

	case 27: /* Loop. */
		if (position.z < 0) {
			value = 0x33;
			effective_x = LEGACY_S16_WRAP_NEGATE(position.x);
			effective_z = LEGACY_S16_WRAP_NEGATE(position.z);
		} else {
			value = 0x2D;
			effective_x = position.x;
			effective_z = position.z;
		}
		if (effective_z <=
			LEGACY_S16_WRAP_ADD(loopSurface_maxZ, 0x64)) {
			if (effective_z > LEGACY_S16_WRAP_SUB(loopSurface_maxZ, 1))
				value2 = LEGACY_S16_WRAP_SUB(loopSurface_maxZ, 1);
			else
				value2 = effective_z;
			index = 0;
			while (loopSurface_ZBounds1[index] < value2)
				index++;
			if (LEGACY_S16_WRAP_SUB(world_position->y,
				terrainHeight) > 0x20C) {
				index = (legacy_u16)(5U - index);
				if (effective_x < loopSurface_XBounds0[index] ||
					effective_x > LEGACY_S16_WRAP_ADD(
						loopSurface_XBounds1[index], 0x190))
					break;
				if (effective_x <= loopSurface_XBounds1[index] ||
					effective_x >= LEGACY_S16_WRAP_ADD(
						loopSurface_XBounds0[index], 0x190)) {
					value3 = track_interpolate(value2,
						loopSurface_ZBounds0[index],
						loopSurface_ZBounds1[index],
						loopSurface_XBounds0[index],
						loopSurface_XBounds1[index]);
					if (effective_x <= value3 || effective_x >=
						LEGACY_S16_WRAP_ADD(value3, 0x190))
						break;
				}
				planindex = LEGACY_S16_WRAP_ADD(
					value, (legacy_s16)index);
				current_surf_type = (legacy_u8)surface_type;
				byte_4392C = 0;
				break;
			}
			if (!((index > 1U && LEGACY_S16_WRAP_SUB(
					world_position->y, terrainHeight) < 0x64) ||
				effective_x < loopSurface_XBounds0[index] ||
				effective_x > LEGACY_S16_WRAP_ADD(
					loopSurface_XBounds1[index], 0x190))) {
				if (effective_x > loopSurface_XBounds1[index] &&
					effective_x < LEGACY_S16_WRAP_ADD(
						loopSurface_XBounds0[index], 0x190)) {
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
						LEGACY_S16_WRAP_ADD(value3, 0x190)) {
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
		if (LEGACY_S16_WRAP_SUB(world_position->y, terrainHeight) >= 0x90 ||
			LEGACY_S16_WRAP_SUB(next_world_position->y,
				terrainHeight) >= 0x90) {
			if (absolute_x < 0x10E) {
				current_surf_type = (legacy_u8)surface_type;
				planindex = 0x85;
			}
			break;
		}
		if (absolute_x < 0x78)
			current_surf_type = (legacy_u8)surface_type;
		if (position.x >= 0x78 && position.x <= 0x10E) {
			wallHeight = 0x90;
			if (next_position.z <= -0x200)
				wallindex = 0x9A;
			else if (next_position.z >= 0x200)
				wallindex = 0x99;
			else if (next_position.x <= 0x78)
				wallindex = 0x98;
			else if (next_position.x >= 0x10E)
				wallindex = 0x96;
		} else if (position.x <= -0x78 && position.x >= -0x10E) {
			wallHeight = 0x90;
			if (next_position.z <= -0x200)
				wallindex = 0x9A;
			else if (next_position.z >= 0x200)
				wallindex = 0x99;
			else if (next_position.x >= -0x78)
				wallindex = 0x97;
			else if (next_position.x <= -0x10E)
				wallindex = 0x95;
		}
		break;

	case 29: /* Pipe entrance. */
		if (absolute_word(next_position.x) >= 0x73 && absolute_x <= 0xA4) {
			wallHeight = 0x97;
			wallindex = next_position.x <= 0 ? 0xA0 : 0x9F;
			break;
		}
		if (absolute_x >= 0x73 ||
			LEGACY_S16_WRAP_SUB(world_position->y,
				terrainHeight) >= 0xAB)
			break;
		current_surf_type = (legacy_u8)surface_type;
		if (absolute_x < 0x1F) {
			planindex = 0x46;
			break;
		}
		if (position.x < -0x54) {
			planindex = 0x49;
			value = -0x64;
			terrain_angle = -5;
		} else if (position.x < 0) {
			planindex = 0x47;
			value = -0x39;
			terrain_angle = -8;
		} else if (position.x > 0x54) {
			planindex = 0x4D;
			value = 0x64;
			terrain_angle = 5;
		} else {
			planindex = 0x4B;
			value = 0x39;
			terrain_angle = 8;
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
		value = 1;
		/* fall through */

	case 30: /* Pipe. */
		if (physical_model == 30)
			value = 0;
		if (absolute_word(next_position.x) >= 0xA4 && absolute_x <= 0xA4) {
			wallHeight = 0x97;
			wallindex = next_position.x <= 0 ? 0x9C : 0x9B;
			break;
		}
		if (absolute_x >= 0xA4 ||
			LEGACY_S16_WRAP_SUB(world_position->y,
				terrainHeight) >= 0x109)
			break;
		if (absolute_x < 0x82)
			current_surf_type = (legacy_u8)surface_type;
		value2 = LEGACY_S16_WRAP_SUB(world_position->y, terrainHeight) >
			0x97 ? 1 : 0;
		if (value != 0 && value2 == 0 && absolute_x <= 0x54 &&
			absolute_z <= 0x4B) {
			planindex = 0x45;
			if (next_position.z <= -0x4B)
				wallindex = 0x9D;
			else if (next_position.z >= 0x4B)
				wallindex = 0x9E;
			break;
		}
		if (LEGACY_S16_WRAP_SUB(world_position->y,
			terrainHeight) > 0x58 && value2 == 0) {
			planindex = position.x < 0 ? 0x3C : 0x42;
		} else if (absolute_x < 0x1F) {
			planindex = value2 != 0 ? 0x3F : 0x39;
		} else if (position.x < -0x54) {
			planindex = value2 != 0 ? 0x3D : 0x3B;
		} else if (position.x < 0) {
			planindex = value2 != 0 ? 0x3E : 0x3A;
		} else if (position.x > 0x54) {
			planindex = value2 != 0 ? 0x41 : 0x43;
		} else {
			planindex = value2 != 0 ? 0x40 : 0x44;
		}
		break;

	case 35: /* Left/right corkscrew. */
		if (absolute_x >= 0x96 ||
			LEGACY_S16_WRAP_SUB(world_position->y,
				terrainHeight) >= 0x109)
			break;
		current_surf_type = (legacy_u8)surface_type;
		value2 = LEGACY_S16_WRAP_SUB(world_position->y, terrainHeight) >
			0x97 ? 1 : 0;
		value3 = 0;
		if (LEGACY_S16_WRAP_SUB(world_position->y,
			terrainHeight) > 0x58 && value2 == 0) {
			value3 = position.x < 0 ? 3 : 9;
		} else if (absolute_x < 0x1F) {
			if (value2 != 0)
				value3 = 6;
		} else if (position.x < -0x54) {
			value3 = value2 != 0 ? 4 : 2;
		} else if (position.x < 0) {
			value3 = value2 != 0 ? 5 : 1;
		} else if (position.x > 0x54) {
			value3 = value2 != 0 ? 8 : 10;
		} else {
			value3 = value2 != 0 ? 7 : 11;
		}
		if (value3 != 0 &&
			position.z > corkLR_negZBound[value3] &&
			position.z < corkLR_posZBound[value3])
			planindex = LEGACY_S16_WRAP_ADD(value3, 0x39);
		if (planindex == 0 && absolute_z < 0x200) {
			wallindex = 0xB9;
			corkFlag = 1;
			wallHeight = 0x75;
		}
		break;

	case 32: /* Up/down corkscrew A. */
		value = LEGACY_S16_WRAP_NEGATE(position.x);
		value2 = 0x4F;
		value3 = 0x32;
		terrain_angle = 0x4B;
		/* fall through */

	case 33: /* Up/down corkscrew B. */
		if (physical_model == 33) {
			value = position.x;
			value2 = 0x69;
			value3 = 0;
			terrain_angle = 0x19;
		}
		corkFlag = 1;
		if (position.z < 0 &&
			LEGACY_S16_WRAP_SUB(world_position->y,
				terrainHeight) < 0x64 && value > 0) {
			if (value >= 0x278 || value <= 0x188)
				break;
			current_surf_type = (legacy_u8)surface_type;
			planindex = value2;
			break;
		}
		if (position.z > 0 &&
			LEGACY_S16_WRAP_SUB(world_position->y,
				terrainHeight) > 0x15E &&
			value < 0x2B4 && value > 0x14C) {
			wallHeight = 0x2A;
			elRdWallRelated = -12;
			wallindex = LEGACY_S16_WRAP_ADD(
				value > 0x200 ? value3 : terrain_angle, 0x18);
			current_surf_type = (legacy_u8)surface_type;
			planindex = LEGACY_S16_WRAP_ADD(value2, 0x19);
			byte_4392C = 0;
			break;
		}
		radius = (legacy_s16)polarRadius2D(value, position.z);
		if (radius <= 0x14C || radius >= 0x2B4)
			break;
		angle_step = (legacy_s16)((((legacy_u16)
			LEGACY_S16_WRAP_NEGATE(LEGACY_S16_WRAP_SUB(
				(legacy_s16)polarAngle(value, position.z), 0x100)) &
				0x03FFU) * 24U) >> 10);
		planindex = LEGACY_S16_WRAP_ADD(
			LEGACY_S16_WRAP_ADD(value2, angle_step), 1);
		current_surf_type = (legacy_u8)surface_type;
		byte_4392C = 0;
		wallHeight = 0x2A;
		elRdWallRelated = -12;
		value = LEGACY_S16_WRAP_SUB(radius, 0x200);
		if (value > 0x5A)
			wallindex = LEGACY_S16_WRAP_ADD(value3, angle_step);
		else if (value < -0x5A)
			wallindex = LEGACY_S16_WRAP_ADD(terrain_angle, angle_step);
		break;

	case 34: /* Slalom. */
		if (absolute_x < 0x78)
			current_surf_type = (legacy_u8)surface_type;
		if (position.x >= 0x17 && position.x <= 0x61 &&
			position.z > -0x10F && position.z < -0xF1) {
			wallHeight = 0x2A;
			if (next_position.z < -0x10F)
				wallindex = 0x91;
			else if (next_position.z > -0xF1)
				wallindex = 0x92;
			else if (next_position.x < 0x17)
				wallindex = 0x94;
			else if (next_position.x > 0x61)
				wallindex = 0x93;
		} else if (position.x <= -0x17 && position.x >= -0x61 &&
			position.z < 0x10F && position.z > 0xF1) {
			wallHeight = 0x2A;
			if (next_position.z > 0x10F)
				wallindex = 0x8D;
			else if (next_position.z < 0xF1)
				wallindex = 0x8E;
			else if (next_position.x > -0x17)
				wallindex = 0x8F;
			else if (next_position.x < -0x61)
				wallindex = 0x90;
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
				element_orientation = 0x300;
				track_rotate_local(&position, 0x300);
				break;
			case 2:
				element_orientation = 0x200;
				track_rotate_local(&position, 0x200);
				break;
			default:
				element_orientation = 0x100;
				track_rotate_local(&position, 0x100);
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
		case 0x100U:
			planindex = LEGACY_S16_WRAP_ADD(planindex, 3);
			break;
		case 0x200U:
			planindex = LEGACY_S16_WRAP_ADD(planindex, 2);
			break;
		case 0x300U:
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
			element_orientation), wall_orientation_modifier)) & 0x3FF);
	switch ((legacy_u16)element_orientation) {
	case 0x100U:
		wallStartX = wall->z;
		wallStartZ = LEGACY_S16_WRAP_NEGATE(wall->x);
		break;
	case 0x200U:
		wallStartX = LEGACY_S16_WRAP_NEGATE(wall->x);
		wallStartZ = LEGACY_S16_WRAP_NEGATE(wall->z);
		break;
	case 0x300U:
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
