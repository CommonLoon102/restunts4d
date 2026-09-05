#include "fileio.h"
#include "game_input.h"
#include "keyboard.h"
#include "memmgr.h"
#include "menu_internal.h"
#include "platform.h"
#include "resource.h"
#include "shape2d.h"
#include "timing.h"

#define INPUT_DIRECTION_COUNT 16U
#define INPUT_KEY_COUNT 10U
#define INPUT_CALLBACK_COUNT 64U
#define INPUT_ASCII_KEY_COUNT 128U
#define INPUT_EXTENDED_KEY_COUNT 133U
#define INPUT_EXTENDED_KEY_MAX_INDEX 132U
#define INPUT_ASCII_BYTE_MASK 255U
#define INPUT_ASCII_INDEX_MASK 127U
#define INPUT_MODE_STACK_LIMIT 8U
#define INPUT_REPEAT_DELAY_FRAMES 20U
#define INPUT_IDLE_LIMIT_FRAMES 500
#define INPUT_COUNTER_WRAP_LIMIT 20000
#define INPUT_COUNTER_WRAP_AMOUNT 10000U
#define VGA_PALETTE_BYTE_COUNT 768U
#define VGA_PALETTE_COLOR_COUNT 256U
#define MOUSE_SPRITE_TRANSPARENT_COLOR 15U
#define MOUSE_LEFT_BUTTON 1U
#define MOUSE_RIGHT_BUTTON 2U
#define MOUSE_BUTTON_MASK 3U
#define MCGA_SCREEN_WIDTH 320
#define MCGA_SCREEN_HEIGHT 200
#define MCGA_SCREEN_CENTER_X 160
#define MCGA_SCREEN_CENTER_Y 100
#define MOUSE_SCREEN_INSET 15

static const legacy_u8 far input_direction_table[INPUT_DIRECTION_COUNT] = {
	0, 1, 5, 0, 3, 2, 4, 3, 7, 8, 6, 7, 0, 1, 5, 0
};
static legacy_u8 input_callback_flags[INPUT_ASCII_KEY_COUNT];
static legacy_u8 input_extended_callback_flags[INPUT_EXTENDED_KEY_COUNT];
static void (far* input_callbacks[INPUT_CALLBACK_COUNT])(void);
static legacy_u8 input_callback_dispatching;
static readchar_callback_type input_readchar_callback = kb_read_char;
struct SPRITE far* mouse_small_sprite;
struct SPRITE far* mouse_medium_sprite;
struct SPRITE far* mouse_background_sprite;
legacy_s8 mouse_background_dirty;
static legacy_s8 mouse_transparent_mode;
static legacy_u8 h_key_toggle;
static legacy_s16 input_elapsed_frames;

/* A control that has not changed still fires again once its 20-frame repeat
   delay has elapsed. */
static legacy_s16 input_repeat_due(legacy_s16 repeat_at)
{
	return LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_ADD(
		repeat_at, INPUT_REPEAT_DELAY_FRAMES)) <
		LEGACY_S16_FROM_BITS(input_elapsed_frames);
}
static legacy_s16 input_mouse_repeat_at;
static legacy_s16 input_joystick_repeat_at;
static legacy_s16 input_mouse_idle_frames;
legacy_s16 input_combined_flags;
static legacy_s16 input_joystick_flags;
static legacy_s16 input_new_joystick_flags;
static legacy_s16 input_joystick_keycode;
static legacy_s16 input_mouse_previous_x;
static legacy_s16 input_mouse_previous_y;
static legacy_s16 input_mouse_previous_buttons;
static legacy_s16 input_mouse_keycode;
static const legacy_u8 input_key_scancodes[INPUT_KEY_COUNT] = {
	57, 28, 71, 72, 73, 77, 81, 80, 79, 75
};
static legacy_u8 input_mode_stack_depth;
static legacy_s8 input_draw_mode_stack[INPUT_MODE_STACK_LIMIT];
static legacy_s8 input_device_mode_stack[INPUT_MODE_STACK_LIMIT];

