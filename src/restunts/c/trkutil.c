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
