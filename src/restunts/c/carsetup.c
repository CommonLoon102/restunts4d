#include "externs.h"
#include "fileio.h"
#include "memmgr.h"
#include "platform.h"
#include "residue.h"

extern legacy_s8 aCarcoun[];
extern legacy_s8 aOpp1[];
extern legacy_s8 gnam_string[];
extern legacy_s8 gsna_string[];
extern legacy_s8 aNam[];
extern legacy_s8 aPath[];
extern legacy_s8 aSped[];
extern legacy_s8 unk_46464[];
extern legacy_u8 oppnentSped[];

#define CAR_FILENAME_PREFIX_LENGTH 2U
#define CAR_ID_LENGTH 4U
#define CAR_RESOURCE_EXTENSION_LENGTH 5U
#define CAR_RESOURCE_FILENAME_LENGTH 11U
#define LEGACY_RESOURCE_WORD_SIZE 2U
#define LEGACY_PARAGRAPH_SIZE 16UL
#define LEGACY_TRACKDATA_PARAGRAPHS 1728U
#define LEGACY_CVX_PARAGRAPHS 1401U
#define LEGACY_PENALTY_ALIAS_OFFSET 65534UL
#define AERO_RESISTANCE_TABLE_COUNT 64
#define OPPONENT_ROUTE_PATH_CAPACITY 902U
#define OPPONENT_ROUTE_BRANCH_CAPACITY 258U
#define OPPONENT_ROUTE_DISTANCE_CAPACITY 259U
#define OPPONENT_ROUTE_DISTANCE_LIMIT 999999UL

static void legacy_car_filename(legacy_s8* destination,
	const legacy_s8 car_id[CAR_ID_LENGTH],
	const legacy_s8 extension[CAR_RESOURCE_EXTENSION_LENGTH])
{
	legacy_u16 index;

	destination[0] = 's';
	destination[1] = 't';
	for (index = 0U; index < CAR_ID_LENGTH; index++)
		destination[index + CAR_FILENAME_PREFIX_LENGTH] = car_id[index];
	for (index = 0U; index < CAR_RESOURCE_EXTENSION_LENGTH; index++)
		destination[index + CAR_FILENAME_PREFIX_LENGTH + CAR_ID_LENGTH] =
			extension[index];
}

static legacy_u16 legacy_car_model_paragraphs(
	const legacy_s8 car_id[CAR_ID_LENGTH])
{
	legacy_s8 filename[CAR_RESOURCE_FILENAME_LENGTH];
	static const legacy_s8 compressed_extension[
		CAR_RESOURCE_EXTENSION_LENGTH] = {
		'.', 'p', '3', 's', 0
	};
	static const legacy_s8 raw_extension[CAR_RESOURCE_EXTENSION_LENGTH] = {
		'.', '3', 's', 'h', 0
	};
	legacy_u16 paragraphs;

	legacy_car_filename(filename, car_id, compressed_extension);
	paragraphs = file_decomp_paras_nofatal(filename);
	if (paragraphs != 0U)
		return paragraphs;
	legacy_car_filename(filename, car_id, raw_extension);
	return file_paras_nofatal(filename);
}

static legacy_s16 legacy_raw_resource_word(const legacy_s8* filename,
	legacy_u32 offset, legacy_s16* value)
{
	legacy_u8 bytes[LEGACY_RESOURCE_WORD_SIZE];
	legacy_u16 handle;

	handle = dos_file_open(filename, 0);
	if (handle == 0U)
		return 0;
	if (dos_file_seek(handle, (legacy_s32)offset, 0) != 0 ||
		dos_file_read(handle, bytes, LEGACY_RESOURCE_WORD_SIZE) !=
		LEGACY_RESOURCE_WORD_SIZE) {
		(void)dos_file_close(handle);
		return 0;
	}
	(void)dos_file_close(handle);
	*value = LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(bytes));
	return 1;
}

static legacy_s16 legacy_alias_from_raw_resource(
	const legacy_s8* filename, legacy_u32* cursor, legacy_s16* value)
{
	legacy_u16 paragraphs;
	legacy_u32 length;
	legacy_u32 resource_offset;

	paragraphs = file_paras_nofatal(filename);
	length = (legacy_u32)paragraphs * LEGACY_PARAGRAPH_SIZE;
	if (LEGACY_PENALTY_ALIAS_OFFSET >= *cursor &&
		LEGACY_PENALTY_ALIAS_OFFSET + 1UL < *cursor + length) {
		resource_offset = LEGACY_PENALTY_ALIAS_OFFSET - *cursor;
		return legacy_raw_resource_word(filename, resource_offset, value);
	}
	*cursor += length;
	return 0;
}

