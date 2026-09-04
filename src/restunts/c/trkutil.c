#include "externs.h"

legacy_s16 track_object_base_x(const struct TRACKOBJECT* track_object,
	legacy_u8 column)
{
	if (((legacy_u8)track_object->ss_multiTileFlag & 2U) != 0)
		return trackpos2[column + 1U];
	return trackcenterpos2[column];
}

legacy_s16 track_object_base_z(const struct TRACKOBJECT* track_object,
	legacy_u8 row)
{
	if (((legacy_u8)track_object->ss_multiTileFlag & 1U) != 0)
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
		state.opponentstate.car_trackdata3_index, 2U);
	route_track_index = LEGACY_READ_S16_LE(
		(const legacy_u8 far*)trackdata3 + route_table_offset);
	sub_18D60(route_track_index, &state.opponentstate.car_vec_unk3,
		route_point, &state.field_3F9);
}

/* Hill terrain 7..10 replaces a flat road piece with the sloped variant that
   matches the hill. Each terrain row lists the road pieces it accepts: the
   first three map one-to-one, the last three are the alternative spellings of
   one and the same piece. */
static const legacy_u8 hillroad_pieces[4][6] = {
	{ 0x04, 0x0E, 0x18, 0x27, 0x3B, 0x62 },
	{ 0x05, 0x0F, 0x19, 0x24, 0x38, 0x5F },
	{ 0x04, 0x0E, 0x18, 0x26, 0x3A, 0x61 },
	{ 0x05, 0x0F, 0x19, 0x25, 0x39, 0x60 }
};

static const legacy_u8 hillroad_shapes[4][4] = {
	{ 0xB6, 0xBA, 0xBE, 0xC2 },
	{ 0xB7, 0xBB, 0xBF, 0xC3 },
	{ 0xB8, 0xBC, 0xC0, 0xC4 },
	{ 0xB9, 0xBD, 0xC1, 0xC5 }
};

legacy_u8 subst_hillroad_track(legacy_u8 terrain, legacy_u8 track)
{
	legacy_u16 row;
	legacy_u16 piece;

	if (terrain < 7U || terrain > 10U)
		return 0;

	row = terrain - 7U;
	for (piece = 0U; piece < 6U; piece++) {
		if (hillroad_pieces[row][piece] == track)
			return hillroad_shapes[row][piece < 3U ? piece : 3U];
	}

	return 0;
}

struct PLANE far plan_memres = {
	0, 0,
	{ 0, 0, 0 },
	{ 0, 0x2000, 0 },
	{ { 0, 0, 0, 0, 0, 0, 0, 0, 0 } }
};
