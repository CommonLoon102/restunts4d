#include "trackdata_layout.h"
#include "memmgr.h"
#include "car_speed.h"
#include "track_types.h"
#include "track_objects.h"
#include "track_collision.h"
#include "opponent.h"
#include "externs.h"

#define TRACK_GRID_SIZE 30
#define TRACK_GRID_LAST_INDEX 29
#define TRACK_SETUP_TILE_COUNT 901U
#define TRACK_SETUP_BRANCH_COUNT 64U
#define TRACK_SETUP_VISITED_TILE_CAPACITY 904U
#define TRACK_SETUP_PIECE_CAPACITY 902U
#define TRACK_TERRAIN_NO_CONNECTION 99U
#define TRACK_INVALID_ELEMENT_FIRST 253U
#define TRACK_LARGE_ELEMENT_FIRST 182U
#define TRACK_GENERIC_LARGE_ELEMENT 4U
#define TRACK_TILE_INDEX_UNASSIGNED 255U
#define TRACK_TERRAIN_HILL 6U
#define TRACK_HILLROAD_TERRAIN_FIRST 7U
#define TRACK_HILLROAD_TERRAIN_END 11U
#define TRACK_CAMERA_COUNT_MAX 64U
#define TRACK_CAMERA_RESERVED_INDEX 48U
#define TRACK_CAMERA_HILL_HEIGHT 450
#define TRACK_PIECE_SUBTYPE_MASK 15U
#define TRACK_PIECE_REVERSE_FLAG 16U
#define TRACK_PIECE_TRAVERSAL_SHIFT 4U
#define TRACK_ARROW_NONE 0U
#define TRACK_CAMERA_VECTOR_STRIDE 2U
#define TRACK_CAMERA_VECTOR_FORWARD_OFFSET 1U
#define TRACK_CAMERA_VECTOR_REVERSE_OFFSET 2U
#define TRACK_ORIENTATION_COUNT 4U
enum TRACK_ORIENTATION_INDEX {
	TRACK_ORIENTATION_INDEX_NORTH = 0,
	TRACK_ORIENTATION_INDEX_EAST = 1,
	TRACK_ORIENTATION_INDEX_SOUTH = 2,
	TRACK_ORIENTATION_INDEX_WEST = 3
};

enum TRACK_ORIENTATION {
	TRACK_ORIENTATION_NORTH = 0,
	TRACK_ORIENTATION_EAST = ANGLE_QUARTER_TURN,
	TRACK_ORIENTATION_SOUTH = ANGLE_HALF_TURN,
	TRACK_ORIENTATION_WEST = ANGLE_THREE_QUARTER_TURN
};

#define TRACK_ORIENTATION_NOT_FOUND (-1)
#define TRACK_PREVIOUS_PIECE_NONE (-1)
enum TRACK_TRAVERSAL {
	TRACK_TRAVERSAL_UNMATCHED = -1,
	TRACK_TRAVERSAL_FORWARD = 0,
	TRACK_TRAVERSAL_REVERSE = 1
};
#define TRACK_EXIT_POINT_FIRST 1U
#define TRACK_EXIT_POINT_LAST 12U
#define TRACK_EXIT_POINT_TABLE_SIZE 13U
#define TRACK_ENTRY_POINT_NONE 0U

enum TRACK_SEAM_SCAN_DIRECTION {
	TRACK_SEAM_SCAN_COLUMNS = 0,
	TRACK_SEAM_SCAN_ROWS = 1
};

#define TRACK_JUMP_CONNECTION_CODE 1U
#define TRACK_JUMP_MAX_EMPTY_TILES 2U
#define TRACK_JUMP_BACKTRACK_LENGTH_LIMIT 1U
#define TRACK_RUNWAY_MINIMUM_LENGTH 2U
#define TRACK_CAMERA_RUNWAY_THRESHOLD 3U
#define TRACK_START_FINISH_VARIANT_COUNT 3U
#define PLAN_TRACK_ROUTE_LENGTH 18U
#define PLAN_TRACK_ROUTE_ENTRY_SIZE 2U
#define PLAN_TRACK_START_ROW 28
#define PLAN_TRACK_OPPONENT_SPEED 200U
#define PLAN_TRACK_PATH_INDEX 28U
#define PLAN_TRACK_PATH_Z_OFFSET 302
#define PLAN_TRACK_OPPONENT_X 96000L
#define PLAN_TRACK_POSITION_SHIFT 6U

enum TRACK_SETUP_ERROR {
	TRACK_SETUP_OK = 0,
	TRACK_SETUP_NO_START_FINISH = 1,
	TRACK_SETUP_INTERNAL_ERROR = 2,
	TRACK_SETUP_MANY_START_FINISH = 3,
	TRACK_SETUP_ELEMENT_MISMATCH = 4,
	TRACK_SETUP_WRONG_WAY = 5,
	TRACK_SETUP_MANY_ELEMENTS = 6,
	TRACK_SETUP_NO_PATH = 7,
	TRACK_SETUP_MANY_PATHS = 8,
	TRACK_SETUP_NO_RUNWAY = 9,
	TRACK_SETUP_LONG_JUMP = 10,
	TRACK_SETUP_TERRAIN_MISMATCH = 11
};