void kb_reg_callback(legacy_s16 code, void (far* callback)(void))
{
	legacy_u16 code_bits;
	legacy_u16 callback_index;
	legacy_u16 key_index;

	for (callback_index = 0; callback_index < INPUT_CALLBACK_COUNT;
		callback_index++) {
		if (input_callbacks[callback_index] == callback)
			break;
		if (dos_memory_pointer_segment(input_callbacks[callback_index]) == 0U) {
			input_callbacks[callback_index] = callback;
			break;
		}
	}
	if (callback_index == INPUT_CALLBACK_COUNT)
		return;

	code_bits = (legacy_u16)code;
	if ((code_bits & INPUT_ASCII_BYTE_MASK) != 0) {
		if (code_bits <= INPUT_ASCII_INDEX_MASK)
			input_callback_flags[code_bits] = (legacy_u8)(callback_index + 1U);
		return;
	}
	key_index = (legacy_u16)(code_bits >> 8);
	if (key_index <= INPUT_EXTENDED_KEY_MAX_INDEX)
		input_extended_callback_flags[key_index] = (legacy_u8)(callback_index + 1U);
}

legacy_s16 kb_parse_key(legacy_s16 code)
{
	legacy_u16 code_bits;
	legacy_u16 key_index;
	legacy_u8 callback_number;

	code_bits = (legacy_u16)code;
	dos_interrupts_disable();
	if (input_callback_dispatching != 0) {
		dos_interrupts_enable();
		return LEGACY_S16_FROM_BITS(code_bits);
	}
	input_callback_dispatching = 1;
	dos_interrupts_enable();

	if ((code_bits & INPUT_ASCII_BYTE_MASK) != 0) {
		key_index = code_bits & INPUT_ASCII_INDEX_MASK;
		callback_number = input_callback_flags[key_index];
		code_bits = key_index;
	} else {
		key_index = code_bits >> 8;
		if (key_index >= INPUT_EXTENDED_KEY_MAX_INDEX)
			key_index = INPUT_EXTENDED_KEY_MAX_INDEX;
		callback_number = input_extended_callback_flags[key_index];
	}

	if (callback_number != 0) {
		input_callbacks[(legacy_u16)callback_number - 1U]();
		code_bits = 0;
	}
	input_callback_dispatching = 0;
	return LEGACY_S16_FROM_BITS(code_bits);
}

void nopsub_304AF(legacy_s16 code)
{
	legacy_u16 code_bits;
	legacy_u16 key_index;

	code_bits = (legacy_u16)code;
	if ((code_bits & INPUT_ASCII_BYTE_MASK) != 0) {
		if (code_bits <= INPUT_ASCII_INDEX_MASK)
			input_callback_flags[code_bits] = 0;
		return;
	}
	key_index = (legacy_u16)(code_bits >> 8);
	if (key_index <= INPUT_EXTENDED_KEY_MAX_INDEX)
		input_extended_callback_flags[key_index] = 0;
}

void nopsub_kb_set_readchar_callback(readchar_callback_type callback)
{
	input_readchar_callback = callback;
}

readchar_callback_type nopsub_kb_get_readchar_callback(void)
{
	return input_readchar_callback;
}

void sub_307B4(void)
{
	dos_joystick_reset_calibration();
}

legacy_s16 sub_307D2(legacy_s16 index)
{
	return input_direction_table[(legacy_u16)index & INPUT_DRIVING_MASK];
}

legacy_s16 sub_307E3(void)
{
	return dos_joystick_get_scaled_axis(0U);
}

legacy_s16 nopsub_307FA(void)
{
	return dos_joystick_get_scaled_axis(1U);
}

