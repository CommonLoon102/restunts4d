#include "frame_internal.h"

struct RECTANGLE* do_sinking(legacy_s16 frame, legacy_s16 top, legacy_s16 height)
{
	legacy_s16 duration;
	legacy_s16 clipped_frame;
	legacy_s16 sink_height;
	legacy_s16 bottom;

	duration = LEGACY_S16_SHL(framespersec, 2U);
	clipped_frame = (legacy_s16)frame;
	if (clipped_frame > duration)
		clipped_frame = duration;
	sink_height = LEGACY_S16_FROM_BITS((legacy_u16)
		LEGACY_S32_DIV_OR_ZERO(LEGACY_S32_WRAP_MUL(
			(legacy_s32)height, (legacy_s32)clipped_frame),
			(legacy_s32)duration));
	bottom = LEGACY_S16_WRAP_ADD(top, height);
	rect_ingame_text.left = 0;
	rect_ingame_text.right = 0x140;
	rect_ingame_text.top = LEGACY_S16_WRAP_SUB(bottom, sink_height);
	rect_ingame_text.bottom = bottom;
	sprite_set_1_size(0, 0x140, rect_ingame_text.top,
		rect_ingame_text.bottom);
	sprite_clear_1_color((legacy_u8)skybox.water_color);
	return &rect_ingame_text;
}

struct RECTANGLE* init_crak(legacy_s16 frame, legacy_s16 top, legacy_s16 height)
{
	legacy_u8 far* crack_lines;
	legacy_u8 far* crack_info;
	legacy_s16 frame_count;
	legacy_s16 frame_index;
	legacy_s16 line_count;
	legacy_s16 start_x;
	legacy_s16 start_y;
	legacy_s16 end_x;
	legacy_s16 end_y;
	legacy_s16 scaled_start_y;
	legacy_s16 scaled_end_y;
	legacy_s16 frame_divisor;
	legacy_s32 scaled_coordinate;
	struct POINT2D point;
	legacy_s16 i;

	crack_lines = (legacy_u8 far*)locate_shape_alt(gameresptr, "crak");
	crack_info = (legacy_u8 far*)locate_shape_alt(gameresptr, "cinf");
	frame_divisor = LEGACY_S16_FROM_BITS(
		LEGACY_U16_DIV_OR_ZERO(framespersec, 7U));
	frame_index = LEGACY_S16_DIV_OR_ZERO(frame, frame_divisor);
	frame_count = LEGACY_READ_S16_LE(crack_info);
	if (frame_index >= frame_count)
		frame_index = LEGACY_S16_WRAP_SUB(frame_count, 1);
	line_count = LEGACY_READ_S16_LE(crack_info +
		((legacy_u16)LEGACY_S16_WRAP_ADD(frame_index, 1) << 1));
	rect_ingame_text = cliprect_unk;

	for (i = 0; i < line_count; i++) {
		legacy_u16 line_offset = (legacy_u16)i << 3;

		start_x = LEGACY_READ_S16_LE(crack_lines + line_offset);
		start_y = LEGACY_READ_S16_LE(crack_lines + line_offset + 2U);
		end_x = LEGACY_READ_S16_LE(crack_lines + line_offset + 4U);
		end_y = LEGACY_READ_S16_LE(crack_lines + line_offset + 6U);
		scaled_coordinate = LEGACY_S32_WRAP_MUL(
			(legacy_s32)start_y, (legacy_s32)height);
		scaled_start_y = LEGACY_S16_FROM_BITS((legacy_u16)
			LEGACY_S32_DIV_OR_ZERO(scaled_coordinate, 200L));
		scaled_coordinate = LEGACY_S32_WRAP_MUL(
			(legacy_s32)end_y, (legacy_s32)height);
		scaled_end_y = LEGACY_S16_FROM_BITS((legacy_u16)
			LEGACY_S32_DIV_OR_ZERO(scaled_coordinate, 200L));

		preRender_line(start_x,
			LEGACY_S16_WRAP_SUB(
				LEGACY_S16_WRAP_ADD(scaled_start_y, top), 1),
			end_x,
			LEGACY_S16_WRAP_SUB(
				LEGACY_S16_WRAP_ADD(scaled_end_y, top), 1),
			0);
		preRender_line(start_x,
			LEGACY_S16_WRAP_ADD(
				LEGACY_S16_WRAP_ADD(scaled_start_y, top), 1),
			end_x,
			LEGACY_S16_WRAP_ADD(
				LEGACY_S16_WRAP_ADD(scaled_end_y, top), 1),
			0);
		preRender_line(start_x,
			LEGACY_S16_WRAP_ADD(scaled_start_y, top),
			end_x,
			LEGACY_S16_WRAP_ADD(scaled_end_y, top),
			dialog_fnt_colour);

		if (slow_video_mgmt_copy != 0) {
			point.px = start_x;
			point.py = LEGACY_S16_WRAP_SUB(
				LEGACY_S16_WRAP_ADD(scaled_start_y, top), 1);
			rect_adjust_from_point(&point, &rect_ingame_text);
			point.px = end_x;
			point.py = LEGACY_S16_WRAP_ADD(
				LEGACY_S16_WRAP_ADD(scaled_end_y, top), 1);
			rect_adjust_from_point(&point, &rect_ingame_text);
			point.px = start_x;
			point.py = LEGACY_S16_WRAP_ADD(
				LEGACY_S16_WRAP_ADD(scaled_start_y, top), 1);
			rect_adjust_from_point(&point, &rect_ingame_text);
			point.px = end_x;
			point.py = LEGACY_S16_WRAP_SUB(
				LEGACY_S16_WRAP_ADD(scaled_end_y, top), 1);
			rect_adjust_from_point(&point, &rect_ingame_text);
		}
	}