#pragma pack(push, 1)
struct TRACK_SETUP_BRANCH {
	legacy_s8 column;
	legacy_s8 row;
	legacy_u8 tile_element;
	legacy_u8 subtype;
	legacy_s8 connection_status;
	legacy_u8 runway_length;
	legacy_s8 previous_column;
	legacy_s8 previous_row;
	legacy_u8 previous_tile_element;
	legacy_u8 previous_subtype;
	legacy_s8 previous_connection_status;
	legacy_u8 previous_connection_code;
	legacy_s16 previous_piece;
};
#pragma pack(pop)

typedef legacy_s8 track_setup_branch_must_be_14_bytes[
	(sizeof(struct TRACK_SETUP_BRANCH) == 14) ? 1 : -1];

/* Arriving on a tile, the entry point the walk uses depends on the direction
   of travel and on which continuation marker (if any) sent it here. Each row
   is indexed by orientation / ANGLE_QUARTER_TURN; a zero means
   the combination does not
   occur, exactly as the original chains fell through to zero. */
static const legacy_u8 track_entry_points_northwest[
	TRACK_ORIENTATION_COUNT] = { 12, 0, 0, 9 };
static const legacy_u8 track_entry_points_north[
	TRACK_ORIENTATION_COUNT] = { 11, 6, 0, 7 };
static const legacy_u8 track_entry_points_west[
	TRACK_ORIENTATION_COUNT] = { 10, 0, 5, 8 };
static const legacy_u8 track_entry_points_owner[
	TRACK_ORIENTATION_COUNT] = { 2, 4, 1, 3 };

static const legacy_u8 track_start_finish_elements[
	TRACK_ORIENTATION_COUNT][TRACK_START_FINISH_VARIANT_COUNT] = {
	{ 1, 134, 147 },
	{ 136, 149, 180 },
	{ 135, 148, 179 },
	{ 137, 150, 181 }
};

static legacy_s16 track_setup_start_finish_orientation(
	legacy_u8 tile_element)
{
	legacy_u16 orientation_index;
	legacy_u16 variant_index;

	for (orientation_index = 0U;
		orientation_index < TRACK_ORIENTATION_COUNT;
		orientation_index++) {
		for (variant_index = 0U;
			variant_index < TRACK_START_FINISH_VARIANT_COUNT;
			variant_index++) {
			if (tile_element == track_start_finish_elements[
				orientation_index][variant_index]) {
				return (legacy_s16)(orientation_index *
					ANGLE_QUARTER_TURN);
			}
		}
	}
	return TRACK_ORIENTATION_NOT_FOUND;
}

static legacy_u8 track_setup_entry_point(const legacy_u8* points,
	legacy_s16 orientation)
{
	if (orientation == TRACK_ORIENTATION_NORTH)
		return points[TRACK_ORIENTATION_INDEX_NORTH];
	if (orientation == TRACK_ORIENTATION_EAST)
		return points[TRACK_ORIENTATION_INDEX_EAST];
	if (orientation == TRACK_ORIENTATION_SOUTH)
		return points[TRACK_ORIENTATION_INDEX_SOUTH];
	if (orientation == TRACK_ORIENTATION_WEST)
		return points[TRACK_ORIENTATION_INDEX_WEST];
	return TRACK_ENTRY_POINT_NONE;
}

/* Leaving a piece through one of its exit points steps on to the neighbour
   tile and fixes the orientation the next piece is entered with. Entry 0 is
   never used: the original switch had no case for it and left the position
   and the orientation untouched. */
struct TRACK_SETUP_STEP {
	legacy_s16 column;
	legacy_s16 row;
	legacy_s16 orientation;
};

static const struct TRACK_SETUP_STEP track_setup_steps[
	TRACK_EXIT_POINT_TABLE_SIZE] = {
	{  0,  0, TRACK_ORIENTATION_NORTH },
	{  0, -1, TRACK_ORIENTATION_NORTH },
	{  0,  1, TRACK_ORIENTATION_SOUTH },
	{  1,  0, TRACK_ORIENTATION_EAST },
	{ -1,  0, TRACK_ORIENTATION_WEST },
	{  1, -1, TRACK_ORIENTATION_NORTH },
	{ -1,  1, TRACK_ORIENTATION_WEST },
	{  1,  1, TRACK_ORIENTATION_EAST },
	{  2,  0, TRACK_ORIENTATION_EAST },
	{  2,  1, TRACK_ORIENTATION_EAST },
	{  1,  1, TRACK_ORIENTATION_SOUTH },
	{  0,  2, TRACK_ORIENTATION_SOUTH },
	{  1,  2, TRACK_ORIENTATION_SOUTH }
};

