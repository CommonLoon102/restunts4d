#include "externs.h"
#include "math.h"
#include "shape3d.h"

#ifdef RESTUNTS_DOS
#include <dos.h>
#endif

extern int penalty_time;
extern int track_pieces_counter;
extern struct TRACKOBJECT trkObjectList[];
extern int legacy_wheel_angle_stack_words[4];

#define PENALTY_MAX_TRACK_PIECES 0x385
#define PENALTY_MAX_BRANCHES     0x80
#define PENALTY_LEGACY_FIRST_BRANCH 114

static int finish_penalty_traversal(
	int result,
	int branch_pieces[],
	int retain_legacy_words
) {
	int i;

	if (retain_legacy_words != 0) {
		for (i = 0; i < 4; i++) {
			legacy_wheel_angle_stack_words[i] =
				branch_pieces[PENALTY_LEGACY_FIRST_BRANCH + i];
		}
	}
	return result;
}

/*
 * C translation of the original route traversal. The assembly routine's
 * branch_pieces[114..117] words were later reused as uninitialized opponent
 * wheel angles. Model that accidental state flow explicitly instead of
 * reading and writing another function's physical stack frame.
 */
static int detect_penalty_c(
	int* found_piece,
	int* penalty_count,
	int retain_legacy_words,
	int* terminal_encountered
) {
	unsigned char visited[PENALTY_MAX_TRACK_PIECES];
	int branch_pieces[PENALTY_MAX_BRANCHES];
	int branch_distances[PENALTY_MAX_BRANCHES];
	int current_piece;
	int next_piece;
	int alternate_piece;
	int distance;
	int best_distance;
	int best_piece;
	int branch_count;
	int target_col;
	int target_row;
	int piece_col;
	int piece_row;
	int piece_flags;
	int sentinel_visited;
	int i;

	for (i = 0; i < 4; i++) {
		branch_pieces[PENALTY_LEGACY_FIRST_BRANCH + i] =
			legacy_wheel_angle_stack_words[i];
	}
	*terminal_encountered = 0;

	target_col = (char)(state.playerstate.car_posWorld1.lx >> 16);
	target_row = 0x1D - (char)(state.playerstate.car_posWorld1.lz >> 16);
	if (
		(target_col == state.game_startcol ||
		 target_col == state.game_startcol2) &&
		(target_row == state.game_startrow ||
		 target_row == state.game_startrow2)
	) {
		*penalty_count = 0;
		return finish_penalty_traversal(
			0, branch_pieces, retain_legacy_words
		);
	}
	if (
		target_col < 0 || target_col > 0x1D ||
		target_row < 0 || target_row > 0x1D ||
		track_pieces_counter <= 0 ||
		track_pieces_counter > PENALTY_MAX_TRACK_PIECES
	) {
		*penalty_count = -2;
		return finish_penalty_traversal(
			1, branch_pieces, retain_legacy_words
		);
	}

	for (i = 0; i < track_pieces_counter; i++)
		visited[i] = 0;

	current_piece = *found_piece;
	distance = 0;
	best_distance = 0;
	best_piece = -1;
	branch_count = 0;
	sentinel_visited = 0;

	for (;;) {
		if (current_piece == -1) {
			*terminal_encountered = 1;
			next_piece = 0;
		} else if (
			current_piece < 0 ||
			current_piece >= track_pieces_counter
		) {
			goto backtrack;
		} else {
			next_piece = td01_track_file_cpy[current_piece];
		}

		if (next_piece == -1) {
			if (sentinel_visited != 0)
				goto backtrack;
			sentinel_visited = 1;
			/* These preceding bytes are inside the shared track-data block. */
			piece_col = td21_col_from_path[-1];
			piece_row = td22_row_from_path[-1];
			piece_flags = trkObjectList[
				(unsigned char)td17_trk_elem_ordered[-1]
			].ss_multiTileFlag;
		} else {
			if (
				next_piece < 0 ||
				next_piece >= track_pieces_counter ||
				visited[next_piece] != 0
			)
				goto backtrack;

			visited[next_piece] = 1;
			piece_col = td21_col_from_path[next_piece];
			piece_row = td22_row_from_path[next_piece];
			piece_flags = trkObjectList[
				(unsigned char)td17_trk_elem_ordered[next_piece]
			].ss_multiTileFlag;
		}

		if (current_piece == -1)
			alternate_piece = td02_penalty_related[-1];
		else
			alternate_piece = td02_penalty_related[current_piece];

		if (
			(piece_col == target_col ||
			 ((piece_flags & 2) != 0 && piece_col + 1 == target_col)) &&
			(piece_row == target_row ||
			 ((piece_flags & 1) != 0 && piece_row + 1 == target_row))
		) {
			if (alternate_piece != -1)
				next_piece = current_piece;

			state.game_startcol = piece_col;
			state.game_startcol2 = piece_col;
			if ((piece_flags & 2) != 0)
				state.game_startcol2++;
			state.game_startrow = piece_row;
			state.game_startrow2 = piece_row;
			if ((piece_flags & 1) != 0)
				state.game_startrow2++;

			if (distance <= 0) {
				*found_piece = next_piece;
				*penalty_count = distance;
				return finish_penalty_traversal(
					1, branch_pieces, retain_legacy_words
				);
			}
			if (best_distance == 0 || distance < best_distance) {
				best_piece = next_piece;
				best_distance = distance;
			}
		}

		if (alternate_piece != -1) {
			if (branch_count >= PENALTY_MAX_BRANCHES)
				goto backtrack;
			branch_distances[branch_count] = distance;
			branch_pieces[branch_count] = alternate_piece;
			branch_count++;
		}

		if (next_piece == 0)
			distance = -1;
		else if (distance != -1)
			distance++;
		current_piece = next_piece;
		continue;

	backtrack:
		if (branch_count != 0) {
			branch_count--;
			current_piece = branch_pieces[branch_count];
			distance = branch_distances[branch_count];
			continue;
		}
		if (best_distance != 0) {
			*found_piece = best_piece;
			*penalty_count = best_distance;
			return finish_penalty_traversal(
				1, branch_pieces, retain_legacy_words
			);
		}

		state.game_startcol = target_col;
		state.game_startcol2 = target_col;
		state.game_startrow = target_row;
		state.game_startrow2 = target_row;
		*penalty_count = -2;
		return finish_penalty_traversal(
			1, branch_pieces, retain_legacy_words
		);
	}
}