void load_palandcursor(void)
{
	legacy_u8 palette[VGA_PALETTE_BYTE_COUNT];
	legacy_s8 far* resource;
	struct SHAPE2D far* mouse_shape;
	legacy_u16 mouse_width;
	legacy_u16 mouse_height;
	legacy_u16 i;

	resource = (legacy_s8 far*)file_load_shape2d_fatal("sdmain");
	mouse_shape = (struct SHAPE2D far*)locate_shape_fatal(resource, "!pal");
	for (i = 0; i < sizeof(palette); ++i)
		palette[i] = ((legacy_u8 far*)mouse_shape)[SHAPE2D_HEADER_SIZE + i];
	dos_video_set_palette(0, VGA_PALETTE_COLOR_COUNT, palette);

	mouse_shape = (struct SHAPE2D far*)locate_shape_fatal(resource, "smou");
	mouse_width = (legacy_u16)(shape2d_get_width(mouse_shape) *
		video_flag2_is1);
	mouse_height = shape2d_get_height(mouse_shape);
	mmgr_free(resource);

	mouse_small_sprite = sprite_make_wnd(mouse_width, mouse_height,
		MOUSE_SPRITE_TRANSPARENT_COLOR);
	mouse_medium_sprite = sprite_make_wnd(mouse_width, mouse_height,
		MOUSE_SPRITE_TRANSPARENT_COLOR);
	mouse_background_sprite = sprite_make_wnd(
		mouse_width + video_flag2_is1, mouse_height,
		MOUSE_SPRITE_TRANSPARENT_COLOR);

	resource = (legacy_s8 far*)file_load_shape2d_fatal("sdmain");
	sprite_set_1_from_argptr(mouse_small_sprite);
	mouse_shape = (struct SHAPE2D far*)locate_shape_fatal(resource, "smou");
	sprite_shape_to_1(mouse_shape, 0, 0);

	sprite_set_1_from_argptr(mouse_medium_sprite);
	mouse_shape = (struct SHAPE2D far*)locate_shape_fatal(resource, "mmou");
	sprite_shape_to_1(mouse_shape, 0, 0);

	mmgr_free(resource);
	sprite_copy_2_to_1_2();
}

legacy_s16 handle_ingame_kb_shortcuts(legacy_s16 key)
{
	switch (key) {
	case KEY_ESCAPE:
		if (game_replay_mode == 0)
			update_crash_state(4, 0);
		byte_449DA = 1;
		return 1;

	case 'D':
	case 'd':
		dashb_toggle ^= 1;
		return 1;

	case 'H':
	case 'h':
		h_key_toggle ^= 1;
		return 1;

	case 'M':
	case 'm':
		do_mou_restext();
		mouse_minmax_position(LEGACY_S8_FROM_BITS(byte_3B8F2));
		return 1;

	case 'R':
	case 'r':
		replaybar_toggle ^= 1;
		return 1;

	case 'C':
	case 'c':
		if (game_replay_mode != 1) {
			cameramode++;
			if (cameramode == 4)
				cameramode = 0;
		}
		return 1;

	case 't':
		if (gameconfig.game_opponenttype != 0)
			followOpponentFlag ^= 1;
		return 1;

	case KEY_F1:
		cameramode = 0;
		return 1;
	case KEY_F2:
		cameramode = 1;
		return 1;
	case KEY_F3:
		cameramode = 2;
		return 1;
	case KEY_F4:
		cameramode = 3;
		return 1;
	}

	if (game_replay_mode != 1)
		return 0;

	game_replay_mode = 0;
	byte_4393C = 0;
	init_game_state_with_frame_rate_byte(framespersec2);
	return 1;
}

void mouse_draw_transparent_check(void)
{
	mouse_transparent_mode = 1;
	if (kbormouse != 0 && mouse_background_dirty == 0)
		mouse_draw_transparent();
}

void mouse_draw_opaque_check(void)
{
	mouse_transparent_mode = 0;
	if (mouse_background_dirty != 0)
		mouse_draw_opaque();
}

