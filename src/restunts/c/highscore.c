#include "fileio.h"
#include "legacy.h"
#include "memmgr.h"
#include "menu_internal.h"
#include "platform.h"
#include "resource.h"
#include "shape2d.h"

static legacy_u8 ranking_highlight;
legacy_s16 ranking_entry_order[HIGHSCORE_ENTRY_COUNT];

extern legacy_s8 gnam_string[];
extern legacy_s8 gsna_string[];
extern legacy_s8 unk_46464[];
extern legacy_s8 byte_459E0[];

void print_int_as_string_maybe(legacy_s8* destination, legacy_s16 value,
	legacy_s16 zero_pad, legacy_s16 width);
void format_frame_as_string(legacy_s8* destination, legacy_s16 frame_count,
	legacy_s16 include_hundredths);
legacy_s16 call_read_line(legacy_s8* text, legacy_s16 max_characters, legacy_s16 x, legacy_s16 y,
	legacy_u32 timeout);
legacy_s16 get_super_random(void);

struct RECTANGLE* hiscore_draw_text(legacy_s8* text, legacy_s16 x, legacy_s16 y, legacy_s16 color,
	legacy_s16 shadow_color)
{
	word_42250.left = LEGACY_S16_WRAP_SUB(x, 1);
	word_42250.right = LEGACY_S16_WRAP_ADD(
		LEGACY_S16_WRAP_ADD(x, font_op2(text)), 1);
	word_42250.top = LEGACY_S16_WRAP_SUB(y, 1);
	word_42250.bottom = LEGACY_S16_WRAP_ADD(
		LEGACY_S16_WRAP_ADD(y, fontdef_unk_0E), 1);
	font_set_unk(shadow_color, 0);
	font_draw_text(text, LEGACY_S16_WRAP_ADD(x, 1),
		LEGACY_S16_WRAP_ADD(y, 1));
	font_draw_text(text, LEGACY_S16_WRAP_SUB(x, 1),
		LEGACY_S16_WRAP_ADD(y, 1));
	font_draw_text(text, LEGACY_S16_WRAP_ADD(x, 1),
		LEGACY_S16_WRAP_SUB(y, 1));
	font_draw_text(text, LEGACY_S16_WRAP_SUB(x, 1),
		LEGACY_S16_WRAP_SUB(y, 1));
	font_set_unk(color, 0);
	font_draw_text(text, LEGACY_S16_FROM_BITS((legacy_u16)x),
		LEGACY_S16_FROM_BITS((legacy_u16)y));
	return &word_42250;
}

void far* sub_29A86(legacy_s16 operation, const legacy_s8* filename,
	void far* destination)
{
	void far* result;

	if (operation == 10)
		return file_read_nofatal(filename, destination);
	if (operation != 9)
		return 0;
	do {
		result = file_read_nofatal(filename, destination);
		if (result != 0)
			return result;
	} while (do_dea_textres() != 2);
	return 0;
}

legacy_s16 highscore_write_a(legacy_s16 create_default)
{
	struct HIGHSCORE_ENTRY record;
	legacy_u8* record_bytes;
	struct HIGHSCORE_ENTRY far* scores;
	void far* read_result;
	legacy_u16 entry;
	legacy_u16 offset;

	ranking_highlight = 0xFFU;
	for (entry = 0; entry < HIGHSCORE_ENTRY_COUNT; entry++)
		ranking_entry_order[entry] = entry;
	file_build_path(byte_3B80C, gameconfig.game_trackname,
		".hig", g_path_buf);
	if (create_default == 0) {
		g_is_busy = 1;
		read_result = sub_29A86(10, g_path_buf, td11_highscores);
		g_is_busy = 0;
		return read_result == 0 ? 1 : 0;
	}

	record_bytes = (legacy_u8*)&record;
	for (offset = 0; offset < HIGHSCORE_COMBINED_NAME_TEXT_BYTES; offset++)
		record_bytes[offset] = '.';
	record_bytes[HIGHSCORE_COMBINED_NAME_TEXT_BYTES] = 0;
	record.car_flag = 0;
	record.opponent[0] = '.';
	record.opponent[1] = '.';
	record.opponent[2] = '/';
	for (offset = 3U; offset < HIGHSCORE_OPPONENT_TEXT_BYTES; offset++)
		record.opponent[offset] = '.';
	record.opponent[HIGHSCORE_OPPONENT_TEXT_BYTES] = 0;
	record.time = 0xFFFFU;
	scores = (struct HIGHSCORE_ENTRY far*)td11_highscores;
	for (entry = 0; entry < HIGHSCORE_ENTRY_COUNT; entry++)
		scores[entry] = record;
	return file_write_fatal(g_path_buf, td11_highscores,
		HIGHSCORE_TABLE_SIZE_BYTES) != 0;
}

