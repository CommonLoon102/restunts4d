#include "restunts.h"
#include "memmgr.h"
#include "trackdata_layout.h"

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
	trkptr += TRACKDATA_LINK_TABLE_SIZE;
	td02_penalty_related = (legacy_s16 far*)trkptr;
	trkptr += TRACKDATA_LINK_TABLE_SIZE;
	trackdata3 = trkptr;
	trkptr += TRACKDATA_LINK_TABLE_SIZE;
	td04_aerotable_pl = (legacy_s16 far*)trkptr;
	trkptr += TRACKDATA_AERO_TABLE_SIZE;
	td05_aerotable_op = (legacy_s16 far*)trkptr;
	trkptr += TRACKDATA_AERO_TABLE_SIZE;
	trackdata6 = (legacy_s16 far*)trkptr;
	trkptr += TRACKDATA_AERO_TABLE_SIZE;
	trackdata7 = (legacy_s16 far*)trkptr;
	trkptr += TRACKDATA_AERO_TABLE_SIZE;
	td08_direction_related = (legacy_s16 far*)trkptr;
	trkptr += TRACKDATA_DIRECTION_TABLE_SIZE;
	trackdata9 = (struct VECTOR far*)trkptr;
	trkptr += TRACKDATA_CAMERA_VECTOR_SIZE;
	td10_track_check_rel = (struct VECTOR far*)trkptr;
	trkptr += TRACKDATA_CHECK_VECTOR_SIZE;
	td11_highscores = trkptr;
	trkptr += TRACKDATA_HIGHSCORE_SIZE;
	trackdata12 = trkptr;
	trkptr += TRACKDATA_UNKNOWN_12_SIZE;
	td13_rpl_header = trkptr;
	trkptr += TRACKDATA_REPLAY_HEADER_SIZE;
	td14_elem_map_main = trkptr;
	trkptr += TRACKDATA_MAP_SIZE;
	td15_terr_map_main = trkptr;
	trkptr += TRACKDATA_MAP_SIZE;
	td16_rpl_buffer = trkptr;
	trkptr += TRACKDATA_REPLAY_INPUT_BUFFER_SIZE;
	td17_trk_elem_ordered = trkptr;
	trkptr += TRACKDATA_MAP_SIZE;
	trackdata18 = trkptr;
	trkptr += TRACKDATA_MAP_SIZE;
	trackdata19 = trkptr;
	trkptr += TRACKDATA_MAP_SIZE;
	td20_trk_file_appnd = trkptr;
	trkptr += TRACKDATA_TRACK_FILE_APPEND_SIZE;
	td21_col_from_path = trkptr;
	trkptr += TRACKDATA_MAP_SIZE;
	td22_row_from_path = trkptr;
	trkptr += TRACKDATA_MAP_SIZE;
	trackdata23 = trkptr;
	trkptr += TRACKDATA_OBJECT_INDEX_SIZE;
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