legacy_s16 mouse_multi_hittest(legacy_s16 count,
	const struct BUTTON_AREA* buttons)
{
	legacy_s16 i;

	if (kbormouse == 0)
		return -1;

	for (i = 0; i < count; i++) {
		if (buttons[i].x1 <= mouse_xpos && mouse_xpos <= buttons[i].x2 &&
			buttons[i].y1 <= mouse_ypos && mouse_ypos <= buttons[i].y2)
			return (legacy_s8)i;
	}

	return -1;
}

legacy_s16 get_kb_or_joy_flags(void)
{
	static const legacy_u8 action_flags[INPUT_KEY_COUNT] = {
		INPUT_PRIMARY_ACTION_FLAG, INPUT_SECONDARY_ACTION_FLAG,
		INPUT_ACCELERATE_FLAG | INPUT_STEER_LEFT_FLAG,
		INPUT_ACCELERATE_FLAG,
		INPUT_ACCELERATE_FLAG | INPUT_STEER_RIGHT_FLAG,
		INPUT_STEER_RIGHT_FLAG,
		INPUT_BRAKE_FLAG | INPUT_STEER_RIGHT_FLAG,
		INPUT_BRAKE_FLAG,
		INPUT_BRAKE_FLAG | INPUT_STEER_LEFT_FLAG,
		INPUT_STEER_LEFT_FLAG
	};
	legacy_u16 flags;
	legacy_u16 index;

	flags = 0;
	for (index = 0; index < INPUT_KEY_COUNT; index++) {
		if (kb_get_key_state(input_key_scancodes[index]) != 0)
			flags |= action_flags[index];
	}
	if (flags == 0)
		flags = (legacy_u16)dos_get_joy_flags();
	return LEGACY_S16_FROM_BITS(flags);
}

