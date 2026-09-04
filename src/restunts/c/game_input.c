#include "fileio.h"
#include "game_input.h"
#include "keyboard.h"
#include "memmgr.h"
#include "menu_internal.h"
#include "platform.h"
#include "resource.h"
#include "shape2d.h"
#include "timing.h"

static const legacy_u8 far input_direction_table[16] = {
	0, 1, 5, 0, 3, 2, 4, 3, 7, 8, 6, 7, 0, 1, 5, 0
};
static legacy_u8 input_callback_flags[128];
static legacy_u8 input_extended_callback_flags[133];
static void (far* input_callbacks[64])(void);
static legacy_u8 input_callback_dispatching;
static readchar_callback_type input_readchar_callback = kb_read_char;
struct SPRITE far* mouse_small_sprite;
struct SPRITE far* mouse_medium_sprite;
struct SPRITE far* mouse_background_sprite;
legacy_s8 mouse_background_dirty;
static legacy_s8 mouse_transparent_mode;
static legacy_u8 h_key_toggle;
static legacy_s16 input_elapsed_frames;
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
static const legacy_u8 input_key_scancodes[10] = {
	57, 28, 71, 72, 73, 77, 81, 80, 79, 75
};
static legacy_u8 input_mode_stack_depth;
static legacy_s8 input_draw_mode_stack[8];
static legacy_s8 input_device_mode_stack[8];