void highscore_write_b(void)
{
	struct HIGHSCORE_ENTRY ordered_scores[HIGHSCORE_ENTRY_COUNT];
	struct HIGHSCORE_ENTRY far* scores;
	legacy_u16 entry;
	legacy_u16 source_entry;

	scores = (struct HIGHSCORE_ENTRY far*)td11_highscores;
	for (entry = 0; entry < HIGHSCORE_ENTRY_COUNT; entry++) {
		source_entry = (legacy_u16)ranking_entry_order[entry];
		ordered_scores[entry] = scores[source_entry];
	}
	file_build_path(byte_3B80C, gameconfig.game_trackname,
		".hig", g_path_buf);
	g_is_busy = 1;
	(void)file_write_fatal(g_path_buf, ordered_scores,
		HIGHSCORE_TABLE_SIZE_BYTES);
	g_is_busy = 0;
}

void print_highscore_entry(legacy_s16 entry, legacy_u8* text_offsets)
{
	struct HIGHSCORE_ENTRY record;
	struct HIGHSCORE_ENTRY far* scores;
	legacy_u16 output_offset;
	legacy_s16 saved_frame_rate;
	legacy_s16 frame_count;
	legacy_s8 formatted_time[18];
	legacy_s8* output;

	scores = (struct HIGHSCORE_ENTRY far*)td11_highscores;
	record = scores[ranking_entry_order[entry]];

	text_offsets[0] = 0;
	strcpy(&resID_byte1, record.player_name);
	output_offset = (legacy_u16)strlen(&resID_byte1) + 1U;
	text_offsets[1] = (legacy_u8)output_offset;
	strcpy(&resID_byte1 + output_offset, record.car_name);
	output_offset = LEGACY_U16_WRAP_ADD(output_offset,
		(legacy_u16)strlen(&resID_byte1 + output_offset) + 1U);
	text_offsets[2] = (legacy_u8)output_offset;

	output = &resID_byte1 + output_offset;
	*output = 0;
	if (record.car_flag == 1)
		strcat(output, "(");
	strcat(output, record.opponent);
	if (record.car_flag == 1)
		strcat(output, ")");
	output_offset = LEGACY_U16_WRAP_ADD(output_offset,
		(legacy_u16)strlen(output) + 1U);

	saved_frame_rate = framespersec;
	framespersec = 0x14;
	frame_count = LEGACY_S16_FROM_BITS(record.time);
	format_frame_as_string(formatted_time,
		frame_count == -1 ? 0 : frame_count, 1);
	text_offsets[3] = (legacy_u8)output_offset;
	strcpy(&resID_byte1 + output_offset, formatted_time);
	framespersec = saved_frame_rate;
}

void highscore_text_unk(void)
{
	legacy_u8 text_offsets[4];
	legacy_s16 row;
	legacy_u16 entry;
	legacy_s16 color;
	legacy_s8 far* text;

	sprite_copy_wnd_to_1();
	copy_string(&resID_byte1, locate_text_res(mainresptr, "hs1"));
	strcat(&resID_byte1, " '");
	strcat(&resID_byte1, gameconfig.game_trackname);
	strcat(&resID_byte1, "'");
	hiscore_draw_text(&resID_byte1, font_op2_alt(&resID_byte1),
		5, dialog_fnt_colour, 0);

	text = locate_text_res(mainresptr, "hs2");
	copy_string(&resID_byte1, text);
	hiscore_draw_text(&resID_byte1, 0x10, 0x0F,
		dialog_fnt_colour, 0);
	text = locate_text_res(mainresptr, "hs3");
	copy_string(&resID_byte1, text);
	hiscore_draw_text(&resID_byte1, 0x78, 0x0F,
		dialog_fnt_colour, 0);
	text = locate_text_res(mainresptr, "hs5");
	copy_string(&resID_byte1, text);
	hiscore_draw_text(&resID_byte1, 0xE0, 0x0F,
		dialog_fnt_colour, 0);
	text = locate_text_res(mainresptr, "hs4");
	copy_string(&resID_byte1, text);
	hiscore_draw_text(&resID_byte1, 0x110, 0x0F,
		dialog_fnt_colour, 0);

	font_set_fontdef2(fontnptr);
	for (entry = 0; entry < 7U; entry++) {
		print_highscore_entry(entry, text_offsets);
		row = LEGACY_S16_WRAP_ADD(
			LEGACY_U16_WRAP_MUL(entry, 10U), 0x19);
		color = entry == (legacy_u8)ranking_highlight ? dialogarg2 : 0;
		font_set_unk(color, 0);
		font_draw_text(&resID_byte1 + text_offsets[0], 0x10, row);
		font_draw_text(&resID_byte1 + text_offsets[1], 0x78, row);
		font_draw_text(&resID_byte1 + text_offsets[2], 0xE0, row);
		font_draw_text(&resID_byte1 + text_offsets[3], 0x110, row);
	}
	font_set_fontdef();
}