legacy_s16 input_checking(legacy_s16 frame_delta)
{
	legacy_u16 current_joy_flags;
	legacy_u16 key;
	legacy_s16 changed_or_repeating;

	input_elapsed_frames = LEGACY_U16_WRAP_ADD(input_elapsed_frames, frame_delta);
	if (LEGACY_S16_FROM_BITS(input_elapsed_frames) >
		INPUT_COUNTER_WRAP_LIMIT) {
		input_elapsed_frames = LEGACY_U16_WRAP_SUB(input_elapsed_frames,
			INPUT_COUNTER_WRAP_AMOUNT);
		input_mouse_repeat_at = LEGACY_U16_WRAP_SUB(input_mouse_repeat_at,
			INPUT_COUNTER_WRAP_AMOUNT);
		input_joystick_repeat_at = LEGACY_U16_WRAP_SUB(
			input_joystick_repeat_at, INPUT_COUNTER_WRAP_AMOUNT);
	}

	key = (legacy_u16)dos_kb_get_char();
	if (key != 0)
		kbormouse = 0;
	current_joy_flags = (legacy_u16)dos_get_joy_flags();
	input_combined_flags = get_kb_or_joy_flags();
	changed_or_repeating = 0;
	if ((legacy_u16)input_joystick_flags != current_joy_flags) {
		input_new_joystick_flags = ((legacy_u16)input_joystick_flags ^ current_joy_flags) &
			current_joy_flags;
		input_joystick_flags = current_joy_flags;
		changed_or_repeating = 1;
	} else if (current_joy_flags != 0 &&
		input_repeat_due(input_joystick_repeat_at)) {
		changed_or_repeating = 1;
	}

	if (changed_or_repeating) {
		if (((legacy_u16)input_new_joystick_flags &
			INPUT_SECONDARY_ACTION_FLAG) != 0)
			input_joystick_keycode = KEY_ENTER;
		else if (((legacy_u16)input_new_joystick_flags &
			INPUT_PRIMARY_ACTION_FLAG) != 0)
			input_joystick_keycode = KEY_SPACE;
		else if (((legacy_u16)input_new_joystick_flags &
			INPUT_ACCELERATE_FLAG) != 0)
			input_joystick_keycode = KEY_UP;
		else if (((legacy_u16)input_new_joystick_flags &
			INPUT_BRAKE_FLAG) != 0)
			input_joystick_keycode = KEY_DOWN;
		else if (((legacy_u16)input_new_joystick_flags &
			INPUT_STEER_LEFT_FLAG) != 0)
			input_joystick_keycode = KEY_LEFT;
		else if (((legacy_u16)input_new_joystick_flags &
			INPUT_STEER_RIGHT_FLAG) != 0)
			input_joystick_keycode = KEY_RIGHT;

		if (input_joystick_keycode != 0) {
			input_joystick_repeat_at = input_elapsed_frames;
			kbormouse = 0;
		}
	}

	dos_mouse_get_state(&mouse_butstate, &mouse_xpos, &mouse_ypos);
	if (input_mouse_previous_x != mouse_xpos || input_mouse_previous_y != mouse_ypos ||
		input_mouse_previous_buttons != mouse_butstate) {
		input_mouse_previous_x = mouse_xpos;
		input_mouse_previous_y = mouse_ypos;
		kbormouse = 1;
		input_mouse_idle_frames = 0;
		if (mouse_transparent_mode != 0) {
			if (mouse_background_dirty != 0)
				mouse_draw_opaque();
			mouse_draw_transparent();
		}
	} else if (kbormouse != 0) {
		input_mouse_idle_frames = LEGACY_U16_WRAP_ADD(
			input_mouse_idle_frames, frame_delta);
		if (LEGACY_S16_FROM_BITS(input_mouse_idle_frames) >
			INPUT_IDLE_LIMIT_FRAMES) {
			input_mouse_idle_frames = 0;
			kbormouse = 0;
			if (mouse_background_dirty != 0)
				mouse_draw_opaque();
		}
	}

	if (kbormouse != 0) {
		changed_or_repeating = 0;
		if (input_mouse_previous_buttons != mouse_butstate) {
			input_mouse_previous_buttons = mouse_butstate;
			changed_or_repeating = 1;
		} else if (mouse_butstate != 0 &&
			input_repeat_due(input_mouse_repeat_at)) {
			changed_or_repeating = 1;
		}

		if (changed_or_repeating) {
			if (((legacy_u16)mouse_butstate & MOUSE_LEFT_BUTTON) != 0)
				input_mouse_keycode = KEY_SPACE;
			else if (((legacy_u16)mouse_butstate & MOUSE_RIGHT_BUTTON) != 0)
				input_mouse_keycode = KEY_ENTER;
			if (input_mouse_keycode != 0)
				input_mouse_repeat_at = input_elapsed_frames;
			input_mouse_idle_frames = 0;
		}

		if (mouse_butstate != 0) {
			if (((legacy_u16)mouse_butstate & MOUSE_LEFT_BUTTON) != 0)
				input_combined_flags = (legacy_u16)input_combined_flags |
					INPUT_SECONDARY_ACTION_FLAG;
			else if (((legacy_u16)mouse_butstate & MOUSE_RIGHT_BUTTON) != 0)
				input_combined_flags = (legacy_u16)input_combined_flags |
					INPUT_PRIMARY_ACTION_FLAG;
		}
	}

	if (key != 0)
		return key;
	if (input_joystick_keycode != 0) {
		key = (legacy_u16)input_joystick_keycode;
		input_joystick_keycode = 0;
		return key;
	}
	if (input_mouse_keycode != 0) {
		key = (legacy_u16)input_mouse_keycode;
		input_mouse_keycode = 0;
		return key;
	}
	return 0;
}

static legacy_s16 mouse_track_divide(legacy_s16 numerator,
	legacy_s16 denominator)
{
	return LEGACY_S16_FROM_BITS((legacy_u16)
		LEGACY_S32_DIV_OR_ZERO(
			(legacy_s32)numerator, (legacy_s32)denominator));
}

static legacy_s16 mouse_track_position(legacy_s16 length,
	legacy_s16 selected, legacy_s16 item_count)
{
	legacy_s16 numerator;
	legacy_s16 denominator;

	numerator = LEGACY_S16_WRAP_MUL(
		LEGACY_S16_WRAP_SUB(length, 1), selected);
	numerator = LEGACY_S16_WRAP_MUL(numerator, 4);
	denominator = LEGACY_S16_WRAP_MUL(item_count, 4);
	return mouse_track_divide(numerator, denominator);
}