	return &rect_ingame_text;
}

static void draw_centered_ingame_resource(legacy_s8* resource_id, legacy_s16 y)
{
	copy_string(&resID_byte1, locate_text_res(gameresptr, resource_id));
	rect_union(&rect_ingame_text,
		intro_draw_text(&resID_byte1, font_op2_alt(&resID_byte1), y,
			dialog_fnt_colour, 0),
		&rect_ingame_text);
}

struct RECTANGLE* draw_ingame_text(void)
{
	legacy_u16 replay_frame;
	legacy_s16 replay_x;

	rect_ingame_text = cliprect_unk;
	if (idle_expired != 0) {
		draw_centered_ingame_resource("dm1", 0xAA);
		draw_centered_ingame_resource("dm2", 0xB6);
		return &rect_ingame_text;
	}

	if (game_replay_mode != 0) {
		if (game_replay_mode != 2)
			return &rect_ingame_text;
		replay_frame = (legacy_u16)state.game_frame % framespersec;
		if (replay_frame >= (legacy_u16)LEGACY_S16_SAR(framespersec, 1U))
			return &rect_ingame_text;
		copy_string(&resID_byte1, locate_text_res(gameresptr, "rpl"));
		replay_x = LEGACY_S16_WRAP_SUB(0x138,
			LEGACY_U16_WRAP_MUL(strlen(&resID_byte1), 8U));
		rect_union(&rect_ingame_text,
			intro_draw_text(&resID_byte1, replay_x, 0x0F,
				dialog_fnt_colour, 0),
			&rect_ingame_text);
		return &rect_ingame_text;
	}

	if (state.game_inputmode == 0) {
		draw_centered_ingame_resource("pre", 0x5A);
		return &rect_ingame_text;
	}
	if (passed_security == 0) {
		draw_centered_ingame_resource("se1", 0x5D);
		draw_centered_ingame_resource("se2", 0x69);
		return &rect_ingame_text;
	}
	if (followOpponentFlag != 0 || cameramode != 0 ||
		state.playerstate.car_crashBmpFlag != 0)
		return &rect_ingame_text;

	switch (state.field_45D) {
	case 1:
		sprite_putimage_transparent(sdgame2shapes[3], 0x94, 0x5D);
		rect_union(&rect_ingame_text, &rect_ingame_text2,
			&rect_ingame_text);
		break;
	case 2:
		sprite_putimage_transparent(sdgame2shapes[4], 0x94, 0x5D);
		rect_union(&rect_ingame_text, &rect_ingame_text2,
			&rect_ingame_text);
		break;
	case 3:
		draw_centered_ingame_resource("www", 0x5D);
		break;
	}

	resID_byte1 = 0;
	switch (state.field_45E) {
	case 1:
		sprite_putimage_transparent(sdgame2shapes[3], 0x44, 0x71);
		rect_union(&rect_ingame_text, &rect_ingame_text3,
			&rect_ingame_text);
		copy_string(&resID_byte1,
			locate_text_res(gameresptr, "opp"));
		break;
	case 2:
		sprite_putimage_transparent(sdgame2shapes[4], 0xE4, 0x71);
		rect_union(&rect_ingame_text, &rect_ingame_text4,
			&rect_ingame_text);
		copy_string(&resID_byte1,
			locate_text_res(gameresptr, "opp"));
		break;
	}
	if (resID_byte1 != 0)
		rect_union(&rect_ingame_text,
			intro_draw_text(&resID_byte1, font_op2_alt(&resID_byte1),
				0x74, dialog_fnt_colour, 0),
			&rect_ingame_text);

	if (show_penalty_counter != 0) {
		copy_string(&resID_byte1, locate_text_res(gameresptr, "pen"));
		format_frame_as_string(&resID_byte1 + strlen(&resID_byte1),
			penalty_time, 0);
		rect_union(&rect_ingame_text,
			intro_draw_text(&resID_byte1, font_op2_alt(&resID_byte1),
				0x66, dialog_fnt_colour, 0),
			&rect_ingame_text);
	}

	return &rect_ingame_text;
}