static const legacy_u16 plan_track_route[PLAN_TRACK_ROUTE_LENGTH] = {
	0, 1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4, 0, 1, 2, 3, 0
};

static legacy_s8 track_setup_add_s8(legacy_s8 value, legacy_s16 amount)
{
	return LEGACY_S8_WRAP_ADD(value, amount);
}

static void track_setup_rotate_vector(
	struct VECTOR* vector,
	legacy_s16 orientation
) {
	legacy_s16 temporary;

	if (orientation == TRACK_ORIENTATION_EAST) {
		temporary = vector->x;
		vector->x = vector->z;
		vector->z = LEGACY_S16_WRAP_NEGATE(temporary);
	} else if (orientation == TRACK_ORIENTATION_SOUTH) {
		vector->x = LEGACY_S16_WRAP_NEGATE(vector->x);
		vector->z = LEGACY_S16_WRAP_NEGATE(vector->z);
	} else if (orientation == TRACK_ORIENTATION_WEST) {
		temporary = vector->x;
		vector->x = LEGACY_S16_WRAP_NEGATE(vector->z);
		vector->z = temporary;
	}
}

static void track_setup_link_piece(
	legacy_s16 source_piece,
	legacy_s16 destination_piece
) {
	if (track_primary_route_links[source_piece] == TRACK_ROUTE_LINK_NONE)
		track_primary_route_links[source_piece] = destination_piece;
	else
		track_alternate_route_links[source_piece] = destination_piece;
}

static legacy_s16 track_setup_error(
	struct TRACK_SETUP_BRANCH far* branches,
	legacy_u8 error_code,
	legacy_s8 column,
	legacy_s8 row
) {
	if (column == -1)
		column = 0;
	else if (column == TRACK_GRID_SIZE)
		column = TRACK_GRID_LAST_INDEX;
	if (row == -1)
		row = 0;
	else if (row == TRACK_GRID_SIZE)
		row = TRACK_GRID_LAST_INDEX;
	track_validation_column = (legacy_u8)column;
	track_validation_row = (legacy_u8)row;
	mmgr_release((legacy_s8 far*)branches);
	return error_code;
}

/* Neighbouring terrain tiles have to agree on the connection code of the
   edge they share. The map is walked twice the same way: once west to east
   along every row, once north to south down every column. Returns 0 when
   every seam matches, otherwise the reported error code. */
static legacy_s16 track_setup_terrain_seams(
	struct TRACK_SETUP_BRANCH far* branches,
	const legacy_u8* incoming, const legacy_u8* outgoing,
	legacy_s16 scan_direction)
{
	legacy_u8 previous_connection_code;
	legacy_u8 tile_terrain;
	legacy_s8 column;
	legacy_s8 row;
	legacy_s8 outer;
	legacy_s8 inner;

	for (outer = 0; outer < TRACK_GRID_SIZE; outer++) {
		previous_connection_code = TRACK_TERRAIN_NO_CONNECTION;
		for (inner = 0; inner < TRACK_GRID_SIZE; inner++) {
			row = scan_direction == TRACK_SEAM_SCAN_ROWS ? outer : inner;
			column = scan_direction == TRACK_SEAM_SCAN_ROWS ? inner : outer;
			tile_terrain = track_terrain_map[
				terrainrows[row] + column];
			if (incoming[tile_terrain] != previous_connection_code &&
				previous_connection_code != TRACK_TERRAIN_NO_CONNECTION) {
				return track_setup_error(branches,
					TRACK_SETUP_TERRAIN_MISMATCH, column, row);
			}
			previous_connection_code = outgoing[tile_terrain];
		}
	}

	return TRACK_SETUP_OK;
}