static legacy_s16 mouse_track_thumb_size(legacy_s16 length,
	legacy_s16 selected, legacy_s16 selection_width, legacy_s16 item_count,
	legacy_s16* thumb_start, legacy_s16* thumb_end)
{
	*thumb_start = mouse_track_position(length, selected, item_count);
	*thumb_end = mouse_track_position(length,
		LEGACY_S16_WRAP_ADD(selected, selection_width), item_count);
	return LEGACY_S16_WRAP_SUB(*thumb_end, *thumb_start);
}

static void mouse_track_draw(legacy_s16 horizontal, legacy_s16 x, legacy_s16 width, legacy_s16 y,
	legacy_s16 height, legacy_s16 thumb_start, legacy_s16 thumb_size)
{
	sprite_1_unk(x, y, width, height, 0);
	if (horizontal) {
		sprite_1_unk(LEGACY_S16_WRAP_ADD(x, thumb_start), y,
			thumb_size, height, dialog_fnt_colour);
	} else {
		sprite_1_unk(x, LEGACY_S16_WRAP_ADD(y, thumb_start),
			width, thumb_size, dialog_fnt_colour);
	}
}

legacy_s16 mouse_track_op(legacy_s16 operation, legacy_s16 x, legacy_s16 width, legacy_s16 y, legacy_s16 height,
	legacy_s16 selected, legacy_s16 selection_width, legacy_s16 item_count)
{
	legacy_s16 length;
	legacy_s16 thumb_start;
	legacy_s16 thumb_end;
	legacy_s16 thumb_size;
	legacy_s16 coordinate;
	legacy_s16 current_coordinate;
	legacy_s16 dragged_start;
	legacy_s16 previous_start;
	legacy_s16 quotient;
	legacy_s16 scaled;
	legacy_s16 horizontal;

	horizontal = LEGACY_S16_FROM_BITS(width) >
		LEGACY_S16_FROM_BITS(height);
	length = horizontal ? (legacy_s16)width : (legacy_s16)height;
	thumb_size = mouse_track_thumb_size(length, selected, selection_width,
		item_count, &thumb_start, &thumb_end);

	if (operation == 0) {
		mouse_track_draw(horizontal, x, width, y, height,
			thumb_start, thumb_size);
		return selected;
	}
	if (operation != 1)
		return selected;

	coordinate = horizontal ?
		LEGACY_S16_WRAP_SUB(mouse_xpos, x) :
		LEGACY_S16_WRAP_SUB(mouse_ypos, y);
	if (coordinate < thumb_start || coordinate > thumb_end) {
		do {
			input_checking((legacy_s16)timer_get_delta_alt());
		} while (((legacy_u16)mouse_butstate & MOUSE_BUTTON_MASK) != 0);
		if (coordinate < thumb_start) {
			if (selected != 0)
				selected = LEGACY_S16_WRAP_SUB(selected, 1);
		} else if (LEGACY_S16_FROM_BITS(selected) <
			LEGACY_S16_WRAP_SUB(item_count, 1)) {
			selected = LEGACY_S16_WRAP_ADD(selected, 1);
		}
	} else {
		selected = -1;
		previous_start = thumb_start;
		do {
			input_checking((legacy_s16)timer_get_delta_alt());
			current_coordinate = horizontal ?
				LEGACY_S16_WRAP_SUB(mouse_xpos, x) :
				LEGACY_S16_WRAP_SUB(mouse_ypos, y);
			dragged_start = LEGACY_S16_WRAP_ADD(
				LEGACY_S16_WRAP_SUB(current_coordinate, coordinate),
				thumb_start);
			if (dragged_start < 0)
				dragged_start = 0;
			else if (LEGACY_S16_WRAP_ADD(dragged_start, thumb_size) >
				LEGACY_S16_WRAP_SUB(length, 1))
				dragged_start = LEGACY_S16_WRAP_SUB(
					LEGACY_S16_WRAP_SUB(length, thumb_size), 1);

			if (dragged_start != previous_start) {
				previous_start = dragged_start;
				mouse_draw_opaque_check();
				mouse_track_draw(horizontal, x, width, y, height,
					dragged_start, thumb_size);
				mouse_draw_transparent_check();
			}
		} while (((legacy_u16)mouse_butstate & MOUSE_BUTTON_MASK) != 0);
	}

	if (selected == -1) {
		quotient = mouse_track_divide(length, (legacy_s16)item_count);
		quotient = LEGACY_S16_SAR(quotient, 1U);
		scaled = LEGACY_S16_WRAP_MUL(
			LEGACY_S16_WRAP_ADD(dragged_start, quotient), item_count);
		selected = mouse_track_divide(scaled, length);
	}

	thumb_size = mouse_track_thumb_size(length, selected, selection_width,
		item_count, &thumb_start, &thumb_end);
	mouse_draw_opaque_check();
	mouse_track_draw(horizontal, x, width, y, height,
		thumb_start, thumb_size);
	mouse_draw_transparent_check();
	return selected;
}

