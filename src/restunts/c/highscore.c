#include "fileio.h"
#include "legacy.h"
#include "memmgr.h"
#include "menu_internal.h"
#include "platform.h"
#include "replay.h"
#include "resource.h"
#include "shape2d.h"
#include "ui_text.h"
#include "shape2d_internal.h"
#include "state_internal.h"
#include "timing.h"
#include "ui_input.h"
#include "game_input.h"
#include "ui_dialog.h"

#define HIGHSCORE_READ_RETRY_CANCEL_RESULT 2

#define HIGHSCORE_NO_HIGHLIGHT LEGACY_U8_MAX
#define HIGHSCORE_EMPTY_TIME LEGACY_U16_MAX
#define HIGHSCORE_FORMAT_BUFFER_SIZE 18
#define HIGHSCORE_TEXT_FIELD_COUNT 4
#define HIGHSCORE_NAME_MAX_CHARACTERS 16
#define HIGHSCORE_NAME_INPUT_TIMEOUT 30000UL
#define HIGHSCORE_LOW_FRAME_RATE_TIME_SCALE 2U

#define HIGHSCORE_TITLE_Y 5
#define HIGHSCORE_HEADING_Y 15
#define HIGHSCORE_FIRST_ROW_Y 25
#define HIGHSCORE_ROW_HEIGHT 10U
#define HIGHSCORE_PLAYER_COLUMN_X 16
#define HIGHSCORE_CAR_COLUMN_X 120
#define HIGHSCORE_OPPONENT_COLUMN_X 224
#define HIGHSCORE_TIME_COLUMN_X 272

#define END_SCREEN_WIDTH 320U
#define END_SCREEN_HEIGHT 200U
#define END_SCREEN_COLOR 15U
#define END_SCREEN_TOP_HEIGHT 100
#define END_SCREEN_BOTTOM_Y 101
#define END_SCREEN_BOTTOM_HEIGHT 99
#define END_SCREEN_TEXT_START_Y 107
#define END_SCREEN_ANIMATION_WIDTH 200U
#define END_SCREEN_ANIMATION_HEIGHT 100U
#define END_SCREEN_ANIMATION_RIGHT 312
#define END_SCREEN_ANIMATION_BOTTOM 99
#define END_SCREEN_ANIMATION_CENTER_SHIFT 1U
#define END_SCREEN_ANIMATION_BORDER_INSET 3
#define END_SCREEN_ANIMATION_BORDER_GROWTH 5
#define END_SCREEN_ANIMATION_FRAME_TICKS 30

#define END_SCREEN_SPEED_FRACTION_BITS 8U
#define END_SCREEN_NUMBER_WIDTH 3

#define END_SCREEN_TEXT_VARIANT_COUNT 3U
#define END_SCREEN_WIN_VARIANT_COUNT 2
#define END_SCREEN_OUTCOME_VARIANT_COUNT 4
#define END_SCREEN_FINISHED_VARIANT_OFFSET 2
#define END_SCREEN_BINARY_RANDOM_MASK 1U
#define END_SCREEN_FOUR_WAY_RANDOM_MASK 3U

#define END_SCREEN_TEXT_WORD_CAPACITY 32
#define END_SCREEN_TEXT_ID_SIZE 4
#define END_SCREEN_TEXT_LEFT 8
#define END_SCREEN_TEXT_TOP 8
#define END_SCREEN_TEXT_LINE_HEIGHT 8
#define END_SCREEN_TEXT_RIGHT_MARGIN 16
#define END_SCREEN_TEXT_OUTPUT_CAPACITY 80U

#define END_SCREEN_TRACK_RESOURCE_INDEX 4
#define END_SCREEN_TRACK_VALIDATION_BYTES 901U
#define END_SCREEN_MENU_AREA_COUNT 5U
#define END_SCREEN_BUTTON_COUNT 4U
#define END_SCREEN_REDUCED_BUTTON_COUNT 3
#define END_SCREEN_MENU_OFFSET_WITHOUT_FIRST_BUTTON (-36)
#define END_SCREEN_LAST_BUTTON_INDEX 3U
#define END_SCREEN_EVALUATION_BUTTON_X 129
#define END_SCREEN_BUTTON_Y 175
#define END_SCREEN_BUTTON_WIDTH 70
#define END_SCREEN_BUTTON_HEIGHT 21
#define END_SCREEN_NO_SCORE_MESSAGE_Y 50

enum HIGHSCORE_READ_OPERATION {
	HIGHSCORE_READ_RETRY_OPERATION = 9,
	HIGHSCORE_READ_ONCE_OPERATION = 10
};

enum END_SCREEN_OUTCOME {
	END_SCREEN_OUTCOME_LOSS = 0,
	END_SCREEN_OUTCOME_WIN = 1,
	END_SCREEN_OUTCOME_NONE = 2
};

static legacy_u8 ranking_highlight;
legacy_s16 ranking_entry_order[HIGHSCORE_ENTRY_COUNT];

legacy_s16 get_super_random(void);

