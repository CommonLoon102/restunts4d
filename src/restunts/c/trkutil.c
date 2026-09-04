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

legacy_u8 subst_hillroad_track(legacy_u8 terrain, legacy_u8 track)
{
	switch (terrain) {
	case 7:
		switch (track) {
		case 4: return 0xB6;
		case 0x0E: return 0xBA;
		case 0x18: return 0xBE;
		case 0x27:
		case 0x3B:
		case 0x62: return 0xC2;
		}
		break;

	case 8:
		switch (track) {
		case 5: return 0xB7;
		case 0x0F: return 0xBB;
		case 0x19: return 0xBF;
		case 0x24:
		case 0x38:
		case 0x5F: return 0xC3;
		}
		break;

	case 9:
		switch (track) {
		case 4: return 0xB8;
		case 0x0E: return 0xBC;
		case 0x18: return 0xC0;
		case 0x26:
		case 0x3A:
		case 0x61: return 0xC4;
		}
		break;

	case 10:
		switch (track) {
		case 5: return 0xB9;
		case 0x0F: return 0xBD;
		case 0x19: return 0xC1;
		case 0x25:
		case 0x39:
		case 0x60: return 0xC5;
		}
		break;
	}

	return 0;
}

struct PLANE far plan_memres = {
	0, 0,
	{ 0, 0, 0 },
	{ 0, 0x2000, 0 },
	{ { 0, 0, 0, 0, 0, 0, 0, 0, 0 } }
};