legacy_s16 track_setup(void)
{
	struct TRACK_SETUP_BRANCH far* branches;
	struct TRACK_SETUP_BRANCH far* branch;
	struct TRACKOBJECT* track_object;
	struct TRACKOBJECT* previous_track_object;
	struct TRKOBJINFO* track_info;
	struct TRKOBJINFO* current_info;
	struct TRKOBJINFO* previous_info;
	struct VECTOR* camera_vectors;
	struct VECTOR camera_vector;
	legacy_s16 far* camera_height;
	legacy_s16 far* reserved_camera_words;
	legacy_u8 visited_tiles[TRACK_SETUP_VISITED_TILE_CAPACITY];
	legacy_u8 subtype_by_piece[TRACK_SETUP_PIECE_CAPACITY];
	legacy_s8 connection_by_piece[TRACK_SETUP_PIECE_CAPACITY];
	legacy_u16 branch_count;
	legacy_u16 block_index;
	legacy_u16 index;
	legacy_u16 sample_index;
	legacy_u16 camera_count;
	legacy_s16 previous_piece;
	legacy_s16 existing_piece;
	legacy_s16 sampled_piece;
	legacy_s16 tile_index;
	legacy_s16 camera_index;
	legacy_s16 orientation;
	legacy_s16 base_position;
	legacy_u16 opponent_path_offset;
	legacy_u8 tile_terrain;
	legacy_u8 tile_element;
	legacy_u8 tile_entry_point;
	legacy_u8 previous_connection_code;
	legacy_u8 subtype;
	legacy_s8 connection_status;
	legacy_s8 selected_connection_status;
	legacy_u8 previous_subtype;
	legacy_s8 previous_connection_status;
	legacy_u8 previous_tile_element;
	legacy_s8 previous_column;
	legacy_s8 previous_row;
	legacy_s8 column;
	legacy_s8 row;
	legacy_u8 start_finish_count;
	legacy_u8 runway_length;
	legacy_u8 jump_length;
	legacy_u8 path_closed;
	legacy_u8 match_count;
	legacy_u8 backtrack_required;
	legacy_u8 arrow_code;
	legacy_s16 seam_status;

	branches = (struct TRACK_SETUP_BRANCH far*)
		mmgr_alloc_resbytes("tcomp",
			TRACK_SETUP_BRANCH_COUNT * sizeof(struct TRACK_SETUP_BRANCH));
	if (branches == 0)
		return TRACK_SETUP_INTERNAL_ERROR;

	camera_height = trackside_camera_ground_heights;
	reserved_camera_words = reserved_trackside_camera_words;
	start_finish_count = 0;
	jump_length = 0;
	track_pieces_counter = 0;
	for (index = 0; index < TRACK_SETUP_TILE_COUNT; index++)
		roadside_sign_indices_by_tile[index] = TRACK_TILE_INDEX_UNASSIGNED;

	seam_status = track_setup_terrain_seams(branches,
		terrConnDataEtoW, terrConnDataWtoE, TRACK_SEAM_SCAN_ROWS);
	if (seam_status != TRACK_SETUP_OK)
		return seam_status;
	seam_status = track_setup_terrain_seams(branches,
		terrConnDataNtoS, terrConnDataStoN, TRACK_SEAM_SCAN_COLUMNS);
	if (seam_status != TRACK_SETUP_OK)
		return seam_status;

	for (row = 0; row < TRACK_GRID_SIZE; row++) {
		for (column = 0; column < TRACK_GRID_SIZE; column++) {
			tile_index = trackrows[row] + column;
			tile_element = track_element_map[tile_index];
			if (tile_element >= TRACK_INVALID_ELEMENT_FIRST)
				tile_element = 0;
			if (tile_element >= TRACK_LARGE_ELEMENT_FIRST) {
				tile_element = TRACK_GENERIC_LARGE_ELEMENT;
				track_element_map[tile_index] =
					TRACK_GENERIC_LARGE_ELEMENT;
			}

			orientation = track_setup_start_finish_orientation(tile_element);

			if (orientation != TRACK_ORIENTATION_NOT_FOUND) {
				track_angle = orientation;
				if (start_finish_count != 0) {
					return track_setup_error(branches,
						TRACK_SETUP_MANY_START_FINISH, column, row);
				}
				start_finish_column = column;
				start_finish_row = row;
				tile_terrain = track_terrain_map[
					terrainrows[row] + column];
				hillFlag = tile_terrain == TRACK_TERRAIN_HILL;
				start_finish_count = LEGACY_U8_WRAP_ADD(
					start_finish_count, 1U);
			}
		}
	}

	if (start_finish_count == 0) {
		return track_setup_error(branches,
			TRACK_SETUP_NO_START_FINISH, column, row);
	}

	track_pieces_counter = 0;
	branch_count = 0;
	roadside_sign_count = 0;
	trackside_camera_count = 0;
	runway_length = 0;
	path_closed = 0;
	for (index = 0; index < TRACK_SETUP_TILE_COUNT; index++) {
		visited_tiles[index] = 0;
		track_primary_route_links[index] = TRACK_ROUTE_LINK_NONE;
		track_alternate_route_links[index] = TRACK_ROUTE_LINK_NONE;
	}

	column = LEGACY_S8_FROM_BITS((legacy_u8)start_finish_column);
	row = LEGACY_S8_FROM_BITS((legacy_u8)start_finish_row);
	orientation = (legacy_s16)track_angle;
	previous_connection_code = 0;
	previous_piece = TRACK_PREVIOUS_PIECE_NONE;

	for (;;) {
	match_count = 0;
	backtrack_required = 0;
	if (column < 0 || row < 0 || column > TRACK_GRID_LAST_INDEX ||
		row > TRACK_GRID_LAST_INDEX)
		backtrack_required = 1;

	if (backtrack_required == 0) {
	tile_element = track_element_map[trackrows[row] + column];
	tile_terrain = track_terrain_map[terrainrows[row] + column];
	if (tile_element != 0 &&
		tile_terrain >= TRACK_HILLROAD_TERRAIN_FIRST &&
		tile_terrain < TRACK_HILLROAD_TERRAIN_END)
		tile_element = subst_hillroad_track(tile_terrain, tile_element);

	if (tile_element == TRACK_TILE_CONTINUATION_SOUTHEAST) {
		column = track_setup_add_s8(column, -1);
		row = track_setup_add_s8(row, -1);
		tile_entry_point = track_setup_entry_point(
			track_entry_points_northwest, orientation);
		tile_element = track_element_map[trackrows[row] + column];
	} else if (tile_element == TRACK_TILE_CONTINUATION_SOUTH) {
		row = track_setup_add_s8(row, -1);
		tile_entry_point = track_setup_entry_point(
			track_entry_points_north, orientation);
		tile_element = track_element_map[trackrows[row] + column];
	} else if (tile_element == TRACK_TILE_CONTINUATION_EAST) {
		column = track_setup_add_s8(column, -1);
		tile_entry_point = track_setup_entry_point(
			track_entry_points_west, orientation);
		tile_element = track_element_map[trackrows[row] + column];
	} else {
		tile_entry_point = track_setup_entry_point(
			track_entry_points_owner, orientation);
	}

	if (jump_length == 0 && tile_entry_point == TRACK_ENTRY_POINT_NONE) {
		return track_setup_error(branches,
			TRACK_SETUP_INTERNAL_ERROR, column, row);
	}

	track_object = &trkObjectList[tile_element];
	track_info = track_object->ss_trkObjInfoPtr;
	if (track_info != 0) {
		for (block_index = 0;
			block_index < (legacy_u8)track_info->si_noOfBlocks;
			block_index++) {
			current_info = &track_info[block_index];
			connection_status = TRACK_TRAVERSAL_UNMATCHED;
			if ((legacy_u8)current_info->si_entryPoint ==
				tile_entry_point) {
				if ((legacy_u8)current_info->si_entryType !=
					previous_connection_code) {
					return track_setup_error(branches,
						TRACK_SETUP_ELEMENT_MISMATCH, column, row);
				}
				connection_status = TRACK_TRAVERSAL_FORWARD;
			} else if ((legacy_u8)current_info->si_exitPoint ==
				tile_entry_point) {
				if ((legacy_u8)current_info->si_exitType !=
					previous_connection_code) {
					return track_setup_error(branches,
						TRACK_SETUP_ELEMENT_MISMATCH, column, row);
				}
				connection_status = TRACK_TRAVERSAL_REVERSE;
			}

			if (connection_status != TRACK_TRAVERSAL_UNMATCHED &&
				visited_tiles[trackrows[row] + column] != 0) {
				for (existing_piece = 0;
					existing_piece < track_pieces_counter;
					existing_piece++) {
					if ((legacy_u8)track_route_columns[existing_piece] ==
						(legacy_u8)column &&
						(legacy_u8)track_route_rows[existing_piece] ==
						(legacy_u8)row &&
						subtype_by_piece[existing_piece] ==
						(legacy_u8)block_index &&
						connection_by_piece[existing_piece] ==
						connection_status) {
						connection_status = TRACK_TRAVERSAL_UNMATCHED;
						track_setup_link_piece(
							previous_piece, existing_piece);
						if (existing_piece == 0)
							path_closed = 1;
						break;
					}
				}
			}

			if (connection_status != TRACK_TRAVERSAL_UNMATCHED) {
				if (match_count == 0) {
					subtype = (legacy_u8)block_index;
					selected_connection_status = connection_status;
				} else {
					if (branch_count == TRACK_SETUP_BRANCH_COUNT) {
						return track_setup_error(branches,
							TRACK_SETUP_MANY_PATHS, column, row);
					}
					branch = &branches[branch_count];
					branch->column = column;
					branch->row = row;
					branch->tile_element = tile_element;
					branch->subtype = (legacy_u8)block_index;
					branch->connection_status = connection_status;
					branch->previous_connection_code =
						previous_connection_code;
					branch->previous_piece = previous_piece;
					branch->runway_length = runway_length;
					branch->previous_column = previous_column;
					branch->previous_row = previous_row;
					branch->previous_tile_element = previous_tile_element;
					branch->previous_subtype = previous_subtype;
					branch->previous_connection_status =
						previous_connection_status;
					branch_count = LEGACY_U16_WRAP_ADD(
						branch_count, 1U);
				}
				match_count = LEGACY_U8_WRAP_ADD(match_count, 1U);
			}
		}
	}

	if (match_count != 0) {
		connection_status = selected_connection_status;
	} else if (previous_connection_code != TRACK_JUMP_CONNECTION_CODE ||
		jump_length >= TRACK_JUMP_MAX_EMPTY_TILES) {
		backtrack_required = 1;
	} else {
		if (runway_length < TRACK_RUNWAY_MINIMUM_LENGTH) {
			return track_setup_error(branches,
				TRACK_SETUP_NO_RUNWAY, column, row);
		}
		runway_length = LEGACY_U8_WRAP_ADD(runway_length, 1U);
		jump_length = LEGACY_U8_WRAP_ADD(jump_length, 1U);
		if (orientation == TRACK_ORIENTATION_NORTH) {
			column = previous_column;
			row = track_setup_add_s8(previous_row,
				-(legacy_s16)jump_length - 1);
		} else if (orientation == TRACK_ORIENTATION_EAST) {
			row = previous_row;
			column = track_setup_add_s8(previous_column,
				(legacy_s16)jump_length + 1);
		} else if (orientation == TRACK_ORIENTATION_SOUTH) {
			column = previous_column;
			row = track_setup_add_s8(previous_row,
				(legacy_s16)jump_length + 1);
		} else if (orientation == TRACK_ORIENTATION_WEST) {
			row = previous_row;
			column = track_setup_add_s8(previous_column,
				-(legacy_s16)jump_length - 1);
		}
		continue;
	}
	}

	if (backtrack_required != 0) {
		if (branch_count == 0) {
			if (path_closed == 0) {
				return track_setup_error(branches,
					TRACK_SETUP_NO_PATH, column, row);
			}
			break;
		}
		branch_count = LEGACY_U16_WRAP_SUB(branch_count, 1U);
		branch = &branches[branch_count];
		column = branch->column;
		row = branch->row;
		tile_element = branch->tile_element;
		subtype = branch->subtype;
		connection_status = branch->connection_status;
		previous_connection_code = branch->previous_connection_code;
		previous_piece = branch->previous_piece;
		runway_length = branch->runway_length;
		previous_column = branch->previous_column;
		previous_row = branch->previous_row;
		previous_tile_element = branch->previous_tile_element;
		previous_subtype = branch->previous_subtype;
		previous_connection_status = branch->previous_connection_status;
		if (jump_length > TRACK_JUMP_BACKTRACK_LENGTH_LIMIT) {
			return track_setup_error(branches,
				TRACK_SETUP_LONG_JUMP, column, row);
		}
	}

	jump_length = 0;
	visited_tiles[trackrows[row] + column] = 1;
	subtype_by_piece[track_pieces_counter] = subtype;
	connection_by_piece[track_pieces_counter] = connection_status;
	if (previous_piece != TRACK_PREVIOUS_PIECE_NONE)
		track_setup_link_piece(previous_piece, track_pieces_counter);
	previous_piece = (legacy_s16)track_pieces_counter;
	track_route_columns[track_pieces_counter] = column;
	track_route_rows[track_pieces_counter] = row;
	track_route_traversal_flags[track_pieces_counter] = (legacy_u8)(
		LEGACY_U16_WRAP_ADD(
			LEGACY_U16_SHL((legacy_u8)connection_status,
				TRACK_PIECE_TRAVERSAL_SHIFT), subtype));
	track_route_element_ids[track_pieces_counter] = tile_element;

	track_info = trkObjectList[tile_element].ss_trkObjInfoPtr;
	current_info = &track_info[subtype];
	arrow_code = (legacy_u8)current_info->roadside_sign_type;
	if (arrow_code == TRACK_ARROW_NONE) {
		runway_length = LEGACY_U8_WRAP_ADD(runway_length, 1U);
	} else {
		if (arrow_code != LEGACY_U8_MAX &&
			runway_length > TRACK_CAMERA_RUNWAY_THRESHOLD &&
			roadside_sign_count != TRACK_CAMERA_RESERVED_INDEX) {
			previous_track_object = &trkObjectList[previous_tile_element];
			previous_info = &previous_track_object->
				ss_trkObjInfoPtr[previous_subtype];
			opponent_path_offset = (legacy_u16)(
				(legacy_u8)previous_info->reverse_path_offset_low |
				LEGACY_U16_SHL(
					(legacy_u8)previous_info->reverse_path_offset_high, 8U));
			if (previous_connection_status == TRACK_TRAVERSAL_REVERSE &&
				opponent_path_offset != 0)
				camera_vectors = track_vector_from_legacy_offset(
					opponent_path_offset);
			else
				camera_vectors = previous_info->route_vectors;
			index = LEGACY_U16_WRAP_MUL(
				(legacy_u8)previous_info->route_point_count,
				TRACK_CAMERA_VECTOR_STRIDE);
			if (previous_connection_status == TRACK_TRAVERSAL_REVERSE)
				index = LEGACY_U16_WRAP_ADD(index,
					TRACK_CAMERA_VECTOR_REVERSE_OFFSET);
			else
				index = LEGACY_U16_WRAP_ADD(index,
					TRACK_CAMERA_VECTOR_FORWARD_OFFSET);
			camera_vector = camera_vectors[index];
			if (connection_status == TRACK_TRAVERSAL_REVERSE)
				arrow_code = roadside_sign_reverse_types[
					LEGACY_S8_FROM_BITS(arrow_code)];
			else
				arrow_code = roadside_sign_forward_types[
					LEGACY_S8_FROM_BITS(arrow_code)];
			orientation = (legacy_s16)previous_info->route_orientation;
			track_setup_rotate_vector(&camera_vector, orientation);
			roadside_sign_headings[roadside_sign_count] =
				previous_connection_status == TRACK_TRAVERSAL_REVERSE ?
				(orientation ^ TRACK_ORIENTATION_SOUTH) : orientation;
			roadside_sign_shape_indices[roadside_sign_count] = arrow_code;
			if (track_terrain_map[terrainrows[previous_row] +
				previous_column] == TRACK_TERRAIN_HILL)
				camera_vector.y = LEGACY_S16_WRAP_ADD(
					camera_vector.y, TRACK_CAMERA_HILL_HEIGHT);
			camera_index = (legacy_s16)roadside_sign_count;
			roadside_sign_positions[camera_index].y = camera_vector.y;
			base_position = track_object_base_z(
				previous_track_object, (legacy_u8)previous_row);
			roadside_sign_positions[camera_index].z =
				LEGACY_S16_WRAP_ADD(camera_vector.z, base_position);
			base_position = track_object_base_x(
				previous_track_object, (legacy_u8)previous_column);
			roadside_sign_positions[camera_index].x =
				LEGACY_S16_WRAP_ADD(camera_vector.x, base_position);
			roadside_sign_indices_by_tile[trackrows[previous_row] + previous_column] =
				roadside_sign_count;
			roadside_sign_count = LEGACY_U8_WRAP_ADD(roadside_sign_count, 1U);
		}
		runway_length = 0;
	}

	track_pieces_counter = LEGACY_S16_WRAP_ADD(
		track_pieces_counter, 1);
	if (track_pieces_counter == TRACK_SETUP_TILE_COUNT) {
		return track_setup_error(branches,
			TRACK_SETUP_MANY_ELEMENTS, column, row);
	}
	current_info = &track_info[subtype];
	if (connection_status == TRACK_TRAVERSAL_REVERSE) {
		tile_entry_point = (legacy_u8)current_info->si_entryPoint;
		previous_connection_code = (legacy_u8)current_info->si_entryType;
	} else {
		tile_entry_point = (legacy_u8)current_info->si_exitPoint;
		previous_connection_code = (legacy_u8)current_info->si_exitType;
	}
	previous_column = column;
	previous_row = row;
	previous_connection_status = connection_status;
	previous_subtype = subtype;
	previous_tile_element = tile_element;

	if (tile_entry_point >= TRACK_EXIT_POINT_FIRST &&
		tile_entry_point <= TRACK_EXIT_POINT_LAST) {
		const struct TRACK_SETUP_STEP* step =
			&track_setup_steps[tile_entry_point];

		column = track_setup_add_s8(column, step->column);
		row = track_setup_add_s8(row, step->row);
		orientation = step->orientation;
	}
	}

	track_validation_column = (legacy_u8)start_finish_column;
	track_validation_row = (legacy_u8)start_finish_row;
	camera_count = (legacy_u16)LEGACY_S16_DIV_OR_ZERO(
		track_pieces_counter, 3);
	if (camera_count > TRACK_CAMERA_COUNT_MAX)
		camera_count = TRACK_CAMERA_COUNT_MAX;
	trackside_camera_count = (legacy_u8)camera_count;
	for (index = 0; index < TRACK_SETUP_TILE_COUNT; index++)
		subtype_by_piece[index] = 0;
	camera_index = 0;
	for (sample_index = 0; sample_index < trackside_camera_count; sample_index++) {
		sampled_piece = LEGACY_S16_FROM_BITS((legacy_u16)
			LEGACY_S32_DIV_OR_ZERO(
				LEGACY_S32_WRAP_MUL(
					(legacy_s32)track_pieces_counter,
					(legacy_s32)sample_index),
				(legacy_s32)(legacy_u16)trackside_camera_count));
		column = LEGACY_S8_FROM_BITS(
			(legacy_u8)track_route_columns[sampled_piece]);
		row = LEGACY_S8_FROM_BITS(
			(legacy_u8)track_route_rows[sampled_piece]);
		tile_index = terrainrows[row] + column;
		if (subtype_by_piece[tile_index] != 0)
			continue;
		subtype_by_piece[tile_index] = 1;
		tile_element = (legacy_u8)track_route_element_ids[sampled_piece];
		subtype = (legacy_u8)track_route_traversal_flags[sampled_piece] &
			TRACK_PIECE_SUBTYPE_MASK;
		connection_status = ((legacy_u8)track_route_traversal_flags[sampled_piece] &
			TRACK_PIECE_REVERSE_FLAG) != 0 ? TRACK_TRAVERSAL_REVERSE :
			TRACK_TRAVERSAL_FORWARD;
		track_object = &trkObjectList[tile_element];
		current_info = &track_object->ss_trkObjInfoPtr[subtype];
		opponent_path_offset = (legacy_u16)(
			(legacy_u8)current_info->reverse_path_offset_low |
			LEGACY_U16_SHL((legacy_u8)current_info->reverse_path_offset_high, 8U));
		if (connection_status == TRACK_TRAVERSAL_REVERSE &&
			opponent_path_offset != 0)
			camera_vectors = track_vector_from_legacy_offset(
				opponent_path_offset);
		else
			camera_vectors = current_info->route_vectors;
		index = LEGACY_U16_WRAP_MUL(
			(legacy_u8)current_info->route_point_count, 2U);
		camera_vector = camera_vectors[index];
		orientation = (legacy_s16)current_info->route_orientation;
		track_setup_rotate_vector(&camera_vector, orientation);
		if (track_terrain_map[terrainrows[row] + column] == 6)
			camera_height[camera_index] = TRACK_CAMERA_HILL_HEIGHT;
		else
			camera_height[camera_index] = 0;
		reserved_camera_words[camera_index] = 0;
		trackside_camera_positions[camera_index].y = LEGACY_S16_WRAP_ADD(
			camera_height[camera_index], camera_vector.y);
		base_position = track_object_base_z(
			track_object, (legacy_u8)row);
		trackside_camera_positions[camera_index].z = LEGACY_S16_WRAP_ADD(
			base_position, camera_vector.z);
		base_position = track_object_base_x(
			track_object, (legacy_u8)column);
		trackside_camera_positions[camera_index].x = LEGACY_S16_WRAP_ADD(
			base_position, camera_vector.x);
		camera_index = LEGACY_S16_WRAP_ADD(camera_index, 1);
	}
	trackside_camera_count = (legacy_u8)camera_index;
	mmgr_release((legacy_s8 far*)branches);
	return TRACK_SETUP_OK;
}

