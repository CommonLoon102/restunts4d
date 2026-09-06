#include "fileio.h"
#include "legacy.h"
#include "memmgr.h"
#include "menu_internal.h"
#include "platform.h"
#include "shape2d.h"
#include "shape3d.h"

#define MENU_ANIMATION_PERIOD 60
#define MENU_ANIMATION_SECOND_STATE_START 30
#define BUTTON_TEXT_LINE_BUFFER_SIZE 86U
#define BUTTON_FONT_LINE_HEIGHT 8U
#define BUTTON_CENTER_DIVISOR 2
#define BUTTON_TEXT_VERTICAL_ADJUSTMENT 1
#define MENU_ANIMATION_INITIAL_COUNTER 0
#define MENU_ANIMATION_INITIAL_STATE 0
#define MENU_IDLE_INITIAL_COUNTER 0
#define MENU_IDLE_EXPIRED_INCREMENT 1U
#define BUTTON_TEXT_NONE 0
#define BUTTON_FONT_SECONDARY_COLOR 0
#define BUTTON_INITIAL_LINE_COUNT 1U
#define BUTTON_TEXT_FIRST_INDEX 0U
#define BUTTON_TEXT_LINE_SEPARATOR ']'
#define BUTTON_TEXT_TERMINATOR '\0'

static legacy_s16 menu_animation_counter;
static legacy_s16 menu_animation_state;
legacy_s16 menu_idle_counter;

void sub_29772(void)
{
	menu_animation_counter = MENU_ANIMATION_INITIAL_COUNTER;
	menu_animation_state = MENU_ANIMATION_INITIAL_STATE;
	menu_idle_counter = MENU_IDLE_INITIAL_COUNTER;
}

void menu_update_idle_counter(legacy_u16 elapsed, legacy_s16 limit)
{
	menu_idle_counter = LEGACY_U16_WRAP_ADD(menu_idle_counter, elapsed);
	if (LEGACY_S16_FROM_BITS((legacy_u16)menu_idle_counter) > limit) {
		menu_idle_counter = MENU_IDLE_INITIAL_COUNTER;
		idle_expired = (legacy_u8)(idle_expired +
			MENU_IDLE_EXPIRED_INCREMENT);
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
	while (LEGACY_S16_FROM_BITS(animation_counter) > MENU_ANIMATION_PERIOD)
		animation_counter = LEGACY_U16_WRAP_SUB(
			animation_counter, MENU_ANIMATION_PERIOD);
	menu_animation_counter = animation_counter;
	selected_state = LEGACY_S16_FROM_BITS(animation_counter) >
		MENU_ANIMATION_SECOND_STATE_START ?
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
	legacy_s8 line[BUTTON_TEXT_LINE_BUFFER_SIZE];
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

	if (text == BUTTON_TEXT_NONE)
		return;

	font_set_unk(font_color, BUTTON_FONT_SECONDARY_COLOR);
	copied_text = &resID_byte1;
	copy_string(copied_text, text);
	length = (legacy_u16)strlen(copied_text);
	line_count = BUTTON_INITIAL_LINE_COUNT;
	for (source_index = BUTTON_TEXT_FIRST_INDEX;
		source_index < length; source_index++) {
		if (copied_text[source_index] == BUTTON_TEXT_LINE_SEPARATOR)
			line_count++;
	}

	remaining = LEGACY_S16_WRAP_SUB(height,
		LEGACY_U16_WRAP_MUL(line_count, BUTTON_FONT_LINE_HEIGHT));
	vertical_offset = LEGACY_S16_WRAP_ADD(
		LEGACY_S16_DIV_OR_ZERO(remaining, BUTTON_CENTER_DIVISOR),
		BUTTON_TEXT_VERTICAL_ADJUSTMENT);
	destination_index = BUTTON_TEXT_FIRST_INDEX;
	line_index = BUTTON_TEXT_FIRST_INDEX;
	for (source_index = BUTTON_TEXT_FIRST_INDEX;
		source_index <= length; source_index++) {
		legacy_s8 character = copied_text[source_index];

		if (character != BUTTON_TEXT_LINE_SEPARATOR &&
			character != BUTTON_TEXT_TERMINATOR) {
			line[destination_index++] = character;
			continue;
		}

		line[destination_index] = BUTTON_TEXT_TERMINATOR;
		remaining = LEGACY_S16_WRAP_SUB(width, font_op2(line));
		horizontal_offset = LEGACY_S16_DIV_OR_ZERO(
			remaining, BUTTON_CENTER_DIVISOR);
		font_draw_text(line,
			LEGACY_S16_WRAP_ADD(x, horizontal_offset),
			LEGACY_S16_WRAP_ADD(
				LEGACY_S16_WRAP_ADD(y, vertical_offset),
				LEGACY_U16_WRAP_MUL(line_index, BUTTON_FONT_LINE_HEIGHT)));
		line_index++;
		destination_index = BUTTON_TEXT_FIRST_INDEX;
	}
}