static legacy_u16 read_highscore_u16(legacy_u8 far* address)
{
	return LEGACY_READ_U16_LE(address);
}

void enter_hiscore(legacy_s16 frame_count, void far* prompt, legacy_u8 car_flag)
{
	struct HIGHSCORE_ENTRY record;
	legacy_u8* record_bytes;
	struct HIGHSCORE_ENTRY far* scores;
	legacy_u16 entry;
	legacy_u16 copied;
	legacy_u16 rank;
	legacy_u16 time_bits;
	legacy_s16 positions[2];

	time_bits = (legacy_u16)frame_count;
	if (framespersec == 0x0A)
		time_bits = LEGACY_U16_WRAP_MUL(time_bits, 2U);
	scores = (struct HIGHSCORE_ENTRY far*)td11_highscores;
	if (scores[HIGHSCORE_LAST_ENTRY_INDEX].time <= time_bits) {
		highscore_text_unk();
		return;
	}

	entry = 0;
	while (scores[entry].time <= time_bits) {
		if (entry >= 7U)
			break;
		ranking_entry_order[entry] = (legacy_s16)entry;
		entry++;
	}
	rank = entry;
	ranking_highlight = (legacy_u8)rank;
	while (entry < 6U) {
		ranking_entry_order[entry + 1U] = (legacy_s16)entry;
		entry++;
	}
	ranking_entry_order[rank] = HIGHSCORE_LAST_ENTRY_INDEX;

	record_bytes = (legacy_u8*)&record;
	for (copied = 0; copied < sizeof(record); copied++)
		record_bytes[copied] = 0;
	strcpy(record.car_name, gnam_string);
	record.car_flag = car_flag;
	if (gameconfig.game_opponenttype != 0) {
		strcpy(record.opponent, unk_46464);
		record.opponent[2] = '/';
		strcpy(&record.opponent[3], gsna_string);
	} else {
		strcpy(record.opponent, " ");
	}
	record.time = time_bits;
	scores[HIGHSCORE_LAST_ENTRY_INDEX] = record;

	sprite_copy_wnd_to_1();
	highscore_text_unk();
	sprite_blit_to_video(render_window_sprite, -1);
	show_dialog(3, 0, prompt, 0xFFFFU, 0xFFFFU,
		dialogarg2, positions, 0);
	check_input();
	call_read_line(byte_459E0, 0x10, positions[0], positions[1],
		0x7530UL);
	strcpy(record.player_name, byte_459E0);
	scores[HIGHSCORE_LAST_ENTRY_INDEX] = record;

	sprite_copy_wnd_to_1();
	highscore_text_unk();
	sprite_blit_to_video(render_window_sprite, -1);
	highscore_write_b();
	highscore_text_unk();
}

static void end_hiscore_set_text(legacy_s8 far* resource, legacy_s8* text_id)
{
	copy_string(&resID_byte1, locate_text_res(resource, text_id));
}

static void end_hiscore_append_text(legacy_s8 far* resource, legacy_s8* text_id)
{
	copy_string(&resID_byte1 + strlen(&resID_byte1),
		locate_text_res(resource, text_id));
}

static void end_hiscore_draw_current_text(legacy_s16* y)
{
	hiscore_draw_text(&resID_byte1, font_op2_alt(&resID_byte1), *y,
		dialog_fnt_colour, 0);
	*y = LEGACY_S16_WRAP_ADD(*y, 10);
}

static void end_hiscore_draw_animation_frame(legacy_s8 far* animation_resource,
	legacy_u8 far* frame_sequence, legacy_u8 frame_index,
	legacy_s16 animation_x, legacy_s16 animation_y,
	struct SPRITE far* animation_sprite, legacy_u8 draw_direct_copy)
{
	struct SHAPE2D far* frame_shape;

	aOp01[3] = (legacy_s8)(frame_sequence[frame_index] + '0');
	frame_shape = (struct SHAPE2D far*)locate_shape_fatal(
		animation_resource, aOp01);
	mouse_draw_opaque_check();
	if (video_flag5_is0 != 0) {
		sprite_set_1_from_argptr(animation_sprite);
		shape2d_op_unk5(frame_shape, 0, 0);
		sprite_copy_2_to_1_2();
		sprite_set_1_size(animation_x,
			LEGACY_S16_WRAP_ADD(animation_x,
				LEGACY_S16_WRAP_MUL(shape2d_get_width(frame_shape),
					video_flag1_is1)),
			animation_y,
			LEGACY_S16_WRAP_ADD(animation_y,
				shape2d_get_height(frame_shape)));
		sprite_putimage_and_alt(animation_sprite->sprite_bitmapptr,
			animation_x, animation_y);
		sprite_copy_2_to_1_2();
	} else {
		shape2d_op_unk5(frame_shape, animation_x, animation_y);
	}
	if (draw_direct_copy != 0)
		shape2d_op_unk5(frame_shape, animation_x, animation_y);
	mouse_draw_transparent_check();
}