void init_plantrak(void) {
	legacy_s16 path_z;
	legacy_s16 route_track_index;
	legacy_u16 route_table_offset;
	legacy_u8 route_index;

	init_game_state(GAMESTATE_INIT_TIMING_ONLY);
	state.game_inputmode = GAME_INPUT_MODE_INTRO;
	planptr = &plan_memres;
	start_finish_column = 1;
	start_finish_row = PLAN_TRACK_START_ROW;

	track_route_element_ids[0] = 7;
	track_route_element_ids[1] = 6;
	track_route_element_ids[2] = 8;
	track_route_element_ids[3] = 9;
	track_route_element_ids[4] = 7;

	track_route_columns[0] = 1;
	track_route_columns[1] = 0;
	track_route_columns[2] = 0;
	track_route_columns[3] = 1;
	track_route_columns[4] = 1;

	track_route_rows[0] = start_finish_row;
	track_route_rows[1] = start_finish_row;
	track_route_rows[2] = LEGACY_U8_WRAP_ADD(start_finish_row, 1U);
	track_route_rows[3] = LEGACY_U8_WRAP_ADD(start_finish_row, 1U);
	track_route_rows[4] = start_finish_row;

	track_route_traversal_flags[0] = 0;
	track_route_traversal_flags[1] = 0;
	track_route_traversal_flags[2] = 0;
	track_route_traversal_flags[3] = 0;
	track_route_traversal_flags[4] = 0;

	for (route_index = 0U; route_index < PLAN_TRACK_ROUTE_LENGTH;
		route_index++) {
		route_table_offset = LEGACY_U16_WRAP_MUL(
			route_index, PLAN_TRACK_ROUTE_ENTRY_SIZE);
		LEGACY_WRITE_U16_LE((legacy_u8 far*)opponent_route_track_indices +
			route_table_offset, plan_track_route[route_index]);
	}

	oppnentSped[0] = PLAN_TRACK_OPPONENT_SPEED;
	path_z = LEGACY_S16_WRAP_ADD(track_row_positions[PLAN_TRACK_PATH_INDEX],
		PLAN_TRACK_PATH_Z_OFFSET);
	init_carstate_from_simd(
		&state.opponentstate,
		&simd_opponent,
		TRANSMISSION_AUTOMATIC,
		(legacy_s32)PLAN_TRACK_OPPONENT_X,
		0L,
		LEGACY_S32_SHL((legacy_s32)path_z, PLAN_TRACK_POSITION_SHIFT),
		0);

	route_index = (legacy_u8)state.opponentstate.car_route_point_index;
	state.opponentstate.car_route_point_index = LEGACY_S8_WRAP_ADD(
		route_index, ROUTE_POINT_STEP);
	opponent_route_advance((legacy_s16)route_index);
}
