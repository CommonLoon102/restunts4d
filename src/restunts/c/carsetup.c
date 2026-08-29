#include <dos.h>
#include "externs.h"
#include "fileio.h"
#include "memmgr.h"

extern legacy_s8 aCarcoun[];
extern legacy_s8 aOpp1[];
extern legacy_s8 gnam_string[];
extern legacy_s8 gsna_string[];
extern legacy_s8 aNam[];
extern legacy_s8 aPath[];
extern legacy_s8 aSped[];
extern legacy_s8 unk_46464[];
extern legacy_u8 oppnentSped[];

#ifdef RESTUNTS_HEADLESS
extern void far* gameresptr;
#endif

static void opponent_route_write(legacy_u16 index, legacy_u16 value)
{
	legacy_u16 offset;

	offset = LEGACY_U16_WRAP_MUL(index, 2U);
	LEGACY_WRITE_U16_LE((legacy_u8 far*)trackdata3 + offset, value);
}

static legacy_s16 aero_resistance_at(legacy_s16 resistance,
	legacy_s16 speed_index)
{
	legacy_s32 product;

	product = LEGACY_S32_WRAP_MUL(resistance, speed_index);
	product = LEGACY_S32_WRAP_MUL(product, speed_index);
	return LEGACY_S16_FROM_BITS(
		(legacy_u16)LEGACY_S32_SAR(product, 9U));
}

void setup_aero_trackdata(void far* carresptr, legacy_s16 is_opponent)
{
	legacy_s16 i;
	const legacy_u8 far* simd_resource;

	simd_resource = (const legacy_u8 far*)locate_shape_alt(
		carresptr, "simd");

	if (is_opponent == 0) {
		(void)simd_decode(&simd_player, simd_resource);
		simd_player.aerorestable = td04_aerotable_pl;
		for (i = 0; i < 0x40; i++)
			td04_aerotable_pl[i] = aero_resistance_at(
				simd_player.aero_resistance, i);
		copy_string(gnam_string, locate_shape_alt(carresptr, "gnam"));
	} else {
		(void)simd_decode(&simd_opponent, simd_resource);
		simd_opponent.aerorestable = td05_aerotable_op;
		for (i = 0; i < 0x40; i++)
			td05_aerotable_op[i] = aero_resistance_at(
				simd_opponent.aero_resistance, i);
		copy_string(gsna_string, locate_shape_alt(carresptr, "gsna"));
	}
}

void load_opponent_data(void)
{
	legacy_u16 path[902];
	legacy_u16 pending_track[258];
	legacy_u16 pending_path_count[258];
	legacy_u32 pending_distance[259];
	void far* resource;
	legacy_u8 far* speed_data;
	legacy_u32 distance;
	legacy_u32 best_distance;
	legacy_u16 track_index;
	legacy_u16 next_track;
	legacy_u16 alternate_track;
	legacy_u16 path_count;
	legacy_u16 pending_count;
	legacy_u16 index;
	legacy_u8 speed_index;
	legacy_s16 terminal;
	legacy_s16 reaches_finish;

	aOpp1[3] = (legacy_s8)((legacy_u8)gameconfig.game_opponenttype + '0');
	resource = file_load_resfile(aOpp1);
	copy_string(unk_46464,
		locate_text_res((legacy_s8 far*)resource, aNam));
	(void)locate_shape_alt((legacy_s8 far*)resource, aPath);
	speed_data = (legacy_u8 far*)locate_shape_alt(
		(legacy_s8 far*)resource, aSped);
	for (index = 0; index < 16U; index++)
		oppnentSped[index] = speed_data[index];

	best_distance = 0x000F423FUL;
	distance = 0;
	track_index = 0;
	path_count = 0;
	pending_count = 0;
	for (;;) {
		terminal = 0;
		reaches_finish = 0;
		next_track = (legacy_u16)td01_track_file_cpy[track_index];
		if (next_track == 0) {
			terminal = 1;
			reaches_finish = 1;
		} else if (next_track == 0xFFFFU) {
			terminal = 1;
		} else if (path_count != 0) {
			for (index = 0; index < path_count; index++) {
				if (path[index] == track_index) {
					terminal = 1;
					break;
				}
			}
		}

		path[path_count] = track_index;
		path_count++;
		speed_index = (legacy_u8)td17_trk_elem_ordered[track_index];
		distance += (legacy_u32)speed_data[speed_index] + 1UL;
		if (!terminal) {
			alternate_track = (legacy_u16)
				td02_penalty_related[track_index];
			if (alternate_track != 0xFFFFU) {
				pending_track[pending_count] = alternate_track;
				pending_path_count[pending_count] = path_count;
				pending_distance[pending_count] = distance;
				pending_count++;
			}
			track_index = next_track;
			continue;
		}

		if (reaches_finish && distance < best_distance) {
			path[path_count] = 0;
			path_count++;
			best_distance = distance;
			for (index = 0; index < path_count; index++)
				opponent_route_write(index, path[index]);
			opponent_route_write(path_count, 0U);
			opponent_route_write(
				LEGACY_U16_WRAP_ADD(path_count, 1U), 1U);
		}
		if (pending_count == 0)
			break;
		pending_count--;
		track_index = pending_track[pending_count];
		path_count = pending_path_count[pending_count];
		distance = pending_distance[pending_count];
	}
	unload_resource(resource);
}

#ifdef RESTUNTS_HEADLESS
legacy_s16 setup_player_cars_repldump(void)
{
	void far* car_resource;
	const legacy_u8 far* plane_resource;
	const legacy_u8 far* wall_resource;
	legacy_u16 index;

	for (index = 0; index < 4U; index++)
		aCarcoun[index + 3U] = gameconfig.game_playercarid[index];
	car_resource = file_load_resfile(aCarcoun);
	setup_aero_trackdata(car_resource, 0);
	unload_resource(car_resource);

	if (gameconfig.game_opponenttype != 0) {
		for (index = 0; index < 4U; index++)
			aCarcoun[index + 3U] = gameconfig.game_opponentcarid[index];
		car_resource = file_load_resfile(aCarcoun);
		setup_aero_trackdata(car_resource, 1);
		unload_resource(car_resource);
		load_opponent_data();
	}

	/* These GAME resources are collision geometry, not renderer state. */
	gameresptr = file_load_resfile("game");
	plane_resource = (const legacy_u8 far*)locate_shape_alt(
		gameresptr, "plan");
	wall_resource = (const legacy_u8 far*)locate_shape_alt(
		gameresptr, "wall");
	track_collision_resources_decode(plane_resource, wall_resource);

	followOpponentFlag = 0;
	is_in_replay_copy = -1;
	return 0;
}
#endif