struct RECTANGLE* hiscore_draw_text(legacy_s8* text, legacy_s16 x, legacy_s16 y, legacy_s16 color,
	legacy_s16 shadow_color)
{
	highscore_text_bounds.left = LEGACY_S16_WRAP_SUB(x, 1);
	highscore_text_bounds.right = LEGACY_S16_WRAP_ADD(
		LEGACY_S16_WRAP_ADD(x, font_text_width(text)), 1);
	highscore_text_bounds.top = LEGACY_S16_WRAP_SUB(y, 1);
	highscore_text_bounds.bottom = LEGACY_S16_WRAP_ADD(
		LEGACY_S16_WRAP_ADD(y, font_glyph_height), 1);
	font_set_colors(shadow_color, 0);
	font_draw_text(text, LEGACY_S16_WRAP_ADD(x, 1),
		LEGACY_S16_WRAP_ADD(y, 1));
	font_draw_text(text, LEGACY_S16_WRAP_SUB(x, 1),
		LEGACY_S16_WRAP_ADD(y, 1));
	font_draw_text(text, LEGACY_S16_WRAP_ADD(x, 1),
		LEGACY_S16_WRAP_SUB(y, 1));
	font_draw_text(text, LEGACY_S16_WRAP_SUB(x, 1),
		LEGACY_S16_WRAP_SUB(y, 1));
	font_set_colors(color, 0);
	font_draw_text(text, LEGACY_S16_FROM_BITS((legacy_u16)x),
		LEGACY_S16_FROM_BITS((legacy_u16)y));
	return &highscore_text_bounds;
}

void far* highscore_read_with_retry(legacy_s16 operation, const legacy_s8* filename,
	void far* destination)
{
	void far* result;

	if (operation == HIGHSCORE_READ_ONCE_OPERATION)
		return file_read_nofatal(filename, destination);
	if (operation != HIGHSCORE_READ_RETRY_OPERATION)
		return 0;
	do {
		result = file_read_nofatal(filename, destination);
		if (result != 0)
			return result;
	} while (show_disk_error_dialog() != HIGHSCORE_READ_RETRY_CANCEL_RESULT);
	return 0;
}