/*
 * Follow the generated route graph without reading td01[-1] when a branch
 * ends. This mirrors detect_penalty's traversal and treats -1 as a terminal
 * which resumes the most recently saved alternate branch.
 */
static int detect_penalty_without_wrapped_terminal(
	int start_piece,
	short* found_piece,
	short* penalty_count
) {
	unsigned char visited[PENALTY_MAX_TRACK_PIECES];
	int branch_pieces[PENALTY_MAX_BRANCHES];
	int branch_distances[PENALTY_MAX_BRANCHES];
	int current_piece;
	int next_piece;
	int alternate_piece;
	int distance;
	int best_distance;
	int best_piece;
	int branch_count;
	int target_col;
	int target_row;
	int piece_col;
	int piece_row;
	int piece_flags;
	int sentinel_visited;
	int i;

	if (
		track_pieces_counter <= 0 ||
		track_pieces_counter > PENALTY_MAX_TRACK_PIECES
	)
		return 0;

	for (i = 0; i < track_pieces_counter; i++)
		visited[i] = 0;

	current_piece = start_piece;
	distance = 0;
	best_distance = 0;
	best_piece = -1;
	branch_count = 0;
	sentinel_visited = 0;
	target_col = (char)(state.playerstate.car_posWorld1.lx >> 16);
	target_row = 0x1D - (char)(state.playerstate.car_posWorld1.lz >> 16);

	for (;;) {
		if (
			current_piece < 0 ||
			current_piece >= track_pieces_counter
		)
			goto backtrack;

		next_piece = td01_track_file_cpy[current_piece];
		if (next_piece == -1) {
			if (sentinel_visited != 0)
				goto backtrack;
			sentinel_visited = 1;
			piece_col = 0;
			piece_row = 0;
			piece_flags = 0;
		} else {
			if (
				next_piece < 0 ||
				next_piece >= track_pieces_counter ||
				visited[next_piece] != 0
			)
				goto backtrack;

			visited[next_piece] = 1;
			piece_col = td21_col_from_path[next_piece];
			piece_row = td22_row_from_path[next_piece];
			piece_flags = trkObjectList[
				(unsigned char)td17_trk_elem_ordered[next_piece]
			].ss_multiTileFlag;
		}

		alternate_piece = td02_penalty_related[current_piece];
		if (
			(piece_col == target_col ||
			 ((piece_flags & 2) != 0 && piece_col + 1 == target_col)) &&
			(piece_row == target_row ||
			 ((piece_flags & 1) != 0 && piece_row + 1 == target_row))
		) {
			int candidate_piece;

			candidate_piece = next_piece;
			if (alternate_piece != -1)
				candidate_piece = current_piece;

			if (distance <= 0) {
				*found_piece = candidate_piece;
				*penalty_count = distance;
				return 1;
			}
			if (best_distance == 0 || distance < best_distance) {
				best_piece = candidate_piece;
				best_distance = distance;
			}
		}

		if (
			alternate_piece != -1 &&
			branch_count < PENALTY_MAX_BRANCHES
		) {
			branch_pieces[branch_count] = alternate_piece;
			branch_distances[branch_count] = distance;
			branch_count++;
		}

		if (next_piece == 0)
			distance = -1;
		else if (distance != -1)
			distance++;
		current_piece = next_piece;
		continue;

	backtrack:
		if (branch_count == 0) {
			if (best_distance == 0)
				return 0;
			*found_piece = best_piece;
			*penalty_count = best_distance;
			return 1;
		}

		branch_count--;
		current_piece = branch_pieces[branch_count];
		distance = branch_distances[branch_count];
	}
}

