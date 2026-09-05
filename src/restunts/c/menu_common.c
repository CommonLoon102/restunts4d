#include "fileio.h"
#include "legacy.h"
#include "memmgr.h"
#include "menu_internal.h"
#include "platform.h"
#include "shape2d.h"
#include "shape3d.h"

static legacy_s16 menu_animation_counter;
static legacy_s16 menu_animation_state;
legacy_s16 menu_idle_counter;

void sub_29772(void)
{
	menu_animation_counter = 0;
	menu_animation_state = 0;
	menu_idle_counter = 0;
}

void menu_update_idle_counter(legacy_u16 elapsed, legacy_s16 limit)
{
	menu_idle_counter = LEGACY_U16_WRAP_ADD(menu_idle_counter, elapsed);
	if (LEGACY_S16_FROM_BITS((legacy_u16)menu_idle_counter) > limit) {
		menu_idle_counter = 0;
		idle_expired = (legacy_u8)(idle_expired + 1U);
	}
}

legacy_s16 mouse_timer_sprite_unk(legacy_s16 item_index,
	const struct BUTTON_AREA* buttons,
	legacy_s16 second_state, legacy_s16 first_state)
{
	legacy_u16 delta;
	legacy_u16 animation_counter;
	legacy_s16 selected_state;

	delta = (legacy_u16)timer_get_delta_alt();
	animation_counter = LEGACY_U16_WRAP_ADD(menu_animation_counter, delta);
	while (LEGACY_S16_FROM_BITS(animation_counter) > 60)
		animation_counter = LEGACY_U16_WRAP_SUB(animation_counter, 60U);
	menu_animation_counter = animation_counter;
	selected_state = LEGACY_S16_FROM_BITS(animation_counter) > 30 ?
		LEGACY_S16_FROM_BITS((legacy_u16)second_state) :
		LEGACY_S16_FROM_BITS((legacy_u16)first_state);
	if (menu_animation_state != selected_state) {
		menu_animation_state = selected_state;
		mouse_draw_opaque_check();
		sprite_1_unk4(buttons[item_index].x1, buttons[item_index].y1,
			buttons[item_index].x2, buttons[item_index].y2,
			selected_state);
		mouse_draw_transparent_check();
	}
	return LEGACY_S16_FROM_BITS(delta);
}

void draw_button(legacy_s8 far* text, legacy_s16 x, legacy_s16 y, legacy_s16 width, legacy_s16 height,
	legacy_s16 top_color, legacy_s16 bottom_color, legacy_s16 fill_color, legacy_s16 font_color)
{
	legacy_s8 line[86];
	legacy_s8* copied_text;
	legacy_u16 length;
	legacy_u16 source_index;
	legacy_u16 destination_index;
	legacy_u16 line_index;
	legacy_u16 line_count;
	legacy_s16 vertical_offset;
	legacy_s16 horizontal_offset;
	legacy_s16 remaining;

	sprite_1_unk(x, y, width, height, fill_color);
	draw_beveled_border(x, y, width, height,
		top_color, top_color, bottom_color, bottom_color);

	if (text == 0)
		return;

	font_set_unk(font_color, 0);
	copied_text = &resID_byte1;
	copy_string(copied_text, text);
	length = (legacy_u16)strlen(copied_text);
	line_count = 1;
	for (source_index = 0; source_index < length; source_index++) {
		if (copied_text[source_index] == ']')
			line_count++;
	}

	remaining = LEGACY_S16_WRAP_SUB(height,
		LEGACY_U16_WRAP_MUL(line_count, 8U));
	vertical_offset = LEGACY_S16_WRAP_ADD(
		LEGACY_S16_DIV_OR_ZERO(remaining, 2), 1);
	destination_index = 0;
	line_index = 0;
	for (source_index = 0; source_index <= length; source_index++) {
		legacy_s8 character = copied_text[source_index];

		if (character != ']' && character != 0) {
			line[destination_index++] = character;
			continue;
		}

		line[destination_index] = 0;
		remaining = LEGACY_S16_WRAP_SUB(width, font_op2(line));
		horizontal_offset = LEGACY_S16_DIV_OR_ZERO(remaining, 2);
		font_draw_text(line,
			LEGACY_S16_WRAP_ADD(x, horizontal_offset),
			LEGACY_S16_WRAP_ADD(
				LEGACY_S16_WRAP_ADD(y, vertical_offset),
				LEGACY_U16_WRAP_MUL(line_index, 8U)));
		line_index++;
		destination_index = 0;
	}
}