legacy_s16 input_do_checking(legacy_s16 frame_delta)
{
	return input_checking(frame_delta);
}

void check_input(void)
{
	legacy_s16 pressed;

	do {
		pressed = (get_kb_or_joy_flags() & INPUT_ACTION_BUTTON_MASK) != 0;
		if (!pressed) {
			pressed = input_checking(
				(legacy_s16)timer_get_delta_alt()) != 0;
		}
		if (!pressed && kbormouse != 0 &&
			(mouse_butstate & MOUSE_BUTTON_MASK) != 0)
			pressed = 1;
	} while (pressed);
}

void nopsub_28F26(void)
{
	do {
		/* Keep advancing input state until an event is reported. */
	} while (input_checking((legacy_s16)timer_get_delta_alt()) == 0);

	check_input();
}

void input_push_status(void)
{
	legacy_s16 index = (legacy_s8)input_mode_stack_depth;

	input_draw_mode_stack[index] = mouse_transparent_mode;
	input_device_mode_stack[index] = kbormouse;
	input_mode_stack_depth++;
}

void input_pop_status(void)
{
	legacy_s16 index;

	if (input_mode_stack_depth == 0)
		return;

	input_mode_stack_depth--;
	index = (legacy_s8)input_mode_stack_depth;
	mouse_transparent_mode = input_draw_mode_stack[index];
	kbormouse = input_device_mode_stack[index];
	if (kbormouse == 0)
		mouse_draw_opaque_check();
}

legacy_s16 input_repeat_check(legacy_s16 duration)
{
	legacy_u16 delta;
	legacy_u16 elapsed;
	legacy_s16 result;

	elapsed = 0;
	timer_get_delta_alt();
	while (LEGACY_S16_FROM_BITS((legacy_u16)duration) >
		LEGACY_S16_FROM_BITS(elapsed)) {
		delta = (legacy_u16)timer_get_delta_alt();
		elapsed = LEGACY_U16_WRAP_ADD(elapsed, delta);
		result = input_do_checking(LEGACY_S16_FROM_BITS(delta));
		if (result != 0)
			return result;
	}
	return 0;
}

void mouse_minmax_position(legacy_s16 inset)
{
	if (inset != 0) {
		dos_mouse_set_minmax(MOUSE_SCREEN_INSET, 0,
			MCGA_SCREEN_WIDTH - MOUSE_SCREEN_INSET, MCGA_SCREEN_HEIGHT);
		dos_mouse_set_position(MCGA_SCREEN_CENTER_X, MCGA_SCREEN_CENTER_Y);
	} else {
		dos_mouse_set_minmax(0, 0, MCGA_SCREEN_WIDTH, MCGA_SCREEN_HEIGHT);
	}
}