static void end_hiscore_advance_animation(legacy_s16 delta,
	legacy_s16* timer, legacy_u8* frame, const legacy_u8 far* frame_sequence)
{
	*timer = LEGACY_S16_WRAP_ADD(*timer, delta);
	if (*timer >= 0x1E) {
		*timer = LEGACY_S16_WRAP_SUB(*timer, 0x1E);
		(*frame)++;
		if (frame_sequence[*frame] == 0)
			*frame = 0;
	}
}

static void end_hiscore_update_animation(legacy_s16 delta,
	legacy_s16* timer, legacy_u8* frame, legacy_u8* previous_frame,
	legacy_s8 far* animation_resource, legacy_u8 far* frame_sequence,
	legacy_s16 animation_x, legacy_s16 animation_y,
	struct SPRITE far* animation_sprite, legacy_u8 draw_direct_copy)
{
	end_hiscore_advance_animation(delta, timer, frame, frame_sequence);
	if (*previous_frame != *frame) {
		*previous_frame = *frame;
		end_hiscore_draw_animation_frame(animation_resource,
			frame_sequence, *frame, animation_x, animation_y,
			animation_sprite, draw_direct_copy);
	}
}

static void end_hiscore_draw_opponent_text(legacy_s8 far* opponent_resource,
	legacy_u8 outcome, legacy_u8 text_prefix, legacy_s16 animation_x)
{
	legacy_s8 word[32];
	legacy_s8 text_id[4];
	legacy_s8 far* text;
	legacy_u8 character;
	legacy_u16 resource_index;
	legacy_u16 resource_count;
	legacy_u16 word_length;
	legacy_u16 output_length;
	legacy_u16 copy_index;
	legacy_u16 first_character;
	legacy_s16 line_width;
	legacy_s16 word_width;
	legacy_s16 line_y;
	legacy_s16 selector;

	line_y = 8;
	output_length = 0;
	line_width = 0;
	word_length = 0;
	resource_count = outcome == 2 ? 1U : 3U;
	for (resource_index = 0; resource_index < resource_count;
		resource_index++) {
		if (outcome == 2) {
			text = locate_text_res(opponent_resource, aD4a);
		} else {
			text_id[0] = (legacy_s8)text_prefix;
			text_id[1] = (legacy_s8)('1' + resource_index);
			if (resource_index == 0)
				selector = word_40D40;
			else if (resource_index == 1)
				selector = end_hiscore_random;
			else
				selector = word_40D44;
			text_id[2] = (legacy_s8)('a' + selector);
			text_id[3] = 0;
			text = locate_text_res(opponent_resource, text_id);
		}

		font_set_fontdef2(fontnptr);
		for (;;) {
			character = (legacy_u8)*text++;
			if (character != ' ' && character != 0) {
				word[word_length++] = (legacy_s8)character;
				continue;
			}

			word[word_length] = 0;
			word_width = (legacy_s16)font_op2(word);
			if (LEGACY_S16_WRAP_ADD(word_width, line_width) <
				LEGACY_S16_WRAP_SUB(animation_x, 0x10) &&
				LEGACY_U16_WRAP_ADD(output_length, word_length) <
				0x50U) {
				for (copy_index = 0; copy_index < word_length;
					copy_index++) {
					(&resID_byte1)[output_length++] = word[copy_index];
				}
				line_width = LEGACY_S16_WRAP_ADD(line_width,
					word_width);
			} else {
				(&resID_byte1)[output_length] = 0;
				font_draw_text(&resID_byte1, 8, line_y);
				line_y = LEGACY_S16_WRAP_ADD(line_y, 8);
				first_character = word[0] == ' ' ? 1U : 0U;
				output_length = 0;
				for (copy_index = first_character;
					copy_index < word_length; copy_index++) {
					(&resID_byte1)[output_length++] = word[copy_index];
				}
				(&resID_byte1)[output_length] = 0;
				line_width = (legacy_s16)font_op2(&resID_byte1);
			}

			word_length = 1;
			word[0] = ' ';
			if (character == 0)
				break;
		}
		font_set_fontdef();
	}

	if (output_length != 0) {
		font_set_fontdef2(fontnptr);
		(&resID_byte1)[output_length] = 0;
		font_draw_text(&resID_byte1, 8, line_y);
		font_set_fontdef();
	}
}

