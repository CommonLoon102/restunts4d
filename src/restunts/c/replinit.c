#include "restunts.h"
#include "memmgr.h"

#define TRACK_GRID_SIZE 30
#define TRACK_GRID_LAST_INDEX 29
#define TRACK_TILE_POSITION_SHIFT 10U
#define TRACK_TILE_HALF_SIZE 512
#define TRACKDATA_ALLOCATION_SIZE 27635
#define TRACK_LINK_TABLE_BYTES 1802
#define TRACK_AERO_TABLE_BYTES 128
#define TRACK_DIRECTION_TABLE_BYTES 96
#define TRACK_CAMERA_VECTOR_BYTES 384
#define TRACK_CHECK_VECTOR_BYTES 288
#define TRACK_HIGHSCORE_BYTES 364
#define TRACKDATA12_BYTES 240
#define REPLAY_HEADER_BYTES 26
#define TRACK_MAP_BYTES 901
#define REPLAY_INPUT_BUFFER_BYTES 12000
#define TRACK_FILE_APPEND_BYTES 1964
#define TRACK_OBJECT_INDEX_BYTES 48
#define REPLAY_SLOW_CALLBACK_DIVISOR 2U

void init_video_geometry_flags(void)
{
	video_flag1_is1 = 1;
	video_flag2_is1 = 1;
	video_flag3_isFFFF = -1;
	video_flag4_is1 = 1;
}

void init_row_tables(void)
{
	legacy_s16 i;
	legacy_s16 inverse_row;
	legacy_s16 track_position;
	legacy_s16 terrain_position;

	for (i = 0; i < TRACK_GRID_SIZE; i++) {
		inverse_row = LEGACY_S16_WRAP_SUB(TRACK_GRID_LAST_INDEX, i);
		track_position = LEGACY_S16_SHL(inverse_row,
			TRACK_TILE_POSITION_SHIFT);
		terrain_position = LEGACY_S16_SHL(i, TRACK_TILE_POSITION_SHIFT);
		trackrows[i] = LEGACY_S16_WRAP_MUL(TRACK_GRID_SIZE, inverse_row);
		terrainrows[i] = LEGACY_S16_WRAP_MUL(TRACK_GRID_SIZE, i);
		trackpos[i] = track_position;
		trackpos2[i] = terrain_position;
		trackcenterpos[i] = LEGACY_S16_WRAP_ADD(
			track_position, TRACK_TILE_HALF_SIZE);
		terrainpos[i] = terrain_position;
		terraincenterpos[i] = LEGACY_S16_WRAP_ADD(
			terrain_position, TRACK_TILE_HALF_SIZE);
		trackcenterpos2[i] = LEGACY_S16_WRAP_ADD(
			terrain_position, TRACK_TILE_HALF_SIZE);
	}
}

void init_trackdata(void)
{
	legacy_s8 far* trkptr;

	trkptr = mmgr_alloc_resbytes("trakdata", TRACKDATA_ALLOCATION_SIZE);
	td01_track_file_cpy = (legacy_s16 far*)trkptr;
	trkptr += TRACK_LINK_TABLE_BYTES;
	td02_penalty_related = (legacy_s16 far*)trkptr;
	trkptr += TRACK_LINK_TABLE_BYTES;
	trackdata3 = trkptr;
	trkptr += TRACK_LINK_TABLE_BYTES;
	td04_aerotable_pl = (legacy_s16 far*)trkptr;
	trkptr += TRACK_AERO_TABLE_BYTES;
	td05_aerotable_op = (legacy_s16 far*)trkptr;
	trkptr += TRACK_AERO_TABLE_BYTES;
	trackdata6 = (legacy_s16 far*)trkptr;
	trkptr += TRACK_AERO_TABLE_BYTES;
	trackdata7 = (legacy_s16 far*)trkptr;
	trkptr += TRACK_AERO_TABLE_BYTES;
	td08_direction_related = (legacy_s16 far*)trkptr;
	trkptr += TRACK_DIRECTION_TABLE_BYTES;
	trackdata9 = (struct VECTOR far*)trkptr;
	trkptr += TRACK_CAMERA_VECTOR_BYTES;
	td10_track_check_rel = (struct VECTOR far*)trkptr;
	trkptr += TRACK_CHECK_VECTOR_BYTES;
	td11_highscores = trkptr;
	trkptr += TRACK_HIGHSCORE_BYTES;
	trackdata12 = trkptr;
	trkptr += TRACKDATA12_BYTES;
	td13_rpl_header = trkptr;
	trkptr += REPLAY_HEADER_BYTES;
	td14_elem_map_main = trkptr;
	trkptr += TRACK_MAP_BYTES;
	td15_terr_map_main = trkptr;
	trkptr += TRACK_MAP_BYTES;
	td16_rpl_buffer = trkptr;
	trkptr += REPLAY_INPUT_BUFFER_BYTES;
	td17_trk_elem_ordered = trkptr;
	trkptr += TRACK_MAP_BYTES;
	trackdata18 = trkptr;
	trkptr += TRACK_MAP_BYTES;
	trackdata19 = trkptr;
	trkptr += TRACK_MAP_BYTES;
	td20_trk_file_appnd = trkptr;
	trkptr += TRACK_FILE_APPEND_BYTES;
	td21_col_from_path = trkptr;
	trkptr += TRACK_MAP_BYTES;
	td22_row_from_path = trkptr;
	trkptr += TRACK_MAP_BYTES;
	trackdata23 = trkptr;
	trkptr += TRACK_OBJECT_INDEX_BYTES;
}

void init_unknown(void)
{
	byte_44A8A = 1;
	byte_4552F = REPLAY_SLOW_CALLBACK_DIVISOR;
	elapsed_time2 = 0;
	byte_449DA = 0;
	byte_4393C = 0;
	word_44DCA = 0;
}
