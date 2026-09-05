#include "state_internal.h"

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
#define TRACK_CONTINUATION_NORTHWEST 253U
#define TRACK_CONTINUATION_NORTH 254U
#define TRACK_CONTINUATION_WEST 255U
#define TRACK_TILE_INDEX_UNASSIGNED 255U
#define TRACK_HILLROAD_TERRAIN_FIRST 7U
#define TRACK_HILLROAD_TERRAIN_END 11U
#define TRACK_CAMERA_COUNT_MAX 64U
#define TRACK_CAMERA_RESERVED_INDEX 48U
#define TRACK_CAMERA_HILL_HEIGHT 450
#define TRACK_PIECE_SUBTYPE_MASK 15U
#define TRACK_PIECE_REVERSE_FLAG 16U
#define TRACK_ORIENTATION_COUNT 4U
#define TRACK_ORIENTATION_QUARTER_TURN 256
#define TRACK_ORIENTATION_NORTH 0
#define TRACK_ORIENTATION_EAST 256
#define TRACK_ORIENTATION_SOUTH 512
#define TRACK_ORIENTATION_WEST 768

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
   is indexed by orientation / TRACK_ORIENTATION_QUARTER_TURN; a zero means
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

static legacy_u8 track_setup_entry_point(const legacy_u8* points,
	legacy_s16 orientation)
{
	if (orientation == TRACK_ORIENTATION_NORTH)
		return points[0];
	if (orientation == TRACK_ORIENTATION_EAST)
		return points[1];
	if (orientation == TRACK_ORIENTATION_SOUTH)
		return points[2];
	if (orientation == TRACK_ORIENTATION_WEST)
		return points[3];
	return 0;
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

static const struct TRACK_SETUP_STEP track_setup_steps[13] = {
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
	if (td01_track_file_cpy[source_piece] == -1)
		td01_track_file_cpy[source_piece] = destination_piece;
	else
		td02_penalty_related[source_piece] = destination_piece;
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
	byte_45D90 = (legacy_u8)column;
	byte_45E16 = (legacy_u8)row;
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
	legacy_s16 along_row)
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
			row = along_row ? outer : inner;
			column = along_row ? inner : outer;
			tile_terrain = td15_terr_map_main[
				terrainrows[row] + column];
			if (incoming[tile_terrain] != previous_connection_code &&
				previous_connection_code != TRACK_TERRAIN_NO_CONNECTION) {
				return track_setup_error(branches,
					TRACK_SETUP_TERRAIN_MISMATCH, column, row);
			}
			previous_connection_code = outgoing[tile_terrain];
		}
	}

	return 0;
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
	legacy_s16 far* camera_unknown;
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

	camera_height = trackdata7;
	camera_unknown = trackdata6;
	start_finish_count = 0;
	jump_length = 0;
	track_pieces_counter = 0;
	for (index = 0; index < TRACK_SETUP_TILE_COUNT; index++)
		trackdata19[index] = TRACK_TILE_INDEX_UNASSIGNED;

	seam_status = track_setup_terrain_seams(branches,
		terrConnDataEtoW, terrConnDataWtoE, 1);
	if (seam_status != 0)
		return seam_status;
	seam_status = track_setup_terrain_seams(branches,
		terrConnDataNtoS, terrConnDataStoN, 0);
	if (seam_status != 0)
		return seam_status;

	for (row = 0; row < TRACK_GRID_SIZE; row++) {
		for (column = 0; column < TRACK_GRID_SIZE; column++) {
			tile_index = trackrows[row] + column;
			tile_element = td14_elem_map_main[tile_index];
			if (tile_element >= TRACK_INVALID_ELEMENT_FIRST)
				tile_element = 0;
			if (tile_element >= TRACK_LARGE_ELEMENT_FIRST) {
				tile_element = TRACK_GENERIC_LARGE_ELEMENT;
				td14_elem_map_main[tile_index] =
					TRACK_GENERIC_LARGE_ELEMENT;
			}

			orientation = -1;
			if (tile_element == 1 || tile_element == 0x86U ||
				tile_element == 0x93U)
				orientation = 0;
			else if (tile_element == 0x87U ||
				tile_element == 0x94U || tile_element == 0xB3U)
				orientation = TRACK_ORIENTATION_SOUTH;
			else if (tile_element == 0x88U ||
				tile_element == 0x95U || tile_element == 0xB4U)
				orientation = TRACK_ORIENTATION_EAST;
			else if (tile_element == 0x89U ||
				tile_element == 0x96U || tile_element == 0xB5U)
				orientation = TRACK_ORIENTATION_WEST;

			if (orientation != -1) {
				track_angle = orientation;
				if (start_finish_count != 0) {
					return track_setup_error(branches,
						TRACK_SETUP_MANY_START_FINISH, column, row);
				}
				startcol2 = column;
				startrow2 = row;
				tile_terrain = td15_terr_map_main[
					terrainrows[row] + column];
				hillFlag = tile_terrain == 6;
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
	byte_45635 = 0;
	byte_4616E = 0;
	runway_length = 0;
	path_closed = 0;
	for (index = 0; index < TRACK_SETUP_TILE_COUNT; index++) {
		visited_tiles[index] = 0;
		td01_track_file_cpy[index] = -1;
		td02_penalty_related[index] = -1;
	}

	column = LEGACY_S8_FROM_BITS((legacy_u8)startcol2);
	row = LEGACY_S8_FROM_BITS((legacy_u8)startrow2);
	orientation = (legacy_s16)track_angle;
	previous_connection_code = 0;
	previous_piece = -1;

	for (;;) {
	match_count = 0;
	backtrack_required = 0;
	if (column < 0 || row < 0 || column > TRACK_GRID_LAST_INDEX ||
		row > TRACK_GRID_LAST_INDEX)
		backtrack_required = 1;

	if (backtrack_required == 0) {
	tile_element = td14_elem_map_main[trackrows[row] + column];
	tile_terrain = td15_terr_map_main[terrainrows[row] + column];
	if (tile_element != 0 &&
		tile_terrain >= TRACK_HILLROAD_TERRAIN_FIRST &&
		tile_terrain < TRACK_HILLROAD_TERRAIN_END)
		tile_element = subst_hillroad_track(tile_terrain, tile_element);

	if (tile_element == TRACK_CONTINUATION_NORTHWEST) {
		column = track_setup_add_s8(column, -1);
		row = track_setup_add_s8(row, -1);
		tile_entry_point = track_setup_entry_point(
			track_entry_points_northwest, orientation);
		tile_element = td14_elem_map_main[trackrows[row] + column];
	} else if (tile_element == TRACK_CONTINUATION_NORTH) {
		row = track_setup_add_s8(row, -1);
		tile_entry_point = track_setup_entry_point(
			track_entry_points_north, orientation);
		tile_element = td14_elem_map_main[trackrows[row] + column];
	} else if (tile_element == TRACK_CONTINUATION_WEST) {
		column = track_setup_add_s8(column, -1);
		tile_entry_point = track_setup_entry_point(
			track_entry_points_west, orientation);
		tile_element = td14_elem_map_main[trackrows[row] + column];
	} else {
		tile_entry_point = track_setup_entry_point(
			track_entry_points_owner, orientation);
	}

	if (jump_length == 0 && tile_entry_point == 0) {
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
			connection_status = -1;
			if ((legacy_u8)current_info->si_entryPoint ==
				tile_entry_point) {
				if ((legacy_u8)current_info->si_entryType !=
					previous_connection_code) {
					return track_setup_error(branches,
						TRACK_SETUP_ELEMENT_MISMATCH, column, row);
				}
				connection_status = 0;
			} else if ((legacy_u8)current_info->si_exitPoint ==
				tile_entry_point) {
				if ((legacy_u8)current_info->si_exitType !=
					previous_connection_code) {
					return track_setup_error(branches,
						TRACK_SETUP_ELEMENT_MISMATCH, column, row);
				}
				connection_status = 1;
			}

			if (connection_status >= 0 &&
				visited_tiles[trackrows[row] + column] != 0) {
				for (existing_piece = 0;
					existing_piece < track_pieces_counter;
					existing_piece++) {
					if ((legacy_u8)td21_col_from_path[existing_piece] ==
						(legacy_u8)column &&
						(legacy_u8)td22_row_from_path[existing_piece] ==
						(legacy_u8)row &&
						subtype_by_piece[existing_piece] ==
						(legacy_u8)block_index &&
						connection_by_piece[existing_piece] ==
						connection_status) {
						connection_status = -1;
						track_setup_link_piece(
							previous_piece, existing_piece);
						if (existing_piece == 0)
							path_closed = 1;
						break;
					}
				}
			}

			if (connection_status >= 0) {
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
	} else if (previous_connection_code != 1 || jump_length >= 2) {
		backtrack_required = 1;
	} else {
		if (runway_length < 2) {
			return track_setup_error(branches,
				TRACK_SETUP_NO_RUNWAY, column, row);
		}
		runway_length = LEGACY_U8_WRAP_ADD(runway_length, 1U);
		jump_length = LEGACY_U8_WRAP_ADD(jump_length, 1U);
		if (orientation == 0) {
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
		if (jump_length > 1) {
			return track_setup_error(branches,
				TRACK_SETUP_LONG_JUMP, column, row);
		}
	}

	jump_length = 0;
	visited_tiles[trackrows[row] + column] = 1;
	subtype_by_piece[track_pieces_counter] = subtype;
	connection_by_piece[track_pieces_counter] = connection_status;
	if (previous_piece != -1)
		track_setup_link_piece(previous_piece, track_pieces_counter);
	previous_piece = (legacy_s16)track_pieces_counter;
	td21_col_from_path[track_pieces_counter] = column;
	td22_row_from_path[track_pieces_counter] = row;
	trackdata18[track_pieces_counter] = (legacy_u8)(
		LEGACY_U16_WRAP_ADD(
			LEGACY_U16_SHL((legacy_u8)connection_status, 4U), subtype));
	td17_trk_elem_ordered[track_pieces_counter] = tile_element;

	track_info = trkObjectList[tile_element].ss_trkObjInfoPtr;
	current_info = &track_info[subtype];
	arrow_code = (legacy_u8)current_info->si_opp3;
	if (arrow_code == 0) {
		runway_length = LEGACY_U8_WRAP_ADD(runway_length, 1U);
	} else {
		if (arrow_code != LEGACY_U8_MAX && runway_length > 3 &&
			byte_45635 != TRACK_CAMERA_RESERVED_INDEX) {
			previous_track_object = &trkObjectList[previous_tile_element];
			previous_info = &previous_track_object->
				ss_trkObjInfoPtr[previous_subtype];
			opponent_path_offset = (legacy_u16)(
				(legacy_u8)previous_info->si_opp1 |
				LEGACY_U16_SHL(
					(legacy_u8)previous_info->si_opp2, 8U));
			if (previous_connection_status != 0 &&
				opponent_path_offset != 0)
				camera_vectors = track_vector_from_legacy_offset(
					opponent_path_offset);
			else
				camera_vectors = previous_info->si_cameraDataOffset;
			index = LEGACY_U16_WRAP_MUL(
				(legacy_u8)previous_info->si_arrowType, 2U);
			if (previous_connection_status != 0)
				index = LEGACY_U16_WRAP_ADD(index, 2U);
			else
				index = LEGACY_U16_WRAP_ADD(index, 1U);
			camera_vector = camera_vectors[index];
			if (connection_status != 0)
				arrow_code = byte_3E724[
					LEGACY_S8_FROM_BITS(arrow_code)];
			else
				arrow_code = byte_3E71E[
					LEGACY_S8_FROM_BITS(arrow_code)];
			orientation = (legacy_s16)previous_info->si_arrowOrient;
			track_setup_rotate_vector(&camera_vector, orientation);
			td08_direction_related[byte_45635] =
				previous_connection_status != 0 ?
				(orientation ^ TRACK_ORIENTATION_SOUTH) : orientation;
			trackdata23[byte_45635] = arrow_code;
			if (td15_terr_map_main[terrainrows[previous_row] +
				previous_column] == 6)
				camera_vector.y = LEGACY_S16_WRAP_ADD(
					camera_vector.y, TRACK_CAMERA_HILL_HEIGHT);
			camera_index = (legacy_s16)byte_45635;
			td10_track_check_rel[camera_index].y = camera_vector.y;
			base_position = track_object_base_z(
				previous_track_object, (legacy_u8)previous_row);
			td10_track_check_rel[camera_index].z =
				LEGACY_S16_WRAP_ADD(camera_vector.z, base_position);
			base_position = track_object_base_x(
				previous_track_object, (legacy_u8)previous_column);
			td10_track_check_rel[camera_index].x =
				LEGACY_S16_WRAP_ADD(camera_vector.x, base_position);
			trackdata19[trackrows[previous_row] + previous_column] =
				byte_45635;
			byte_45635 = LEGACY_U8_WRAP_ADD(byte_45635, 1U);
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
	if (connection_status != 0) {
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

	if (tile_entry_point >= 1U && tile_entry_point <= 12U) {
		const struct TRACK_SETUP_STEP* step =
			&track_setup_steps[tile_entry_point];

		column = track_setup_add_s8(column, step->column);
		row = track_setup_add_s8(row, step->row);
		orientation = step->orientation;
	}
	}

	byte_45D90 = (legacy_u8)startcol2;
	byte_45E16 = (legacy_u8)startrow2;
	camera_count = (legacy_u16)LEGACY_S16_DIV_OR_ZERO(
		track_pieces_counter, 3);
	if (camera_count > TRACK_CAMERA_COUNT_MAX)
		camera_count = TRACK_CAMERA_COUNT_MAX;
	byte_4616E = (legacy_u8)camera_count;
	for (index = 0; index < TRACK_SETUP_TILE_COUNT; index++)
		subtype_by_piece[index] = 0;
	camera_index = 0;
	for (sample_index = 0; sample_index < byte_4616E; sample_index++) {
		sampled_piece = LEGACY_S16_FROM_BITS((legacy_u16)
			LEGACY_S32_DIV_OR_ZERO(
				LEGACY_S32_WRAP_MUL(
					(legacy_s32)track_pieces_counter,
					(legacy_s32)sample_index),
				(legacy_s32)(legacy_u16)byte_4616E));
		column = LEGACY_S8_FROM_BITS(
			(legacy_u8)td21_col_from_path[sampled_piece]);
		row = LEGACY_S8_FROM_BITS(
			(legacy_u8)td22_row_from_path[sampled_piece]);
		tile_index = terrainrows[row] + column;
		if (subtype_by_piece[tile_index] != 0)
			continue;
		subtype_by_piece[tile_index] = 1;
		tile_element = (legacy_u8)td17_trk_elem_ordered[sampled_piece];
		subtype = (legacy_u8)trackdata18[sampled_piece] &
			TRACK_PIECE_SUBTYPE_MASK;
		connection_status = ((legacy_u8)trackdata18[sampled_piece] &
			TRACK_PIECE_REVERSE_FLAG) != 0;
		track_object = &trkObjectList[tile_element];
		current_info = &track_object->ss_trkObjInfoPtr[subtype];
		opponent_path_offset = (legacy_u16)(
			(legacy_u8)current_info->si_opp1 |
			LEGACY_U16_SHL((legacy_u8)current_info->si_opp2, 8U));
		if (connection_status != 0 && opponent_path_offset != 0)
			camera_vectors = track_vector_from_legacy_offset(
				opponent_path_offset);
		else
			camera_vectors = current_info->si_cameraDataOffset;
		index = LEGACY_U16_WRAP_MUL(
			(legacy_u8)current_info->si_arrowType, 2U);
		camera_vector = camera_vectors[index];
		orientation = (legacy_s16)current_info->si_arrowOrient;
		track_setup_rotate_vector(&camera_vector, orientation);
		if (td15_terr_map_main[terrainrows[row] + column] == 6)
			camera_height[camera_index] = TRACK_CAMERA_HILL_HEIGHT;
		else
			camera_height[camera_index] = 0;
		camera_unknown[camera_index] = 0;
		trackdata9[camera_index].y = LEGACY_S16_WRAP_ADD(
			camera_height[camera_index], camera_vector.y);
		base_position = track_object_base_z(
			track_object, (legacy_u8)row);
		trackdata9[camera_index].z = LEGACY_S16_WRAP_ADD(
			base_position, camera_vector.z);
		base_position = track_object_base_x(
			track_object, (legacy_u8)column);
		trackdata9[camera_index].x = LEGACY_S16_WRAP_ADD(
			base_position, camera_vector.x);
		camera_index = LEGACY_S16_WRAP_ADD(camera_index, 1);
	}
	byte_4616E = (legacy_u8)camera_index;
	mmgr_release((legacy_s8 far*)branches);
	return TRACK_SETUP_OK;
}

void init_plantrak(void) {
	legacy_s16 path_z;
	legacy_s16 route_track_index;
	legacy_u16 route_table_offset;
	legacy_u8 route_index;

	init_game_state(-3);
	state.game_inputmode = 2;
	planptr = &plan_memres;
	startcol2 = 1;
	startrow2 = 0x1C;

	td17_trk_elem_ordered[0] = 7;
	td17_trk_elem_ordered[1] = 6;
	td17_trk_elem_ordered[2] = 8;
	td17_trk_elem_ordered[3] = 9;
	td17_trk_elem_ordered[4] = 7;

	td21_col_from_path[0] = 1;
	td21_col_from_path[1] = 0;
	td21_col_from_path[2] = 0;
	td21_col_from_path[3] = 1;
	td21_col_from_path[4] = 1;

	td22_row_from_path[0] = startrow2;
	td22_row_from_path[1] = startrow2;
	td22_row_from_path[2] = LEGACY_U8_WRAP_ADD(startrow2, 1U);
	td22_row_from_path[3] = LEGACY_U8_WRAP_ADD(startrow2, 1U);
	td22_row_from_path[4] = startrow2;

	trackdata18[0] = 0;
	trackdata18[1] = 0;
	trackdata18[2] = 0;
	trackdata18[3] = 0;
	trackdata18[4] = 0;

	trackdata3[0x00] = 0; trackdata3[0x01] = 0;
	trackdata3[0x02] = 1; trackdata3[0x03] = 0;
	trackdata3[0x04] = 2; trackdata3[0x05] = 0;
	trackdata3[0x06] = 3; trackdata3[0x07] = 0;
	trackdata3[0x08] = 4; trackdata3[0x09] = 0;
	trackdata3[0x0A] = 1; trackdata3[0x0B] = 0;
	trackdata3[0x0C] = 2; trackdata3[0x0D] = 0;
	trackdata3[0x0E] = 3; trackdata3[0x0F] = 0;
	trackdata3[0x10] = 4; trackdata3[0x11] = 0;
	trackdata3[0x12] = 1; trackdata3[0x13] = 0;
	trackdata3[0x14] = 2; trackdata3[0x15] = 0;
	trackdata3[0x16] = 3; trackdata3[0x17] = 0;
	trackdata3[0x18] = 4; trackdata3[0x19] = 0;
	trackdata3[0x1A] = 0; trackdata3[0x1B] = 0;
	trackdata3[0x1C] = 1; trackdata3[0x1D] = 0;
	trackdata3[0x1E] = 2; trackdata3[0x1F] = 0;
	trackdata3[0x20] = 3; trackdata3[0x21] = 0;
	trackdata3[0x22] = 0; trackdata3[0x23] = 0;

	oppnentSped[0] = 0xC8;
	path_z = LEGACY_S16_WRAP_ADD(trackpos[0x1C], 0x012E);
	init_carstate_from_simd(
		&state.opponentstate,
		&simd_opponent,
		1,
		(legacy_s32)0x00017700L,
		0L,
		LEGACY_S32_SHL((legacy_s32)path_z, 6U),
		0);

	route_index = (legacy_u8)state.opponentstate.field_CE;
	state.opponentstate.field_CE = LEGACY_S8_WRAP_ADD(route_index, 1);
	opponent_route_advance((legacy_s16)route_index);
}