void kb_reg_callback(legacy_s16 code, void (far* callback)(void))
{
	legacy_u16 code_bits;
	legacy_u16 callback_index;
	legacy_u16 key_index;

	for (callback_index = 0; callback_index < 64U; callback_index++) {
		if (input_callbacks[callback_index] == callback)
			break;
		if (dos_memory_pointer_segment(input_callbacks[callback_index]) == 0U) {
			input_callbacks[callback_index] = callback;
			break;
		}
	}
	if (callback_index == 64U)
		return;

	code_bits = (legacy_u16)code;
	if ((code_bits & 0x00FFU) != 0) {
		if (code_bits <= 0x007FU)
			input_callback_flags[code_bits] = (legacy_u8)(callback_index + 1U);
		return;
	}
	key_index = (legacy_u16)(code_bits >> 8);
	if (key_index <= 0x84U)
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

	if ((code_bits & 0x00FFU) != 0) {
		key_index = code_bits & 0x007FU;
		callback_number = input_callback_flags[key_index];
		code_bits = key_index;
	} else {
		key_index = code_bits >> 8;
		if (key_index >= 0x84U)
			key_index = 0x84U;
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
	if ((code_bits & 0x00FFU) != 0) {
		if (code_bits <= 0x007FU)
			input_callback_flags[code_bits] = 0;
		return;
	}
	key_index = (legacy_u16)(code_bits >> 8);
	if (key_index <= 0x84U)
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
	return input_direction_table[(legacy_u16)index & 0x0FU];
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
	legacy_u8 palette[0x300];
	legacy_s8 far* resource;
	struct SHAPE2D far* mouse_shape;
	legacy_u16 mouse_width;
	legacy_u16 mouse_height;
	legacy_u16 i;

	resource = (legacy_s8 far*)file_load_shape2d_fatal("sdmain");
	mouse_shape = (struct SHAPE2D far*)locate_shape_fatal(resource, "!pal");
	for (i = 0; i < sizeof(palette); ++i)
		palette[i] = ((legacy_u8 far*)mouse_shape)[0x10U + i];
	dos_video_set_palette(0, 0x100, palette);

	mouse_shape = (struct SHAPE2D far*)locate_shape_fatal(resource, "smou");
	mouse_width = (legacy_u16)(shape2d_get_width(mouse_shape) *
		video_flag2_is1);
	mouse_height = shape2d_get_height(mouse_shape);
	mmgr_free(resource);

	mouse_small_sprite = sprite_make_wnd(mouse_width, mouse_height, 0x0F);
	mouse_medium_sprite = sprite_make_wnd(mouse_width, mouse_height, 0x0F);
	mouse_background_sprite = sprite_make_wnd(
		mouse_width + video_flag2_is1, mouse_height, 0x0F);

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
	case 0x1B:
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

	case 0x3B00:
		cameramode = 0;
		return 1;
	case 0x3C00:
		cameramode = 1;
		return 1;
	case 0x3D00:
		cameramode = 2;
		return 1;
	case 0x3E00:
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

legacy_s16 mouse_multi_hittest(legacy_s16 count, legacy_s16* x1_array, legacy_s16* x2_array,
	legacy_s16* y1_array, legacy_s16* y2_array)
{
	legacy_s16 i;

	if (kbormouse == 0)
		return -1;

	for (i = 0; i < count; i++) {
		if (x1_array[i] <= mouse_xpos && mouse_xpos <= x2_array[i] &&
			y1_array[i] <= mouse_ypos && mouse_ypos <= y2_array[i])
			return (legacy_s8)i;
	}

	return -1;
}

legacy_s16 get_kb_or_joy_flags(void)
{
	static const legacy_u8 action_flags[10] = {
		0x10, 0x20, 0x09, 0x01, 0x05,
		0x04, 0x06, 0x02, 0x0A, 0x08
	};
	legacy_u16 flags;
	legacy_u16 index;

	flags = 0;
	for (index = 0; index < 10U; index++) {
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
	if (LEGACY_S16_FROM_BITS(input_elapsed_frames) > 20000) {
		input_elapsed_frames = LEGACY_U16_WRAP_SUB(input_elapsed_frames, 10000U);
		input_mouse_repeat_at = LEGACY_U16_WRAP_SUB(input_mouse_repeat_at, 10000U);
		input_joystick_repeat_at = LEGACY_U16_WRAP_SUB(input_joystick_repeat_at, 10000U);
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
		LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_ADD(
			input_joystick_repeat_at, 20U)) <
		LEGACY_S16_FROM_BITS(input_elapsed_frames)) {
		changed_or_repeating = 1;
	}

	if (changed_or_repeating) {
		if (((legacy_u16)input_new_joystick_flags & 0x20U) != 0)
			input_joystick_keycode = 0x0D;
		else if (((legacy_u16)input_new_joystick_flags & 0x10U) != 0)
			input_joystick_keycode = 0x20;
		else if (((legacy_u16)input_new_joystick_flags & 1U) != 0)
			input_joystick_keycode = 0x4800;
		else if (((legacy_u16)input_new_joystick_flags & 2U) != 0)
			input_joystick_keycode = 0x5000;
		else if (((legacy_u16)input_new_joystick_flags & 8U) != 0)
			input_joystick_keycode = 0x4B00;
		else if (((legacy_u16)input_new_joystick_flags & 4U) != 0)
			input_joystick_keycode = 0x4D00;

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
		if (LEGACY_S16_FROM_BITS(input_mouse_idle_frames) > 500) {
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
			LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_ADD(
				input_mouse_repeat_at, 20U)) <
			LEGACY_S16_FROM_BITS(input_elapsed_frames)) {
			changed_or_repeating = 1;
		}

		if (changed_or_repeating) {
			if (((legacy_u16)mouse_butstate & 1U) != 0)
				input_mouse_keycode = 0x20;
			else if (((legacy_u16)mouse_butstate & 2U) != 0)
				input_mouse_keycode = 0x0D;
			if (input_mouse_keycode != 0)
				input_mouse_repeat_at = input_elapsed_frames;
			input_mouse_idle_frames = 0;
		}

		if (mouse_butstate != 0) {
			if (((legacy_u16)mouse_butstate & 1U) != 0)
				input_combined_flags = (legacy_u16)input_combined_flags | 0x20U;
			else if (((legacy_u16)mouse_butstate & 2U) != 0)
				input_combined_flags = (legacy_u16)input_combined_flags | 0x10U;
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
	thumb_start = mouse_track_position(length, (legacy_s16)selected,
		(legacy_s16)item_count);
	thumb_end = mouse_track_position(length,
		LEGACY_S16_WRAP_ADD(selected, selection_width),
		(legacy_s16)item_count);
	thumb_size = LEGACY_S16_WRAP_SUB(thumb_end, thumb_start);

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
		} while (((legacy_u16)mouse_butstate & 3U) != 0);
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
		} while (((legacy_u16)mouse_butstate & 3U) != 0);
	}

	if (selected == -1) {
		quotient = mouse_track_divide(length, (legacy_s16)item_count);
		quotient = LEGACY_S16_SAR(quotient, 1U);
		scaled = LEGACY_S16_WRAP_MUL(
			LEGACY_S16_WRAP_ADD(dragged_start, quotient), item_count);
		selected = mouse_track_divide(scaled, length);
	}

	thumb_start = mouse_track_position(length, (legacy_s16)selected,
		(legacy_s16)item_count);
	thumb_end = mouse_track_position(length,
		LEGACY_S16_WRAP_ADD(selected, selection_width),
		(legacy_s16)item_count);
	thumb_size = LEGACY_S16_WRAP_SUB(thumb_end, thumb_start);
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
		pressed = (get_kb_or_joy_flags() & 0x30) != 0;
		if (!pressed) {
			pressed = input_checking(
				(legacy_s16)timer_get_delta_alt()) != 0;
		}
		if (!pressed && kbormouse != 0 && (mouse_butstate & 3) != 0)
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
		dos_mouse_set_minmax(0x0F, 0, 0x131, 0xC8);
		dos_mouse_set_position(0xA0, 0x64);
	} else {
		dos_mouse_set_minmax(0, 0, 0x140, 0xC8);
	}
}
