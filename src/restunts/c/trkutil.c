#include "externs.h"

#define MULTI_TILE_ROW_FLAG 1U
#define MULTI_TILE_COLUMN_FLAG 2U
#define ROUTE_TRACK_INDEX_SIZE 2U

#define HILLROAD_TERRAIN_FIRST 7U
#define HILLROAD_TERRAIN_LAST 10U
#define HILLROAD_TERRAIN_COUNT 4
#define HILLROAD_PIECE_VARIANT_COUNT 6
#define HILLROAD_SHAPE_COUNT 4
#define HILLROAD_DIRECT_SHAPE_COUNT 3U
#define HILLROAD_ALTERNATE_SHAPE_INDEX 3U

#define DEFAULT_PLANE_NORMAL_Y 8192

legacy_s16 track_object_base_x(const struct TRACKOBJECT* track_object,
	legacy_u8 column)
{
	if (((legacy_u8)track_object->ss_multiTileFlag &
		MULTI_TILE_COLUMN_FLAG) != 0)
		return trackpos2[column + 1U];
	return trackcenterpos2[column];
}

legacy_s16 track_object_base_z(const struct TRACKOBJECT* track_object,
	legacy_u8 row)
{
	if (((legacy_u8)track_object->ss_multiTileFlag & MULTI_TILE_ROW_FLAG) != 0)
		return trackpos[row];
	return trackcenterpos[row];
}

/* Hand the opponent its next route point: the route index table in
   trackdata3 is looked up through the car's current entry, and the route
   walker is given that track piece. */
void opponent_route_advance(legacy_s16 route_point)
{
	legacy_u16 route_table_offset;
	legacy_s16 route_track_index;

	route_table_offset = LEGACY_U16_WRAP_MUL(
		state.opponentstate.car_trackdata3_index, ROUTE_TRACK_INDEX_SIZE);
	route_track_index = LEGACY_READ_S16_LE(
		(const legacy_u8 far*)trackdata3 + route_table_offset);
	sub_18D60(route_track_index, &state.opponentstate.car_vec_unk3,
		route_point, &state.field_3F9);
}

/* Hill terrain 7..10 replaces a flat road piece with the sloped variant that
   matches the hill. Each terrain row lists the road pieces it accepts: the
   first three map one-to-one, the last three are the alternative spellings of
   one and the same piece. */
static const legacy_u8 hillroad_pieces
	[HILLROAD_TERRAIN_COUNT][HILLROAD_PIECE_VARIANT_COUNT] = {
	{ 4, 14, 24, 39, 59, 98 },
	{ 5, 15, 25, 36, 56, 95 },
	{ 4, 14, 24, 38, 58, 97 },
	{ 5, 15, 25, 37, 57, 96 }
};

static const legacy_u8 hillroad_shapes
	[HILLROAD_TERRAIN_COUNT][HILLROAD_SHAPE_COUNT] = {
	{ 182, 186, 190, 194 },
	{ 183, 187, 191, 195 },
	{ 184, 188, 192, 196 },
	{ 185, 189, 193, 197 }
};

legacy_u8 subst_hillroad_track(legacy_u8 terrain, legacy_u8 track)
{
	legacy_u16 row;
	legacy_u16 piece;

	if (terrain < HILLROAD_TERRAIN_FIRST || terrain > HILLROAD_TERRAIN_LAST)
		return 0;

	row = terrain - HILLROAD_TERRAIN_FIRST;
	for (piece = 0U; piece < HILLROAD_PIECE_VARIANT_COUNT; piece++) {
		if (hillroad_pieces[row][piece] == track)
			return hillroad_shapes[row][
				piece < HILLROAD_DIRECT_SHAPE_COUNT ?
				piece : HILLROAD_ALTERNATE_SHAPE_INDEX];
	}

	return 0;
}

struct PLANE far plan_memres = {
	0, 0,
	{ 0, 0, 0 },
	{ 0, DEFAULT_PLANE_NORMAL_Y, 0 },
	{ { 0, 0, 0, 0, 0, 0, 0, 0, 0 } }
};