legacy_u16 end_hiscore(void)
{
	legacy_s8 number[18];
	legacy_s8 far* misc_resource;
	legacy_s8 far* opponent_resource;
	legacy_s8 far* animation_resource;
	legacy_u8 far* animation_sequence;
	legacy_u8 far* track_resource;
	struct HIGHSCORE_ENTRY far* scores;
	struct SPRITE far* animation_sprite;
	struct SHAPE2D far* frame_shape;
	struct BUTTON_AREA menu_areas[5];
	struct BUTTON_AREA button_areas[4];
	legacy_s8 score_status;
	legacy_u8 outcome;
	legacy_u8 opponent_active;
	legacy_u8 evaluation_screen;
	legacy_u8 selected;
	legacy_u8 previous_selection;
	legacy_u8 blit_mode;
	legacy_u8 animation_frame;
	legacy_u8 previous_animation_frame;
	legacy_u8 text_prefix;
	legacy_u16 i;
	legacy_u16 duration;
	legacy_u16 average_speed;
	legacy_u16 text_resource_count;
	legacy_u16 input;
	legacy_s16 text_y;
	legacy_s16 finish_time;
	legacy_s16 animation_width;
	legacy_s16 animation_x;
	legacy_s16 animation_y;
	legacy_s16 animation_timer;
	legacy_s16 delta;
	legacy_s16 menu_offset;
	legacy_s16 hit;
	legacy_s16 random_value;
	legacy_u16 result;

	ensure_file_exists(4);
	misc_resource = (legacy_s8 far*)file_load_resfile(aMisc_2);
	opponent_resource = 0;
	if (gameconfig.game_opponenttype != 0) {
		aOpp1[3] = (legacy_s8)((legacy_u8)gameconfig.game_opponenttype + '0');
		opponent_resource = (legacy_s8 far*)file_load_resfile(aOpp1);
	}

	render_window_sprite = sprite_make_wnd(0x140U, 0xC8U, 0x0FU);
	animation_sprite = 0;
	if (video_flag5_is0 != 0)
		animation_sprite = sprite_make_wnd(0xC8U, 0x64U, 0x0FU);
	blit_mode = 0xFFU;
	sprite_copy_wnd_to_1_clear();
	draw_button(0, 0, 0, 0x140, 0x64,
		word_407F4, word_407F6, word_407F8, 0);
	draw_button(0, 0, 0x65, 0x140, 0x63,
		word_407F4, word_407F6, word_407F8, 0);

	text_y = 0x6B;
	end_hiscore_set_text(misc_resource, aElt);
	if (gState_total_finish_time != 0) {
		format_frame_as_string(number,
			LEGACY_S16_WRAP_SUB(gState_total_finish_time,
				gState_penalty), 1);
		strcat(&resID_byte1, number);
		if (((legacy_u8)byte_43966 & 2U) != 0)
			end_hiscore_append_text(misc_resource, aCon);
		end_hiscore_draw_current_text(&text_y);
		if (gState_penalty != 0) {
			end_hiscore_set_text(misc_resource, aPpt);
			format_frame_as_string(number, gState_penalty, 1);
			strcat(&resID_byte1, number);
			end_hiscore_draw_current_text(&text_y);
		}
	} else {
		end_hiscore_append_text(misc_resource, aDnf);
		end_hiscore_draw_current_text(&text_y);
	}

	outcome = 2;
	if (gameconfig.game_opponenttype != 0) {
		if (gState_144 == 0) {
			end_hiscore_set_text(misc_resource, aOlt);
			end_hiscore_append_text(misc_resource, aDnf_0);
			if (gState_total_finish_time != 0)
				outcome = 0;
		} else if (gState_total_finish_time == 0 ||
			(legacy_u16)gState_144 <
				(legacy_u16)gState_total_finish_time) {
			end_hiscore_set_text(misc_resource, aOwt);
			format_frame_as_string(number, gState_144, 1);
			strcat(&resID_byte1, number);
			outcome = 1;
		} else {
			end_hiscore_set_text(misc_resource, aOlt_0);
			format_frame_as_string(number, gState_144, 1);
			strcat(&resID_byte1, number);
			outcome = 0;
		}
		end_hiscore_draw_current_text(&text_y);
	}

	if (outcome == 0)
		file_load_audiores(aSkidvict, aSkidms_1, aVict);
	else
		file_load_audiores(aSkidover, aSkidms_2, aOver);

	opponent_active = (legacy_u8)gameconfig.game_opponenttype;
	if (outcome == 2 && gState_pEndFrame != gState_oEndFrame)
		opponent_active = 0;

	end_hiscore_set_text(misc_resource, aAvs);
	duration = LEGACY_U16_WRAP_ADD(gState_pEndFrame, elapsed_time1);
	if (duration != 0) {
		average_speed = (legacy_u16)(LEGACY_U32_DIV_OR_ZERO(
			(legacy_u32)gState_travDist, (legacy_u32)duration) >> 8);
	} else {
		average_speed = 0;
	}
	print_int_as_string_maybe(number, average_speed, 0, 3);
	strcat(&resID_byte1, number);
	end_hiscore_append_text(misc_resource, aMph);
	end_hiscore_draw_current_text(&text_y);

	if (gState_impactSpeed != 0) {
		end_hiscore_set_text(misc_resource, aImp);
		print_int_as_string_maybe(number,
			(legacy_u16)gState_impactSpeed >> 8, 0, 3);
		strcat(&resID_byte1, number);
		end_hiscore_append_text(misc_resource, aMph_0);
		end_hiscore_draw_current_text(&text_y);
	}

	end_hiscore_set_text(misc_resource, aTop);
	print_int_as_string_maybe(number,
		(legacy_u16)gState_topSpeed >> 8, 0, 3);
	strcat(&resID_byte1, number);
	end_hiscore_append_text(misc_resource, aMph_1);
	end_hiscore_draw_current_text(&text_y);
	if (gState_jumpCount != 0) {
		end_hiscore_set_text(misc_resource, aJum);
		print_int_as_string_maybe(number, gState_jumpCount, 0, 3);
		strcat(&resID_byte1, number);
		hiscore_draw_text(&resID_byte1, font_op2_alt(&resID_byte1),
			text_y, dialog_fnt_colour, 0);
	}

	animation_resource = 0;
	animation_sequence = 0;
	text_prefix = 0;
	if (opponent_active != 0) {
		if (((legacy_u8)byte_43966 & 4U) == 0) {
			word_40D3A = word_40D40;
			word_40D3C = end_hiscore_random;
			word_40D3E = word_40D44;
			random_value = (legacy_s16)get_super_random();
			word_40D40 = (legacy_s16)(random_value % 3);
			if (word_40D40 == word_40D3A)
				word_40D40 = word_3BCDE[(legacy_u16)word_40D40];
			random_value = (legacy_s16)get_super_random();
			word_40D44 = (legacy_s16)(random_value % 3);
			if (word_40D44 == word_40D3E)
				word_40D44 = word_3BCDE[(legacy_u16)word_40D44];

			random_value = (legacy_s16)get_super_random();
			if (outcome == 1) {
				end_hiscore_random = (legacy_s16)(random_value % 2);
				if (gState_total_finish_time != 0)
					end_hiscore_random = LEGACY_S16_WRAP_ADD(
						end_hiscore_random, 2);
			} else {
				end_hiscore_random = (legacy_s16)(random_value % 4);
			}
			if (end_hiscore_random == word_40D3C) {
				end_hiscore_random = word_3BCE4[
					(legacy_u16)end_hiscore_random];
			}
		}

		if (outcome == 1) {
			aOpp2win[3] = (legacy_s8)(opponent_active + '0');
			animation_resource = (legacy_s8 far*)file_load_resource(
				3, aOpp2win);
			animation_sequence = (legacy_u8 far*)locate_shape_alt(
				opponent_resource, aWinn);
			end_hiscore_random = (legacy_s16)(
				LEGACY_U16_WRAP_ADD(get_kevinrandom(), gState_frame) & 1U);
			if (gState_total_finish_time != 0)
				end_hiscore_random = LEGACY_S16_WRAP_ADD(
					end_hiscore_random, 2);
			text_prefix = 'v';
		} else {
			aOpp2lose[3] = (legacy_s8)(opponent_active + '0');
			animation_resource = (legacy_s8 far*)file_load_resource(
				3, aOpp2lose);
			animation_sequence = (legacy_u8 far*)locate_shape_alt(
				opponent_resource, aLose);
			end_hiscore_random = (legacy_s16)(
				LEGACY_U16_WRAP_ADD(get_kevinrandom(), gState_frame) & 3U);
			text_prefix = 'd';
		}
	}

	score_status = 0;
	file_build_path(byte_3B80C, gameconfig.game_trackname,
		a_trk_5, g_path_buf);
	track_resource = (legacy_u8 far*)file_load_resource(
		FILE_RESOURCE_BINARY_OPTIONAL, g_path_buf);
	if (track_resource == 0) {
		result = show_dialog(1, 1,
			locate_text_res(mainresptr, aIhd),
			0xFFFFU, 0xFFFFU, dialogarg2, 0, 0);
		if (result != 0)
			track_resource = (legacy_u8 far*)file_load_resource(
				1, g_path_buf);
	}
	if (track_resource != 0) {
		for (i = 0; i < 0x385U; i++) {
			if (track_resource[i] != td14_elem_map_main[i]) {
				score_status = -1;
				break;
			}
		}
		mmgr_release((legacy_s8 far*)track_resource);
	} else {
		score_status = -1;
	}

	if (score_status == 0 && highscore_write_a(0) != 0) {
		if (highscore_write_a(1) != 0)
			score_status = -1;
	}
	finish_time = 0;
	if (score_status == 0 && gState_total_finish_time != 0) {
		finish_time = gState_total_finish_time;
		scores = (struct HIGHSCORE_ENTRY far*)td11_highscores;
		if (((legacy_u8)byte_43966 & 6U) == 0 &&
			scores[HIGHSCORE_LAST_ENTRY_INDEX].time >
				(legacy_u16)finish_time) {
			score_status = 1;
		}
	}

	animation_frame = 0;
	animation_timer = 0x1E;
	evaluation_screen = 1;

	for (;;) {
	do {
	if (opponent_active != 0 && score_status == 2) {
		score_status = 0;
		sprite_copy_wnd_to_1();
		highscore_text_unk();
		selected = 1;
		evaluation_screen = 1;
		break;
	}

	if (opponent_active == 0) {
		if (score_status > 0) {
			check_input();
			mouse_draw_opaque_check();
			enter_hiscore(finish_time,
				locate_text_res(misc_resource, aInh_0), 0);
			score_status = 0;
			blit_mode = 0xFEU;
		} else {
			mouse_draw_opaque_check();
			if (score_status == -1) {
				end_hiscore_set_text(misc_resource, aHna);
				hiscore_draw_text(&resID_byte1,
					font_op2_alt(&resID_byte1), 0x32,
					dialog_fnt_colour, 0);
			} else {
				highscore_text_unk();
			}
		}
		break;
	}

	aOp01[3] = '1';
	frame_shape = (struct SHAPE2D far*)locate_shape_fatal(
		animation_resource, aOp01);
	animation_width = LEGACY_S16_WRAP_MUL(shape2d_get_width(frame_shape),
		video_flag1_is1);
	animation_x = LEGACY_S16_WRAP_SUB(0x138, animation_width);
	animation_y = LEGACY_S16_WRAP_SUB(
		0x63, shape2d_get_height(frame_shape));
	animation_y = LEGACY_S16_FROM_BITS(
		LEGACY_U16_SAR((legacy_u16)animation_y, 1U));
	draw_lines_unk(LEGACY_S16_WRAP_SUB(animation_x, 3),
		LEGACY_S16_WRAP_SUB(animation_y, 3),
		LEGACY_S16_WRAP_ADD(animation_width, 5),
		LEGACY_S16_WRAP_ADD(shape2d_get_height(frame_shape), 5),
		dialog_fnt_colour, 0, word_407D2);
	aOp01[3] = (legacy_s8)(animation_sequence[animation_frame] + '0');
	shape2d_op_unk5((struct SHAPE2D far*)locate_shape_fatal(
		animation_resource, aOp01), animation_x, animation_y);
	previous_animation_frame = animation_frame;
	font_set_unk(0, 0);
	end_hiscore_draw_opponent_text(opponent_resource, outcome,
		text_prefix, animation_x);
	evaluation_screen = 0;
	if (score_status <= 0)
		break;

	score_status = 0;
	evaluation_screen = 1;
	draw_button(locate_text_res(misc_resource, aBct),
		0x81, 0xAF, 0x46, 0x15,
		word_407F4, word_407F6, word_407F8, 0);
	(void)sprite_blit_to_video(render_window_sprite,
		LEGACY_S8_FROM_BITS(blit_mode));
	blit_mode = 0xFEU;
	sub_29772();
	check_input();
	sprite_copy_2_to_1_2();
	for (i = 0; i < 5U; i++) {
		menu_areas[i].x1 = word_3BCEC[i];
		menu_areas[i].x2 = word_3BCF6[i];
		menu_areas[i].y1 = hiscore_buttons_y1[i];
		menu_areas[i].y2 = hiscore_buttons_y2[i];
	}
	text_resource_count = outcome == 2 ? 1U : 3U;
	for (;;) {
		delta = (legacy_s16)mouse_timer_sprite_unk(4, menu_areas,
			word_407CE, word_407D0);
		end_hiscore_update_animation(delta, &animation_timer,
			&animation_frame, &previous_animation_frame,
			animation_resource, animation_sequence, animation_x,
			animation_y, animation_sprite, 0);
		input = (legacy_u16)input_checking(
			(legacy_s16)text_resource_count);
		if (input == 0x0DU || input == 0x20U || input == 0x1BU)
			break;
	}

	sprite_copy_wnd_to_1();
	draw_button(0, 0, 0, 0x140, 0x64,
		word_407F4, word_407F6, word_407F8, 0);
	sprite_set_1_size(8, 0x138, hiscore_buttons_y1[0],
		LEGACY_S16_WRAP_ADD(hiscore_buttons_y2[0], 1));
	sprite_clear_1_color(word_407F8);
	mouse_draw_opaque_check();
	enter_hiscore(finish_time,
		locate_text_res(misc_resource, aInh), outcome);

	} while (0);
	selected = 1;
	previous_selection = 1;
	sub_29772();
	sprite_copy_wnd_to_1();
	if (opponent_active == 0 || score_status == -1) {
		menu_offset = -0x24;
	} else {
		menu_offset = 0;
		draw_button(locate_text_res(misc_resource,
			evaluation_screen != 0 ? aBev : aBhi),
			LEGACY_S16_WRAP_ADD(word_3BCEC[0], 1),
			0xAF, 0x46, 0x15,
			word_407F4, word_407F6, word_407F8, 0);
	}
	draw_button(locate_text_res(misc_resource, aBrp),
		LEGACY_S16_WRAP_ADD(
			LEGACY_S16_WRAP_ADD(word_3BCEC[1], menu_offset), 1),
		0xAF, 0x46, 0x15,
		word_407F4, word_407F6, word_407F8, 0);
	draw_button(locate_text_res(misc_resource,
		opponent_active != 0 ? aBra : aBdr),
		LEGACY_S16_WRAP_ADD(
			LEGACY_S16_WRAP_ADD(word_3BCEC[2], menu_offset), 1),
		0xAF, 0x46, 0x15,
		word_407F4, word_407F6, word_407F8, 0);
	draw_button(locate_text_res(misc_resource, aBmm_0),
		LEGACY_S16_WRAP_ADD(
			LEGACY_S16_WRAP_ADD(word_3BCEC[3], menu_offset), 1),
		0xAF, 0x46, 0x15,
		word_407F4, word_407F6, word_407F8, 0);
	for (i = 0; i < 4U; i++) {
		button_areas[i].x1 = LEGACY_S16_WRAP_ADD(word_3BCEC[i], menu_offset);
		button_areas[i].x2 = LEGACY_S16_WRAP_ADD(word_3BCF6[i], menu_offset);
		button_areas[i].y1 = hiscore_buttons_y1[i];
		button_areas[i].y2 = hiscore_buttons_y2[i];
	}
	check_input();
	(void)sprite_blit_to_video(render_window_sprite,
		LEGACY_S8_FROM_BITS(blit_mode));
	blit_mode = 0xFEU;
	sprite_copy_2_to_1_2();

	for (;;) {
	if (previous_selection != selected) {
		previous_selection = selected;
		sprite_copy_2_to_1_2();
		sprite_set_1_size(0, 0x140,
			hiscore_buttons_y1[0],
			LEGACY_S16_WRAP_ADD(hiscore_buttons_y2[0], 1));
		mouse_draw_opaque_check();
		sprite_putimage(render_window_sprite->sprite_bitmapptr);
		mouse_draw_transparent_check();
		(void)timer_get_delta_alt();
		sub_29772();
	}

		delta = (legacy_s16)mouse_timer_sprite_unk(selected, button_areas,
			word_407CE, word_407D0);
		if (evaluation_screen == 0 && outcome != 2) {
			end_hiscore_update_animation(delta, &animation_timer,
				&animation_frame, &previous_animation_frame,
				animation_resource, animation_sequence, animation_x,
				animation_y, animation_sprite, 1);
		}

	if (opponent_active == 0 || score_status == -1) {
		hit = (legacy_s16)mouse_multi_hittest(3, &button_areas[1]);
		if (hit != -1)
			selected = (legacy_u8)(hit + 1);
	} else {
		hit = (legacy_s16)mouse_multi_hittest(4, button_areas);
		if (hit != -1)
			selected = (legacy_u8)hit;
	}

	input = (legacy_u16)input_checking(delta);
	if (input == 0)
		continue;
	if (input == 0x4B00U) {
		if (opponent_active == 0 || score_status == -1) {
			selected = selected <= 1 ? 3U :
				(legacy_u8)(selected - 1U);
		} else {
			selected = selected == 0 ? 3U :
				(legacy_u8)(selected - 1U);
		}
		continue;
	}
	if (input == 0x4D00U) {
		if (selected < 3U)
			selected++;
		else
			selected = (opponent_active == 0 ||
				score_status == -1) ? 1U : 0U;
		continue;
	}
	if (input != 0x0DU && input != 0x20U)
		continue;

	if (selected == 0) {
		sprite_copy_wnd_to_1();
		draw_button(0, 0, 0, 0x140, 0x64,
			word_407F4, word_407F6, word_407F8, 0);
		score_status = evaluation_screen != 0 ? 0 : 2;
		break;
	}

	audio_unload();
	if (opponent_active != 0)
		mmgr_release(animation_resource);
	if (video_flag5_is0 != 0)
		sprite_free_wnd(animation_sprite);
	sprite_free_wnd(render_window_sprite);
	if (gameconfig.game_opponenttype != 0)
		unload_resource(opponent_resource);
	unload_resource(misc_resource);
	return (legacy_u16)(selected - 1U);
	}
	}
}