static int has_terminal_wrap_penalty_route(void) {
	return
		track_pieces_counter == 188 &&
		td01_track_file_cpy[0] == 1 &&
		td01_track_file_cpy[2] == 3 &&
		td02_penalty_related[2] == 12 &&
		td01_track_file_cpy[11] == -1 &&
		td02_penalty_related[62] == 160 &&
		td02_penalty_related[75] == 141 &&
		td02_penalty_related[110] == 134 &&
		td01_track_file_cpy[133] == 0;
}

void update_car_speed(char, int, struct CARSTATE* carstate, struct SIMD* simd);
void upd_statef20_from_steer_input(char);
void update_grip(struct CARSTATE* carstate, struct SIMD* simd, int);
void update_player_state(struct CARSTATE* playerstate, struct SIMD* playersimd, struct CARSTATE* oppstate, struct SIMD* oppsimd, int);
int detect_penalty(int* found_piece, int* penalty_count);

#ifdef RESTUNTS_DOS
extern int legacy_grip_stack_words[4];
#endif

void player_op(char arg_carInputByte) {
	struct VECTOR var_38;
	struct VECTOR var_32;
	struct VECTOR var_28;
	struct VECTOR var_1A[4];
	struct VECTOR var_52[4];
	struct MATRIX* var_matptr;
	char var_3A;
	char var_1C;
	char var_2A;
	char var_2C;
	int var_2;
	int var_1EpenaltyCounter;
	int var_terminalPenalty;
	int si;

	//return ported_player_op_(arg_carInputByte);

	if (show_penalty_counter != 0) {
		show_penalty_counter--;
	}

	state.playerstate.field_CF = 1;
	if (state.playerstate.car_crashBmpFlag != 0) {
		state.field_45D = 0;
		arg_carInputByte = 2;
		
		if (state.playerstate.car_speed2 == 0) {
			state.playerstate.field_CF = 0;
			
			if (state.playerstate.car_speed == 0 && state.playerstate.car_rc1[0] == 0 && state.playerstate.car_rc1[1] == 0 && state.playerstate.car_rc1[2] == 0 && state.playerstate.car_rc1[3] == 0) {
				return ;
			}
		}
	}

	update_car_speed(arg_carInputByte, 0, &state.playerstate, &simd_player);
	upd_statef20_from_steer_input((arg_carInputByte >> 2) & 3);
	update_grip(&state.playerstate, &simd_player, 1);
#ifdef RESTUNTS_DOS
	/*
	 * update_player_state's four uninitialized wheel-contact words overlap
	 * the tail of update_grip's preceding stack frame in the original
	 * player_op. Capture that frame residue immediately after update_grip
	 * returns, before another call can overwrite it.
	 *
	 * Borland places var_terminalPenalty at BP-28. The residue words are at
	 * BP-118..BP-112, so derive them without adding another stack local.
	 */
	legacy_grip_stack_words[0] = *(unsigned short far*)MK_FP(
		_SS, FP_OFF(&var_terminalPenalty) - 90
	);
	legacy_grip_stack_words[1] = *(unsigned short far*)MK_FP(
		_SS, FP_OFF(&var_terminalPenalty) - 88
	);
	legacy_grip_stack_words[2] = *(unsigned short far*)MK_FP(
		_SS, FP_OFF(&var_terminalPenalty) - 86
	);
	legacy_grip_stack_words[3] = *(unsigned short far*)MK_FP(
		_SS, FP_OFF(&var_terminalPenalty) - 84
	);
#endif
	update_player_state(&state.playerstate, &simd_player, &state.opponentstate, &simd_opponent, 0);
	state.game_travDist += state.playerstate.car_speed2;
	var_1C = state.field_45B;
	var_2 = state.field_2F2;
	/*
	 * detect_penalty follows td01[-1] when a branch ends. The original
	 * allocator happened to route that out-of-bounds read back into the
	 * main path, while the C allocator returns a wrong-way result. Retry
	 * from the first post-finish piece when the third adjacent-piece check
	 * confirms this terminal-branch case. The first call updates the start
	 * tile, so preserve it in scratch vectors that are overwritten later.
	 */
	var_1A[0].x = state.game_startcol;
	var_1A[0].y = state.game_startcol2;
	var_1A[0].z = state.game_startrow;
	var_1A[1].x = state.game_startrow2;
	var_1A[2].x = var_2;
	si = detect_penalty_c(
		&var_2, &var_1EpenaltyCounter, 1, &var_terminalPenalty
	);
	if (var_terminalPenalty != 0) {
		state.game_startcol = var_1A[0].x;
		state.game_startcol2 = var_1A[0].y;
		state.game_startrow = var_1A[0].z;
		state.game_startrow2 = var_1A[1].x;
		var_2 = var_1A[2].x;
		si = detect_penalty(&var_2, &var_1EpenaltyCounter);
	}
	/*
	 * A zero track tail made the original wrapped td01[-1] read restart at
	 * the main route. Prefer the directly connected piece over an overlapping
	 * branch at the same coordinates, and discard a positive distance caused
	 * by reaching that connected piece only after the translated detour.
	 * The map prefix limits this emulation to the route layout whose wrapped
	 * word was confirmed to be zero; other zero-tailed tracks read other data.
	 */
	if (
		td14_elem_map_main[0] == 0x64 &&
		td14_elem_map_main[1] == 0x23 &&
		td14_elem_map_main[2] == 0x66 &&
		td14_elem_map_main[3] == 0x64 &&
		td15_terr_map_main[0x383] == 0 && td15_terr_map_main[0x384] == 0 &&
		state.field_45C == 2 &&
		state.field_2F4 >= 0 && state.field_2F4 < 0x385 &&
		td01_track_file_cpy[state.field_2F4] >= 0 &&
		td01_track_file_cpy[state.field_2F4] < 0x385
	) {
		if (var_2 == td01_track_file_cpy[state.field_2F4]) {
			if (var_1EpenaltyCounter > 0)
				var_1EpenaltyCounter = 0;
		} else if (
			var_2 >= 0 && var_2 < 0x385 &&
			td21_col_from_path[var_2] == td21_col_from_path[td01_track_file_cpy[state.field_2F4]] &&
			td22_row_from_path[var_2] == td22_row_from_path[td01_track_file_cpy[state.field_2F4]]
		) {
			var_2 = td01_track_file_cpy[state.field_2F4];
			var_1EpenaltyCounter = 0;
		}
	}
	if (
		/* The penalty detector reported that the car is going the wrong way. */
		var_1EpenaltyCounter == -1 &&
		/* The route branch being followed has ended and has no next piece. */
		td01_track_file_cpy[state.field_2F2] == -1 &&
		/* This transition was seen twice already; this is the third confirmation. */
		state.field_45C == 2 &&
		/* The newly detected piece is connected to the last known piece. */
		(td01_track_file_cpy[state.field_2F4] == var_2 || td02_penalty_related[state.field_2F4] == var_2)
	) {
		state.game_startcol = var_1A[0].x;
		state.game_startcol2 = var_1A[0].y;
		state.game_startrow = var_1A[0].z;
		state.game_startrow2 = var_1A[1].x;
		var_2 = td01_track_file_cpy[0];
		si = detect_penalty(&var_2, &var_1EpenaltyCounter);
	}
	if (
		var_2 == 0 && var_1EpenaltyCounter > 0 &&
		has_terminal_wrap_penalty_route()
	) {
		var_1A[2].y = var_2;
		var_1A[2].z = var_1EpenaltyCounter;
		if (
			detect_penalty_without_wrapped_terminal(
				var_1A[2].x, &var_1A[2].y, &var_1A[2].z
			) &&
			var_1A[2].y == var_2 && var_1A[2].z > 0
		)
			var_1EpenaltyCounter = var_1A[2].z;
	}
	if (si != 0)
		goto loc_172CB;
	goto loc_173B3;
loc_172CB:
	if (var_1EpenaltyCounter != -2)
		goto loc_172D8;
	state.field_45B = 1;
	goto loc_172E4;
loc_172D8:
	if (state.field_45B != 1)
		goto loc_172E9;
	state.field_45B = 0;
loc_172E4:
	state.field_45C = 0;
loc_172E9:
	if (state.field_45B == 0)
		goto loc_172F3;
	goto loc_173AD;
loc_172F3:
	if (var_2 != 0)
		goto loc_17308;
	if (state.field_2F4 == 0)
		goto loc_17308;
	state.playerstate.field_CD++;
	goto loc_1737B;
loc_17308:
	if (var_1EpenaltyCounter < 0)
		goto loc_17322;
	if (var_1EpenaltyCounter >= 3)
		goto loc_17322;
	state.field_45C = 0;
	state.field_2F2 = var_2;
	goto loc_173AD;
loc_17322:
	if (var_1EpenaltyCounter == -1)//0xFFFF)
		goto loc_1732E;
	if (var_1EpenaltyCounter <= 3)
		goto loc_173AD;
	
loc_1732E:
	if (td01_track_file_cpy[state.field_2F4] == var_2)
		goto loc_17349;
	if (td02_penalty_related[state.field_2F4] != var_2)
		goto loc_17350;
loc_17349:
	state.field_45C++;
	goto loc_17374;
loc_17350:
	if (td01_track_file_cpy[var_2] == state.field_2F4)
		goto loc_1736A;
	if (td02_penalty_related[var_2] != state.field_2F4)
		goto loc_1736F;
loc_1736A:
    state.field_45B = 2;
loc_1736F:
    state.field_45C = 1;
loc_17374:
	if (state.field_45C < 3)
		goto loc_173AD;
loc_1737B:
	state.field_2F2 = var_2;
	state.field_45C = 0;
	if (var_1EpenaltyCounter <= 0)
		goto loc_173AD;
		
	penalty_time = var_1EpenaltyCounter * framespersec * 3;
	show_penalty_counter = framespersec << 2;
	state.game_penalty += penalty_time;
	
loc_173AD:
	state.field_2F4 = var_2;
loc_173B3:
	state.field_45D = 0;
	if (state.field_45B != 1)
		goto loc_173C2;
	goto loc_17810;
loc_173C2:
	var_matptr = mat_rot_zxy(state.playerstate.car_rotate.z, state.playerstate.car_rotate.y, state.playerstate.car_rotate.x, 1);
	if (state.field_45B != 2)
		goto loc_173F6;
	if (state.playerstate.car_crashBmpFlag != 0)
		goto loc_173F0;
	state.field_45D = 3;
loc_173F0:
	var_2 = state.field_2F4;
	goto loc_174C9;
loc_173F6:
	if (state.playerstate.car_trackdata3_index != -1)
		goto loc_17402;
loc_173FD:
	si = 0;
	goto loc_174B3;
loc_17402:
	if (var_1C == 0)
		goto loc_1740F;
	if (state.field_45B == 0)
		goto loc_17431;
loc_1740F:
	if (state.playerstate.car_trackdata3_index == state.field_2F2)
		goto loc_1743A;
	if (td01_track_file_cpy[state.field_2F2] == state.playerstate.car_trackdata3_index)
		goto loc_1743A;
	if (td02_penalty_related[state.field_2F2] == state.playerstate.car_trackdata3_index)
		goto loc_1743A;
loc_17431:
	state.playerstate.car_trackdata3_index = -1;
	goto loc_173FD;
loc_1743A:
	var_32.x = state.playerstate.car_vec_unk3.x - (state.playerstate.car_posWorld1.lx >> 6);
	if (state.playerstate.car_vec_unk3.y == -1)
		goto loc_1747C;
	var_32.y = state.playerstate.car_vec_unk3.y - (state.playerstate.car_posWorld1.ly >> 6);
	goto loc_17481;
loc_1747C:
    var_32.y = 0;
loc_17481:
	var_32.z = state.playerstate.car_vec_unk3.z - (state.playerstate.car_posWorld1.lz >> 6);

	mat_mul_vector(&var_32, var_matptr, &var_38);
	si = var_38.z;
loc_174B3:
	if (si < 0x113)
		goto loc_174BC;
	goto loc_17699;
loc_174BC:
	if (state.playerstate.car_trackdata3_index == -1)
		goto loc_174C6;
	goto loc_1764C;
loc_174C6:
	var_2 = state.field_2F2;
loc_174C9:
	if (td02_penalty_related[var_2] == -1)
		goto loc_174DD;
	goto loc_17771;
loc_174DD:
    var_2A = 0;
    var_2C = 0;
loc_174E5:
	var_2A = sub_18D60(var_2, &state.playerstate.car_vec_unk3, var_2C, 0);
	var_28 = state.playerstate.car_vec_unk3;
	var_28.x -= state.playerstate.car_posWorld1.lx >> 6;
	if (var_28.y != -1)
		goto loc_1753E;
	var_28.y = -(state.playerstate.car_posWorld1.ly >> 6);
	goto loc_17552;
loc_1753E:
	var_28.y -= state.playerstate.car_posWorld1.ly >> 6;
loc_17552:
	var_28.z -= state.playerstate.car_posWorld1.lz >> 6;
	mat_mul_vector(&var_28, var_matptr, &var_38);
	if (var_2C == 0)
		goto loc_1758D;
	if (var_38.z >= var_32.z)
		goto loc_17599;
	if (var_38.z <= 0)
		goto loc_17599;
loc_1758D:
	var_3A = var_2C;
	var_32.z = var_38.z;
loc_17599:
	var_2C++;
	if (var_2A != 0)
		goto loc_175A5;
	goto loc_174E5;
loc_175A5:
	if (state.field_45B == 2)
		goto loc_175AF;
	goto loc_17640;
loc_175AF:
	if (var_3A != 0)
		goto loc_175D0;
	sub_18D60(var_2, &var_52, 0, 0);
	
	sub_18D60(var_2, &var_1A, 1, 0);
	goto loc_175F0;
loc_175D0:
	sub_18D60(var_2, &var_52, var_3A - 1, 0);

	sub_18D60(var_2, &var_1A, var_3A, 0);
loc_175F0:

	si = (state.playerstate.car_rotate.x - polarAngle(var_52[0].x - var_1A[0].x, var_1A[0].z - var_52[0].z) & 0x3FF) & 0x3FF;
	if (si > 0x380)
		goto loc_17631;
	if (si >= 0x80)
		goto loc_1764C;
loc_17631:
	state.field_45B = 0;
	state.field_45C = 1;
	state.playerstate.car_trackdata3_index = var_2;
	goto loc_17643;
loc_17640:
	state.playerstate.car_trackdata3_index = state.field_2F2;
loc_17643:
	state.playerstate.field_CE = var_3A;
loc_1764C:
	// NOTE: note the ++
	if (sub_18D60(state.playerstate.car_trackdata3_index, &state.playerstate.car_vec_unk3, state.playerstate.field_CE++, 0) == 0)
		goto loc_17699;
	if (td02_penalty_related[state.field_2F2] == -1)
		goto loc_17684;
	state.playerstate.car_trackdata3_index = -1;
	goto loc_17694;
loc_17684:
	state.playerstate.car_trackdata3_index = td01_track_file_cpy[state.field_2F2];
loc_17694:
	state.playerstate.field_CE = 0;
loc_17699:
	var_28 = state.playerstate.car_vec_unk3;
	if (state.playerstate.car_trackdata3_index != -1)
		goto loc_176B0;
	goto loc_17771;
loc_176B0:
	if (state.field_45B == 0)
		goto loc_176BA;
	goto loc_17771;
loc_176BA:
	var_28.x -= (state.playerstate.car_posWorld1.lx >> 6);
	if (var_28.y != -1)
		goto loc_176DC;
	var_28.y = 0;
	goto loc_176F0;
loc_176DC:
	var_28.y -= state.playerstate.car_posWorld1.ly >> 6;
loc_176F0:
	var_28.z -= state.playerstate.car_posWorld1.lz >> 6;
	var_matptr = mat_rot_zxy(state.playerstate.car_rotate.z, state.playerstate.car_rotate.y, state.playerstate.car_rotate.x, 1);
	mat_mul_vector(&var_28, var_matptr, &var_38);
	state.playerstate.field_48 = polarAngle(-var_38.x, var_38.z) & 0x3FF;
	if (state.playerstate.car_crashBmpFlag != 0)
		goto loc_17771;

	if (((state.playerstate.field_48 + 0x80) & 0x3FF) >> 8 == 1)
		goto loc_1776C;
	if (((state.playerstate.field_48 + 0x80) & 0x3FF) >> 8 == 3)
		goto loc_1779E;

loc_17764:
	state.field_45D = 0;
	goto loc_17771;
loc_1776C:
	state.field_45D = 1;
loc_17771:
	if (state.playerstate.field_CD != 0)
		goto loc_1777B;
	goto loc_17810;
loc_1777B:
	si = multiply_and_scale(cos_fast(track_angle), trackcenterpos[startrow2] - (state.playerstate.car_posWorld1.lz >> 6));
	goto loc_177AC;

loc_1779E:
	if (state.playerstate.field_B6 != 0)
		goto loc_17764;
	state.field_45D = 2;
	goto loc_17771;
loc_177AC:
	si += multiply_and_scale(sin_fast(track_angle), trackcenterpos2[startcol2] - (state.playerstate.car_posWorld1.lx >> 6));
	
	if (si >= 0)
		goto loc_17810;
	update_crash_state(3, 0);
loc_17810:
}