/*
 * detect_penalty in the original executable indexes td01 with route -1.
 * Its 16-bit offset wraps to FFFEh in the trakdata segment, aliasing a word
 * in resources allocated later. Reconstruct the original low-arena offsets
 * without loading the dashboard or the menu-only car shapes. Words in an
 * unmodeled compressed/render resource are treated as an invalid route; the
 * observed stock and custom resources in that position contain out-of-range
 * values, while valid aliases occur in the small raw resources handled here.
 */
void setup_legacy_penalty_route_word(void)
{
	legacy_u32 cursor;
	legacy_u16 player_model_paragraphs;
	legacy_u16 opponent_model_paragraphs;
	legacy_u16 index;
	legacy_s16 same_car;
	legacy_s16 value;

	value = (legacy_s16)LEGACY_S16_MAX;
	cursor = (legacy_u32)(LEGACY_TRACKDATA_PARAGRAPHS +
		LEGACY_CVX_PARAGRAPHS) * LEGACY_PARAGRAPH_SIZE;
	player_model_paragraphs = legacy_car_model_paragraphs(
		gameconfig.game_playercarid);
	cursor += (legacy_u32)player_model_paragraphs * LEGACY_PARAGRAPH_SIZE;

	if ((legacy_u8)gameconfig.game_opponentcarid[0] != LEGACY_U8_MAX) {
		same_car = 1;
		for (index = 0U; index < CAR_ID_LENGTH; index++) {
			if (gameconfig.game_playercarid[index] !=
				gameconfig.game_opponentcarid[index])
				same_car = 0;
		}
		if (same_car != 0) {
			cursor += (legacy_u32)LEGACY_U16_WRAP_ADD(
				player_model_paragraphs, 1U) * LEGACY_PARAGRAPH_SIZE;
		} else {
			opponent_model_paragraphs = legacy_car_model_paragraphs(
				gameconfig.game_opponentcarid);
			cursor += (legacy_u32)opponent_model_paragraphs *
				LEGACY_PARAGRAPH_SIZE;
		}
	}

	if (legacy_alias_from_raw_resource("pceng1.vce", &cursor, &value) == 0 &&
		legacy_alias_from_raw_resource("geeng.sfx", &cursor, &value) == 0)
		(void)legacy_alias_from_raw_resource("fontled.fnt", &cursor, &value);
	legacy_execution_residue.penalty_route_word = value;
}

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
		for (i = 0; i < AERO_RESISTANCE_TABLE_COUNT; i++)
			td04_aerotable_pl[i] = aero_resistance_at(
				simd_player.aero_resistance, i);
		copy_string(gnam_string, locate_shape_alt(carresptr, "gnam"));
	} else {
		(void)simd_decode(&simd_opponent, simd_resource);
		simd_opponent.aerorestable = td05_aerotable_op;
		for (i = 0; i < AERO_RESISTANCE_TABLE_COUNT; i++)
			td05_aerotable_op[i] = aero_resistance_at(
				simd_opponent.aero_resistance, i);
		copy_string(gsna_string, locate_shape_alt(carresptr, "gsna"));
	}
}

void load_opponent_data(void)
{
	legacy_u16 path[OPPONENT_ROUTE_PATH_CAPACITY];
	legacy_u16 pending_track[OPPONENT_ROUTE_BRANCH_CAPACITY];
	legacy_u16 pending_path_count[OPPONENT_ROUTE_BRANCH_CAPACITY];
	legacy_u32 pending_distance[OPPONENT_ROUTE_DISTANCE_CAPACITY];
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
	for (index = 0; index < OPPONENT_SPEED_COUNT; index++)
		oppnentSped[index] = speed_data[index];

	best_distance = OPPONENT_ROUTE_DISTANCE_LIMIT;
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
		} else if (next_track == LEGACY_U16_MAX) {
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
			if (alternate_track != LEGACY_U16_MAX) {
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
	legacy_u16 index;

	setup_legacy_penalty_route_word();

	for (index = 0; index < CAR_ID_LENGTH; index++)
		aCarcoun[index + 3U] = gameconfig.game_playercarid[index];
	car_resource = file_load_resfile(aCarcoun);
	setup_aero_trackdata(car_resource, 0);
	unload_resource(car_resource);

	if (gameconfig.game_opponenttype != 0) {
		for (index = 0; index < CAR_ID_LENGTH; index++)
			aCarcoun[index + 3U] = gameconfig.game_opponentcarid[index];
		car_resource = file_load_resfile(aCarcoun);
		setup_aero_trackdata(car_resource, 1);
		unload_resource(car_resource);
		load_opponent_data();
	}

	/* These GAME resources are collision geometry, not renderer state. */
	load_track_collision_resources();

	followOpponentFlag = 0;
	is_in_replay_copy = -1;
	return 0;
}
#endif
