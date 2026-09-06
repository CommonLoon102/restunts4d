#include "restunts.h"
#include "memmgr.h"
#include "trackdata_layout.h"

#define REPLAY_SLOW_CALLBACK_DIVISOR 2U
#define VIDEO_GEOMETRY_UNIT_SCALE 1
#define VIDEO_GEOMETRY_FULL_MASK (-1)

void init_video_geometry_flags(void)
{
	video_shape_width_scale = VIDEO_GEOMETRY_UNIT_SCALE;
	video_x_alignment = VIDEO_GEOMETRY_UNIT_SCALE;
	video_x_alignment_mask = VIDEO_GEOMETRY_FULL_MASK;
	video_buffer_height_divisor = VIDEO_GEOMETRY_UNIT_SCALE;
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
		track_row_positions[i] = track_position;
		track_column_positions[i] = terrain_position;
		track_row_centers[i] = LEGACY_S16_WRAP_ADD(
			track_position, TRACK_TILE_HALF_SIZE);
		terrainpos[i] = terrain_position;
		terraincenterpos[i] = LEGACY_S16_WRAP_ADD(
			terrain_position, TRACK_TILE_HALF_SIZE);
		track_column_centers[i] = LEGACY_S16_WRAP_ADD(
			terrain_position, TRACK_TILE_HALF_SIZE);
	}
}

void init_trackdata(void)
{
	legacy_s8 far* trkptr;

	trkptr = mmgr_alloc_resbytes("trakdata", TRACKDATA_ALLOCATION_SIZE);
	track_primary_route_links = (legacy_s16 far*)trkptr;
	trkptr += TRACKDATA_LINK_TABLE_SIZE;
	track_alternate_route_links = (legacy_s16 far*)trkptr;
	trkptr += TRACKDATA_LINK_TABLE_SIZE;
	opponent_route_track_indices = trkptr;
	trkptr += TRACKDATA_LINK_TABLE_SIZE;
	td04_aerotable_pl = (legacy_s16 far*)trkptr;
	trkptr += TRACKDATA_AERO_TABLE_SIZE;
	td05_aerotable_op = (legacy_s16 far*)trkptr;
	trkptr += TRACKDATA_AERO_TABLE_SIZE;
	reserved_trackside_camera_words = (legacy_s16 far*)trkptr;
	trkptr += TRACKDATA_AERO_TABLE_SIZE;
	trackside_camera_ground_heights = (legacy_s16 far*)trkptr;
	trkptr += TRACKDATA_AERO_TABLE_SIZE;
	roadside_sign_headings = (legacy_s16 far*)trkptr;
	trkptr += TRACKDATA_DIRECTION_TABLE_SIZE;
	trackside_camera_positions = (struct VECTOR far*)trkptr;
	trkptr += TRACKDATA_CAMERA_VECTOR_SIZE;
	roadside_sign_positions = (struct VECTOR far*)trkptr;
	trkptr += TRACKDATA_CHECK_VECTOR_SIZE;
	td11_highscores = trkptr;
	trkptr += TRACKDATA_HIGHSCORE_SIZE;
	sprite_background_state_stack = trkptr;
	trkptr += TRACKDATA_SPRITE_STATE_STACK_SIZE;
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
	track_route_traversal_flags = trkptr;
	trkptr += TRACKDATA_MAP_SIZE;
	roadside_sign_indices_by_tile = trkptr;
	trkptr += TRACKDATA_MAP_SIZE;
	td20_trk_file_appnd = trkptr;
	trkptr += TRACKDATA_TRACK_FILE_APPEND_SIZE;
	td21_col_from_path = trkptr;
	trkptr += TRACKDATA_MAP_SIZE;
	td22_row_from_path = trkptr;
	trkptr += TRACKDATA_MAP_SIZE;
	roadside_sign_shape_indices = trkptr;
	trkptr += TRACKDATA_OBJECT_INDEX_SIZE;
}

void reset_race_loop_state(void)
{
	frame_callback_countdown = 1;
	slow_replay_countdown = REPLAY_SLOW_CALLBACK_DIVISOR;
	elapsed_time2 = 0;
	race_exit_request = 0;
	race_start_sequence_state = RACE_START_SEQUENCE_INACTIVE;
	start_flag_animation = 0;
}