legacy_s16 highscore_load_or_create(legacy_s16 create_default)
{
	struct HIGHSCORE_ENTRY record;
	legacy_u8* record_bytes;
	struct HIGHSCORE_ENTRY far* scores;
	void far* read_result;
	legacy_u16 entry;
	legacy_u16 offset;

	ranking_highlight = HIGHSCORE_NO_HIGHLIGHT;
	for (entry = 0; entry < HIGHSCORE_ENTRY_COUNT; entry++)
		ranking_entry_order[entry] = entry;
	file_build_path(track_directory, gameconfig.game_trackname,
		".hig", g_path_buf);
	if (create_default == 0) {
		g_is_busy = 1;
		read_result = highscore_read_with_retry(HIGHSCORE_READ_ONCE_OPERATION,
			g_path_buf, track_highscore_table);
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
	record.time = HIGHSCORE_EMPTY_TIME;
	scores = (struct HIGHSCORE_ENTRY far*)track_highscore_table;
	for (entry = 0; entry < HIGHSCORE_ENTRY_COUNT; entry++)
		scores[entry] = record;
	return file_write_fatal(g_path_buf, track_highscore_table,
		HIGHSCORE_TABLE_SIZE_BYTES) != 0;
}

void highscore_save_sorted(void)
{
	struct HIGHSCORE_ENTRY ordered_scores[HIGHSCORE_ENTRY_COUNT];
	struct HIGHSCORE_ENTRY far* scores;
	legacy_u16 entry;
	legacy_u16 source_entry;

	scores = (struct HIGHSCORE_ENTRY far*)track_highscore_table;
	for (entry = 0; entry < HIGHSCORE_ENTRY_COUNT; entry++) {
		source_entry = (legacy_u16)ranking_entry_order[entry];
		ordered_scores[entry] = scores[source_entry];
	}
	file_build_path(track_directory, gameconfig.game_trackname,
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
	legacy_s8 formatted_time[HIGHSCORE_FORMAT_BUFFER_SIZE];
	legacy_s8* output;

	scores = (struct HIGHSCORE_ENTRY far*)track_highscore_table;
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
	framespersec = GAME_FRAME_RATE_NORMAL;
	frame_count = LEGACY_S16_FROM_BITS(record.time);
	format_frame_as_string(formatted_time,
		frame_count == -1 ? 0 : frame_count, 1);
	text_offsets[3] = (legacy_u8)output_offset;
	strcpy(&resID_byte1 + output_offset, formatted_time);
	framespersec = saved_frame_rate;
}

void highscore_draw_table(void)
{
	legacy_u8 text_offsets[HIGHSCORE_TEXT_FIELD_COUNT];
	legacy_s16 row;
	legacy_u16 entry;
	legacy_s16 color;
	legacy_s8 far* text;

	sprite_select_render_window();
	copy_string(&resID_byte1, locate_text_res(mainresptr, "hs1"));
	strcat(&resID_byte1, " '");
	strcat(&resID_byte1, gameconfig.game_trackname);
	strcat(&resID_byte1, "'");
	hiscore_draw_text(&resID_byte1, font_centered_text_x(&resID_byte1),
		HIGHSCORE_TITLE_Y, dialog_fnt_colour, 0);

	text = locate_text_res(mainresptr, "hs2");
	copy_string(&resID_byte1, text);
	hiscore_draw_text(&resID_byte1, HIGHSCORE_PLAYER_COLUMN_X,
		HIGHSCORE_HEADING_Y,
		dialog_fnt_colour, 0);
	text = locate_text_res(mainresptr, "hs3");
	copy_string(&resID_byte1, text);
	hiscore_draw_text(&resID_byte1, HIGHSCORE_CAR_COLUMN_X,
		HIGHSCORE_HEADING_Y,
		dialog_fnt_colour, 0);
	text = locate_text_res(mainresptr, "hs5");
	copy_string(&resID_byte1, text);
	hiscore_draw_text(&resID_byte1, HIGHSCORE_OPPONENT_COLUMN_X,
		HIGHSCORE_HEADING_Y,
		dialog_fnt_colour, 0);
	text = locate_text_res(mainresptr, "hs4");
	copy_string(&resID_byte1, text);
	hiscore_draw_text(&resID_byte1, HIGHSCORE_TIME_COLUMN_X,
		HIGHSCORE_HEADING_Y,
		dialog_fnt_colour, 0);

	font_set_fontdef2(fontnptr);
	for (entry = 0; entry < HIGHSCORE_ENTRY_COUNT; entry++) {
		print_highscore_entry(entry, text_offsets);
		row = LEGACY_S16_WRAP_ADD(
			LEGACY_U16_WRAP_MUL(entry, HIGHSCORE_ROW_HEIGHT),
			HIGHSCORE_FIRST_ROW_Y);
		color = entry == (legacy_u8)ranking_highlight ? dialog_border_color : 0;
		font_set_colors(color, 0);
		font_draw_text(&resID_byte1 + text_offsets[0],
			HIGHSCORE_PLAYER_COLUMN_X, row);
		font_draw_text(&resID_byte1 + text_offsets[1],
			HIGHSCORE_CAR_COLUMN_X, row);
		font_draw_text(&resID_byte1 + text_offsets[2],
			HIGHSCORE_OPPONENT_COLUMN_X, row);
		font_draw_text(&resID_byte1 + text_offsets[3],
			HIGHSCORE_TIME_COLUMN_X, row);
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
	if (framespersec == GAME_FRAME_RATE_LOW)
		time_bits = LEGACY_U16_WRAP_MUL(time_bits,
			HIGHSCORE_LOW_FRAME_RATE_TIME_SCALE);
	scores = (struct HIGHSCORE_ENTRY far*)track_highscore_table;
	if (scores[HIGHSCORE_LAST_ENTRY_INDEX].time <= time_bits) {
		highscore_draw_table();
		return;
	}

	entry = 0;
	while (scores[entry].time <= time_bits) {
		if (entry >= HIGHSCORE_ENTRY_COUNT)
			break;
		ranking_entry_order[entry] = (legacy_s16)entry;
		entry++;
	}
	rank = entry;
	ranking_highlight = (legacy_u8)rank;
	while (entry < HIGHSCORE_LAST_ENTRY_INDEX) {
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
		strcpy(record.opponent, opponent_highscore_name);
		record.opponent[2] = '/';
		strcpy(&record.opponent[3], gsna_string);
	} else {
		strcpy(record.opponent, " ");
	}
	record.time = time_bits;
	scores[HIGHSCORE_LAST_ENTRY_INDEX] = record;

	sprite_select_render_window();
	highscore_draw_table();
	sprite_blit_to_video(render_window_sprite, -1);
	show_dialog(DIALOG_TYPE_PLACEHOLDERS, DIALOG_NO_BACKGROUND_SAVE,
		prompt, DIALOG_AUTO_POSITION, DIALOG_AUTO_POSITION,
		dialog_border_color, positions, 0);
	check_input();
	call_read_line(highscore_player_name_input, HIGHSCORE_NAME_MAX_CHARACTERS,
		positions[0], positions[1], HIGHSCORE_NAME_INPUT_TIMEOUT);
	strcpy(record.player_name, highscore_player_name_input);
	scores[HIGHSCORE_LAST_ENTRY_INDEX] = record;

	sprite_select_render_window();
	highscore_draw_table();
	sprite_blit_to_video(render_window_sprite, -1);
	highscore_save_sorted();
	highscore_draw_table();
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
	hiscore_draw_text(&resID_byte1, font_centered_text_x(&resID_byte1), *y,
		dialog_fnt_colour, 0);
	*y = LEGACY_S16_WRAP_ADD(*y, HIGHSCORE_ROW_HEIGHT);
}

static void end_hiscore_draw_animation_frame(legacy_s8 far* animation_resource,
	legacy_u8 far* frame_sequence, legacy_u8 frame_index,
	legacy_s16 animation_x, legacy_s16 animation_y,
	struct SPRITE far* animation_sprite, legacy_u8 draw_direct_copy)
{
	struct SHAPE2D far* frame_shape;

	opponent_animation_frame_id[3] = (legacy_s8)(frame_sequence[frame_index] + '0');
	frame_shape = (struct SHAPE2D far*)locate_shape_fatal(
		animation_resource, opponent_animation_frame_id);
	mouse_draw_opaque_check();
	if (video_uses_page_flipping != 0) {
		sprite_select_target(animation_sprite);
		shape2d_rle_copy(frame_shape, 0, 0);
		sprite_select_screen_compat();
		sprite_set_target_clip_bounds(animation_x,
			LEGACY_S16_WRAP_ADD(animation_x,
				LEGACY_S16_WRAP_MUL(shape2d_get_width(frame_shape),
					video_shape_width_scale)),
			animation_y,
			LEGACY_S16_WRAP_ADD(animation_y,
				shape2d_get_height(frame_shape)));
		sprite_copy_image_at(animation_sprite->sprite_bitmapptr,
			animation_x, animation_y);
		sprite_select_screen_compat();
	} else {
		shape2d_rle_copy(frame_shape, animation_x, animation_y);
	}
	if (draw_direct_copy != 0)
		shape2d_rle_copy(frame_shape, animation_x, animation_y);
	mouse_draw_transparent_check();
}

static void end_hiscore_advance_animation(legacy_s16 delta,
	legacy_s16* timer, legacy_u8* frame, const legacy_u8 far* frame_sequence)
{
	*timer = LEGACY_S16_WRAP_ADD(*timer, delta);
	if (*timer >= END_SCREEN_ANIMATION_FRAME_TICKS) {
		*timer = LEGACY_S16_WRAP_SUB(*timer,
			END_SCREEN_ANIMATION_FRAME_TICKS);
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
	legacy_s8 word[END_SCREEN_TEXT_WORD_CAPACITY];
	legacy_s8 text_id[END_SCREEN_TEXT_ID_SIZE];
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

	line_y = END_SCREEN_TEXT_TOP;
	output_length = 0;
	line_width = 0;
	word_length = 0;
	resource_count = outcome == END_SCREEN_OUTCOME_NONE ?
		1U : END_SCREEN_TEXT_VARIANT_COUNT;
	for (resource_index = 0; resource_index < resource_count;
		resource_index++) {
		if (outcome == END_SCREEN_OUTCOME_NONE) {
			text = locate_text_res(opponent_resource, opponent_neutral_result_text_id);
		} else {
			text_id[0] = (legacy_s8)text_prefix;
			text_id[1] = (legacy_s8)('1' + resource_index);
			if (resource_index == 0)
				selector = end_opening_variant;
			else if (resource_index == 1)
				selector = end_outcome_variant;
			else
				selector = end_closing_variant;
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
			word_width = (legacy_s16)font_text_width(word);
			if (LEGACY_S16_WRAP_ADD(word_width, line_width) <
				LEGACY_S16_WRAP_SUB(animation_x,
					END_SCREEN_TEXT_RIGHT_MARGIN) &&
				LEGACY_U16_WRAP_ADD(output_length, word_length) <
				END_SCREEN_TEXT_OUTPUT_CAPACITY) {
				for (copy_index = 0; copy_index < word_length;
					copy_index++) {
					(&resID_byte1)[output_length++] = word[copy_index];
				}
				line_width = LEGACY_S16_WRAP_ADD(line_width,
					word_width);
			} else {
				(&resID_byte1)[output_length] = 0;
				font_draw_text(&resID_byte1, END_SCREEN_TEXT_LEFT,
					line_y);
				line_y = LEGACY_S16_WRAP_ADD(line_y,
					END_SCREEN_TEXT_LINE_HEIGHT);
				first_character = word[0] == ' ' ? 1U : 0U;
				output_length = 0;
				for (copy_index = first_character;
					copy_index < word_length; copy_index++) {
					(&resID_byte1)[output_length++] = word[copy_index];
				}
				(&resID_byte1)[output_length] = 0;
				line_width = (legacy_s16)font_text_width(&resID_byte1);
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
		font_draw_text(&resID_byte1, END_SCREEN_TEXT_LEFT, line_y);
		font_set_fontdef();
	}
}

legacy_u16 end_hiscore(void)
{
	legacy_s8 number[HIGHSCORE_FORMAT_BUFFER_SIZE];
	legacy_s8 far* misc_resource;
	legacy_s8 far* opponent_resource;
	legacy_s8 far* animation_resource;
	legacy_u8 far* animation_sequence;
	legacy_u8 far* track_resource;
	struct HIGHSCORE_ENTRY far* scores;
	struct SPRITE far* animation_sprite;
	struct SHAPE2D far* frame_shape;
	struct BUTTON_AREA menu_areas[END_SCREEN_MENU_AREA_COUNT];
	struct BUTTON_AREA button_areas[END_SCREEN_BUTTON_COUNT];
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

	ensure_file_exists(END_SCREEN_TRACK_RESOURCE_INDEX);
	misc_resource = (legacy_s8 far*)file_load_resfile(results_misc_resource_name);
	opponent_resource = 0;
	if (gameconfig.game_opponenttype != 0) {
		opponent_resource_name[3] = (legacy_s8)((legacy_u8)gameconfig.game_opponenttype + '0');
		opponent_resource = (legacy_s8 far*)file_load_resfile(opponent_resource_name);
	}

	render_window_sprite = sprite_make_wnd(END_SCREEN_WIDTH,
		END_SCREEN_HEIGHT, END_SCREEN_COLOR);
	animation_sprite = 0;
	if (video_uses_page_flipping != 0)
		animation_sprite = sprite_make_wnd(END_SCREEN_ANIMATION_WIDTH,
			END_SCREEN_ANIMATION_HEIGHT, END_SCREEN_COLOR);
	blit_mode = MENU_BLIT_MODE_INITIAL;
	sprite_select_render_window_and_clear();
	draw_button(0, 0, 0, END_SCREEN_WIDTH, END_SCREEN_TOP_HEIGHT,
		button_top_color, button_bottom_color, button_fill_color, 0);
	draw_button(0, 0, END_SCREEN_BOTTOM_Y, END_SCREEN_WIDTH,
		END_SCREEN_BOTTOM_HEIGHT,
		button_top_color, button_bottom_color, button_fill_color, 0);

	text_y = END_SCREEN_TEXT_START_Y;
	end_hiscore_set_text(misc_resource, elapsed_time_label_id);
	if (gState_total_finish_time != 0) {
		format_frame_as_string(number,
			LEGACY_S16_WRAP_SUB(gState_total_finish_time,
				gState_penalty), 1);
		strcat(&resID_byte1, number);
		if (((legacy_u8)replay_recording_flags &
			REPLAY_RECORDING_MODIFIED_FLAG) != 0)
			end_hiscore_append_text(misc_resource, continued_race_label_id);
		end_hiscore_draw_current_text(&text_y);
		if (gState_penalty != 0) {
			end_hiscore_set_text(misc_resource, penalty_time_label_id);
			format_frame_as_string(number, gState_penalty, 1);
			strcat(&resID_byte1, number);
			end_hiscore_draw_current_text(&text_y);
		}
	} else {
		end_hiscore_append_text(misc_resource, player_did_not_finish_label_id);
		end_hiscore_draw_current_text(&text_y);
	}

	outcome = END_SCREEN_OUTCOME_NONE;
	if (gameconfig.game_opponenttype != 0) {
		if (gState_opponent_finish_time == 0) {
			end_hiscore_set_text(misc_resource, opponent_unfinished_time_label_id);
			end_hiscore_append_text(misc_resource, opponent_did_not_finish_label_id);
			if (gState_total_finish_time != 0)
				outcome = END_SCREEN_OUTCOME_LOSS;
		} else if (gState_total_finish_time == 0 ||
			(legacy_u16)gState_opponent_finish_time <
				(legacy_u16)gState_total_finish_time) {
			end_hiscore_set_text(misc_resource, opponent_win_time_label_id);
			format_frame_as_string(number, gState_opponent_finish_time, 1);
			strcat(&resID_byte1, number);
			outcome = END_SCREEN_OUTCOME_WIN;
		} else {
			end_hiscore_set_text(misc_resource, opponent_loss_time_label_id);
			format_frame_as_string(number, gState_opponent_finish_time, 1);
			strcat(&resID_byte1, number);
			outcome = END_SCREEN_OUTCOME_LOSS;
		}
		end_hiscore_draw_current_text(&text_y);
	}

	if (outcome == END_SCREEN_OUTCOME_LOSS)
		file_load_audiores(victory_music_resource_name, victory_instrument_resource_name, victory_song_id);
	else
		file_load_audiores(race_end_music_resource_name, race_end_instrument_resource_name, race_end_song_id);

	opponent_active = (legacy_u8)gameconfig.game_opponenttype;
	if (outcome == END_SCREEN_OUTCOME_NONE &&
		gState_pEndFrame != gState_oEndFrame)
		opponent_active = 0;

	end_hiscore_set_text(misc_resource, average_speed_label_id);
	duration = LEGACY_U16_WRAP_ADD(gState_pEndFrame, elapsed_time1);
	if (duration != 0) {
		average_speed = (legacy_u16)(LEGACY_U32_DIV_OR_ZERO(
			(legacy_u32)gState_travDist, (legacy_u32)duration) >>
			END_SCREEN_SPEED_FRACTION_BITS);
	} else {
		average_speed = 0;
	}
	format_integer(number, average_speed, 0,
		END_SCREEN_NUMBER_WIDTH);
	strcat(&resID_byte1, number);
	end_hiscore_append_text(misc_resource, average_speed_units_id);
	end_hiscore_draw_current_text(&text_y);

	if (gState_impactSpeed != 0) {
		end_hiscore_set_text(misc_resource, impact_speed_label_id);
		format_integer(number,
			(legacy_u16)gState_impactSpeed >>
				END_SCREEN_SPEED_FRACTION_BITS,
			0, END_SCREEN_NUMBER_WIDTH);
		strcat(&resID_byte1, number);
		end_hiscore_append_text(misc_resource, impact_speed_units_id);
		end_hiscore_draw_current_text(&text_y);
	}

	end_hiscore_set_text(misc_resource, top_speed_label_id);
	format_integer(number,
		(legacy_u16)gState_topSpeed >> END_SCREEN_SPEED_FRACTION_BITS,
		0, END_SCREEN_NUMBER_WIDTH);
	strcat(&resID_byte1, number);
	end_hiscore_append_text(misc_resource, top_speed_units_id);
	end_hiscore_draw_current_text(&text_y);
	if (gState_jumpCount != 0) {
		end_hiscore_set_text(misc_resource, jump_count_label_id);
		format_integer(number, gState_jumpCount, 0,
			END_SCREEN_NUMBER_WIDTH);
		strcat(&resID_byte1, number);
		hiscore_draw_text(&resID_byte1, font_centered_text_x(&resID_byte1),
			text_y, dialog_fnt_colour, 0);
	}

	animation_resource = 0;
	animation_sequence = 0;
	text_prefix = 0;
	if (opponent_active != 0) {
		if (((legacy_u8)replay_recording_flags &
			REPLAY_RECORDING_RESTARTABLE_FLAG) == 0) {
			previous_end_opening_variant = end_opening_variant;
			previous_end_outcome_variant = end_outcome_variant;
			previous_end_closing_variant = end_closing_variant;
			random_value = (legacy_s16)get_super_random();
			end_opening_variant = (legacy_s16)(random_value %
				END_SCREEN_TEXT_VARIANT_COUNT);
			if (end_opening_variant == previous_end_opening_variant)
				end_opening_variant = end_text_alternate_variant[(legacy_u16)end_opening_variant];
			random_value = (legacy_s16)get_super_random();
			end_closing_variant = (legacy_s16)(random_value %
				END_SCREEN_TEXT_VARIANT_COUNT);
			if (end_closing_variant == previous_end_closing_variant)
				end_closing_variant = end_text_alternate_variant[(legacy_u16)end_closing_variant];

			random_value = (legacy_s16)get_super_random();
			if (outcome == END_SCREEN_OUTCOME_WIN) {
				end_outcome_variant = (legacy_s16)(random_value %
					END_SCREEN_WIN_VARIANT_COUNT);
				if (gState_total_finish_time != 0)
					end_outcome_variant = LEGACY_S16_WRAP_ADD(
						end_outcome_variant,
						END_SCREEN_FINISHED_VARIANT_OFFSET);
			} else {
				end_outcome_variant = (legacy_s16)(random_value %
					END_SCREEN_OUTCOME_VARIANT_COUNT);
			}
			if (end_outcome_variant == previous_end_outcome_variant) {
				end_outcome_variant = end_outcome_alternate_variant[
					(legacy_u16)end_outcome_variant];
			}
		}

		if (outcome == END_SCREEN_OUTCOME_WIN) {
			opponent_win_animation_name[3] = (legacy_s8)(opponent_active + '0');
			animation_resource = (legacy_s8 far*)file_load_resource(
				FILE_RESOURCE_SHAPE2D_COLLECTION, opponent_win_animation_name);
			animation_sequence = (legacy_u8 far*)locate_shape_alt(
				opponent_resource, opponent_win_text_id);
			end_outcome_variant = (legacy_s16)(
				LEGACY_U16_WRAP_ADD(get_kevinrandom(), gState_frame) &
				END_SCREEN_BINARY_RANDOM_MASK);
			if (gState_total_finish_time != 0)
				end_outcome_variant = LEGACY_S16_WRAP_ADD(
					end_outcome_variant,
					END_SCREEN_FINISHED_VARIANT_OFFSET);
			text_prefix = 'v';
		} else {
			opponent_loss_animation_name[3] = (legacy_s8)(opponent_active + '0');
			animation_resource = (legacy_s8 far*)file_load_resource(
				FILE_RESOURCE_SHAPE2D_COLLECTION, opponent_loss_animation_name);
			animation_sequence = (legacy_u8 far*)locate_shape_alt(
				opponent_resource, opponent_loss_text_id);
			end_outcome_variant = (legacy_s16)(
				LEGACY_U16_WRAP_ADD(get_kevinrandom(), gState_frame) &
				END_SCREEN_FOUR_WAY_RANDOM_MASK);
			text_prefix = 'd';
		}
	}

	score_status = 0;
	file_build_path(track_directory, gameconfig.game_trackname,
		track_file_extension, g_path_buf);
	track_resource = (legacy_u8 far*)file_load_resource(
		FILE_RESOURCE_BINARY_OPTIONAL, g_path_buf);
	if (track_resource == 0) {
		result = show_dialog(DIALOG_TYPE_ACKNOWLEDGEMENT,
			DIALOG_SAVE_BACKGROUND,
			locate_text_res(mainresptr, highscore_track_disk_prompt_id),
			DIALOG_AUTO_POSITION, DIALOG_AUTO_POSITION,
			dialog_border_color, 0, 0);
		if (result != 0)
			track_resource = (legacy_u8 far*)file_load_resource(
				FILE_RESOURCE_BINARY_OPTIONAL, g_path_buf);
	}
	if (track_resource != 0) {
		for (i = 0; i < END_SCREEN_TRACK_VALIDATION_BYTES; i++) {
			if (track_resource[i] != track_element_map[i]) {
				score_status = -1;
				break;
			}
		}
		mmgr_release((legacy_s8 far*)track_resource);
	} else {
		score_status = -1;
	}

	if (score_status == 0 && highscore_load_or_create(0) != 0) {
		if (highscore_load_or_create(1) != 0)
			score_status = -1;
	}
	finish_time = 0;
	if (score_status == 0 && gState_total_finish_time != 0) {
		finish_time = gState_total_finish_time;
		scores = (struct HIGHSCORE_ENTRY far*)track_highscore_table;
		if (((legacy_u8)replay_recording_flags &
			REPLAY_RECORDING_HIGHSCORE_INELIGIBLE_FLAGS) == 0 &&
			scores[HIGHSCORE_LAST_ENTRY_INDEX].time >
				(legacy_u16)finish_time) {
			score_status = 1;
		}
	}

	animation_frame = 0;
	animation_timer = END_SCREEN_ANIMATION_FRAME_TICKS;
	evaluation_screen = 1;

	for (;;) {
	do {
	if (opponent_active != 0 && score_status == 2) {
		score_status = 0;
		sprite_select_render_window();
		highscore_draw_table();
		selected = 1;
		evaluation_screen = 1;
		break;
	}

	if (opponent_active == 0) {
		if (score_status > 0) {
			check_input();
			mouse_draw_opaque_check();
			enter_hiscore(finish_time,
				locate_text_res(misc_resource, solo_highscore_prompt_id), 0);
			score_status = 0;
			blit_mode = MENU_BLIT_MODE_REFRESH;
		} else {
			mouse_draw_opaque_check();
			if (score_status == -1) {
				end_hiscore_set_text(misc_resource, highscore_unavailable_message_id);
				hiscore_draw_text(&resID_byte1,
					font_centered_text_x(&resID_byte1),
					END_SCREEN_NO_SCORE_MESSAGE_Y,
					dialog_fnt_colour, 0);
			} else {
				highscore_draw_table();
			}
		}
		break;
	}

	opponent_animation_frame_id[3] = '1';
	frame_shape = (struct SHAPE2D far*)locate_shape_fatal(
		animation_resource, opponent_animation_frame_id);
	animation_width = LEGACY_S16_WRAP_MUL(shape2d_get_width(frame_shape),
		video_shape_width_scale);
	animation_x = LEGACY_S16_WRAP_SUB(END_SCREEN_ANIMATION_RIGHT,
		animation_width);
	animation_y = LEGACY_S16_WRAP_SUB(
		END_SCREEN_ANIMATION_BOTTOM, shape2d_get_height(frame_shape));
	animation_y = LEGACY_S16_FROM_BITS(
		LEGACY_U16_SAR((legacy_u16)animation_y,
			END_SCREEN_ANIMATION_CENTER_SHIFT));
	draw_three_color_beveled_border(LEGACY_S16_WRAP_SUB(animation_x,
			END_SCREEN_ANIMATION_BORDER_INSET),
		LEGACY_S16_WRAP_SUB(animation_y,
			END_SCREEN_ANIMATION_BORDER_INSET),
		LEGACY_S16_WRAP_ADD(animation_width,
			END_SCREEN_ANIMATION_BORDER_GROWTH),
		LEGACY_S16_WRAP_ADD(shape2d_get_height(frame_shape),
			END_SCREEN_ANIMATION_BORDER_GROWTH),
		dialog_fnt_colour, 0, end_animation_border_shadow_color);
	opponent_animation_frame_id[3] = (legacy_s8)(animation_sequence[animation_frame] + '0');
	shape2d_rle_copy((struct SHAPE2D far*)locate_shape_fatal(
		animation_resource, opponent_animation_frame_id), animation_x, animation_y);
	previous_animation_frame = animation_frame;
	font_set_colors(0, 0);
	end_hiscore_draw_opponent_text(opponent_resource, outcome,
		text_prefix, animation_x);
	evaluation_screen = 0;
	if (score_status <= 0)
		break;

	score_status = 0;
	evaluation_screen = 1;
	draw_button(locate_text_res(misc_resource, result_continue_button_id),
		END_SCREEN_EVALUATION_BUTTON_X, END_SCREEN_BUTTON_Y,
		END_SCREEN_BUTTON_WIDTH, END_SCREEN_BUTTON_HEIGHT,
		button_top_color, button_bottom_color, button_fill_color, 0);
	(void)sprite_blit_to_video(render_window_sprite,
		LEGACY_S8_FROM_BITS(blit_mode));
	blit_mode = MENU_BLIT_MODE_REFRESH;
	menu_reset_animation_timers();
	check_input();
	sprite_select_screen_compat();
	for (i = 0; i < END_SCREEN_MENU_AREA_COUNT; i++) {
		menu_areas[i].x1 = result_button_left[i];
		menu_areas[i].x2 = result_button_right[i];
		menu_areas[i].y1 = hiscore_buttons_y1[i];
		menu_areas[i].y2 = hiscore_buttons_y2[i];
	}
	text_resource_count = outcome == END_SCREEN_OUTCOME_NONE ?
		1U : END_SCREEN_TEXT_VARIANT_COUNT;
	for (;;) {
		delta = (legacy_s16)menu_animate_button_highlight(
			END_SCREEN_BUTTON_COUNT, menu_areas,
			menu_highlight_second_color, menu_highlight_first_color);
		end_hiscore_update_animation(delta, &animation_timer,
			&animation_frame, &previous_animation_frame,
			animation_resource, animation_sequence, animation_x,
			animation_y, animation_sprite, 0);
		input = (legacy_u16)input_checking(
			(legacy_s16)text_resource_count);
		if (input == KEY_ENTER || input == KEY_SPACE ||
			input == KEY_ESCAPE)
			break;
	}

	sprite_select_render_window();
	draw_button(0, 0, 0, END_SCREEN_WIDTH, END_SCREEN_TOP_HEIGHT,
		button_top_color, button_bottom_color, button_fill_color, 0);
	sprite_set_target_clip_bounds(END_SCREEN_TEXT_LEFT, END_SCREEN_ANIMATION_RIGHT,
		hiscore_buttons_y1[0],
		LEGACY_S16_WRAP_ADD(hiscore_buttons_y2[0], 1));
	sprite_clear_target(button_fill_color);
	mouse_draw_opaque_check();
	enter_hiscore(finish_time,
		locate_text_res(misc_resource, opponent_highscore_prompt_id), outcome);

	} while (0);
	selected = 1;
	previous_selection = 1;
	menu_reset_animation_timers();
	sprite_select_render_window();
	if (opponent_active == 0 || score_status == -1) {
		menu_offset = END_SCREEN_MENU_OFFSET_WITHOUT_FIRST_BUTTON;
	} else {
		menu_offset = 0;
		draw_button(locate_text_res(misc_resource,
			evaluation_screen != 0 ? result_evaluation_button_id : result_highscore_button_id),
			LEGACY_S16_WRAP_ADD(result_button_left[0], 1),
			END_SCREEN_BUTTON_Y, END_SCREEN_BUTTON_WIDTH,
			END_SCREEN_BUTTON_HEIGHT,
			button_top_color, button_bottom_color, button_fill_color, 0);
	}
	draw_button(locate_text_res(misc_resource, result_replay_button_id),
		LEGACY_S16_WRAP_ADD(
			LEGACY_S16_WRAP_ADD(result_button_left[1], menu_offset), 1),
		END_SCREEN_BUTTON_Y, END_SCREEN_BUTTON_WIDTH,
		END_SCREEN_BUTTON_HEIGHT,
		button_top_color, button_bottom_color, button_fill_color, 0);
	draw_button(locate_text_res(misc_resource,
		opponent_active != 0 ? result_race_button_id : result_drive_button_id),
		LEGACY_S16_WRAP_ADD(
			LEGACY_S16_WRAP_ADD(result_button_left[2], menu_offset), 1),
		END_SCREEN_BUTTON_Y, END_SCREEN_BUTTON_WIDTH,
		END_SCREEN_BUTTON_HEIGHT,
		button_top_color, button_bottom_color, button_fill_color, 0);
	draw_button(locate_text_res(misc_resource, result_main_menu_button_data),
		LEGACY_S16_WRAP_ADD(
			LEGACY_S16_WRAP_ADD(result_button_left[3], menu_offset), 1),
		END_SCREEN_BUTTON_Y, END_SCREEN_BUTTON_WIDTH,
		END_SCREEN_BUTTON_HEIGHT,
		button_top_color, button_bottom_color, button_fill_color, 0);
	for (i = 0; i < END_SCREEN_BUTTON_COUNT; i++) {
		button_areas[i].x1 = LEGACY_S16_WRAP_ADD(result_button_left[i], menu_offset);
		button_areas[i].x2 = LEGACY_S16_WRAP_ADD(result_button_right[i], menu_offset);
		button_areas[i].y1 = hiscore_buttons_y1[i];
		button_areas[i].y2 = hiscore_buttons_y2[i];
	}
	check_input();
	(void)sprite_blit_to_video(render_window_sprite,
		LEGACY_S8_FROM_BITS(blit_mode));
	blit_mode = MENU_BLIT_MODE_REFRESH;
	sprite_select_screen_compat();

	for (;;) {
	if (previous_selection != selected) {
		previous_selection = selected;
		sprite_select_screen_compat();
		sprite_set_target_clip_bounds(0, END_SCREEN_WIDTH,
			hiscore_buttons_y1[0],
			LEGACY_S16_WRAP_ADD(hiscore_buttons_y2[0], 1));
		mouse_draw_opaque_check();
		sprite_putimage(render_window_sprite->sprite_bitmapptr);
		mouse_draw_transparent_check();
		(void)timer_get_delta_alt();
		menu_reset_animation_timers();
	}

		delta = (legacy_s16)menu_animate_button_highlight(selected, button_areas,
			menu_highlight_second_color, menu_highlight_first_color);
		if (evaluation_screen == 0 &&
			outcome != END_SCREEN_OUTCOME_NONE) {
			end_hiscore_update_animation(delta, &animation_timer,
				&animation_frame, &previous_animation_frame,
				animation_resource, animation_sequence, animation_x,
				animation_y, animation_sprite, 1);
		}

	if (opponent_active == 0 || score_status == -1) {
		hit = (legacy_s16)mouse_multi_hittest(
			END_SCREEN_REDUCED_BUTTON_COUNT, &button_areas[1]);
		if (hit != -1)
			selected = (legacy_u8)(hit + 1);
	} else {
		hit = (legacy_s16)mouse_multi_hittest(
			END_SCREEN_BUTTON_COUNT, button_areas);
		if (hit != -1)
			selected = (legacy_u8)hit;
	}

	input = (legacy_u16)input_checking(delta);
	if (input == 0)
		continue;
	if (input == KEY_LEFT) {
		if (opponent_active == 0 || score_status == -1) {
			selected = selected <= 1 ? END_SCREEN_LAST_BUTTON_INDEX :
				(legacy_u8)(selected - 1U);
		} else {
			selected = selected == 0 ? END_SCREEN_LAST_BUTTON_INDEX :
				(legacy_u8)(selected - 1U);
		}
		continue;
	}
	if (input == KEY_RIGHT) {
		if (selected < END_SCREEN_LAST_BUTTON_INDEX)
			selected++;
		else
			selected = (opponent_active == 0 ||
				score_status == -1) ? 1U : 0U;
		continue;
	}
	if (input != KEY_ENTER && input != KEY_SPACE)
		continue;

	if (selected == 0) {
		sprite_select_render_window();
		draw_button(0, 0, 0, END_SCREEN_WIDTH, END_SCREEN_TOP_HEIGHT,
			button_top_color, button_bottom_color, button_fill_color, 0);
		score_status = evaluation_screen != 0 ? 0 : 2;
		break;
	}

	audio_unload();
	if (opponent_active != 0)
		mmgr_release(animation_resource);
	if (video_uses_page_flipping != 0)
		sprite_free_wnd(animation_sprite);
	sprite_free_wnd(render_window_sprite);
	if (gameconfig.game_opponenttype != 0)
		unload_resource(opponent_resource);
	unload_resource(misc_resource);
	return (legacy_u16)(selected - 1U);
	}
	}
}
