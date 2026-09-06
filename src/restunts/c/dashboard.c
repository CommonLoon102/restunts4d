#include "dashboard.h"
#include "fileio.h"
#include "memmgr.h"
#include "menu_internal.h"
#include "platform.h"
#include "shape2d.h"

#define DASHBOARD_STEERING_SCALE_SHIFT 3U
#define DASHBOARD_VIEWPORT_WIDTH 320
#define DASHBOARD_SPRITE_WINDOW_LEGACY_ARGUMENT 15U
#define DASHBOARD_ANALOG_SPEED_DIVISOR 640U
#define DASHBOARD_CAR_ID_LENGTH 4U
#define DASHBOARD_CAR_ID_RESOURCE_OFFSET 4U

#define DASHBOARD_WHEEL_LEFT_SHAPE 0U
#define DASHBOARD_WHEEL_CENTER_SHAPE 1U
#define DASHBOARD_WHEEL_RIGHT_SHAPE 2U
#define DASHBOARD_INSTRUMENT_PANEL_SHAPE 3U
#define DASHBOARD_GEARBOX_SHAPE 4U
#define DASHBOARD_LEFT_INSTRUMENT_SOURCE_SHAPE 5U
#define DASHBOARD_RIGHT_INSTRUMENT_SOURCE_SHAPE 6U
#define DASHBOARD_LEFT_INSTRUMENT_MASK_SHAPE 7U
#define DASHBOARD_RIGHT_INSTRUMENT_MASK_SHAPE 8U

#define DASHBOARD_GEAR_KNOB_SHAPE 0U
#define DASHBOARD_GEAR_KNOB_BACKGROUND_SHAPE 1U
#define DASHBOARD_STEERING_DOT_SHAPE 2U
#define DASHBOARD_STEERING_DOT_BACKGROUND_SHAPE 3U
#define DASHBOARD_STEERING_DOT_BUFFER_FIRST_SHAPE 4U

#define DASHBOARD_CACHE_VALUE_INVALID (-1)
#define DASHBOARD_STEERING_LEFT_LIMIT (-10)
#define DASHBOARD_STEERING_RIGHT_LIMIT 10
#define DASHBOARD_WHEEL_STATE_LEFT DASHBOARD_WHEEL_LEFT_SHAPE
#define DASHBOARD_WHEEL_STATE_CENTER DASHBOARD_WHEEL_CENTER_SHAPE
#define DASHBOARD_WHEEL_STATE_RIGHT DASHBOARD_WHEEL_RIGHT_SHAPE

#define DASHBOARD_GAUGE_ANALOG 0U
#define DASHBOARD_GAUGE_DIGITAL 1U
#define DASHBOARD_GAUGE_HIDDEN 2U
#define DASHBOARD_GAUGE_HIDDEN_CENTER_Y (-1)
#define DASHBOARD_GAUGE_DIGITAL_CENTER_Y 0
#define DASHBOARD_DIGITAL_SPEED_SHIFT 8U
#define DASHBOARD_RPM_INDEX_SHIFT 7U
#define DASHBOARD_POINT_COORDINATE_STRIDE 2U
#define DASHBOARD_POINT_Y_OFFSET 1U
#define DASHBOARD_STEERING_MIRROR_SHIFT 1U

#define DASHBOARD_DECIMAL_BASE 10U
#define DASHBOARD_ONE_HUNDRED 100U
#define DASHBOARD_TWO_HUNDRED 200U
#define DASHBOARD_ONE_HUNDRED_DIGIT 1U
#define DASHBOARD_TWO_HUNDRED_DIGIT 2U
#define DASHBOARD_SPEED_HUNDREDS_X 0U
#define DASHBOARD_SPEED_HUNDREDS_Y 1U
#define DASHBOARD_SPEED_TENS_X 2U
#define DASHBOARD_SPEED_TENS_Y 3U
#define DASHBOARD_SPEED_UNITS_X 4U
#define DASHBOARD_SPEED_UNITS_Y 5U

static legacy_s16 dashboard_steering_position(legacy_s16 angle)
{
	legacy_s16 magnitude;
	legacy_u16 bits;

	magnitude = angle < 0 ? LEGACY_S16_WRAP_NEGATE(angle) : angle;
	bits = (legacy_u16)magnitude;
	bits = LEGACY_U16_SAR(bits, DASHBOARD_STEERING_SCALE_SHIFT);
	magnitude = LEGACY_S16_FROM_BITS(bits);
	return angle < 0 ? LEGACY_S16_WRAP_NEGATE(magnitude) : magnitude;
}

static legacy_u8 dashboard_clear_steering_dot(legacy_u16 player_index)
{
	if (word_40DF6[player_index] == 0)
		return 0;
	sprite_putimage_and_alt(
		gnobshapes[DASHBOARD_STEERING_DOT_BUFFER_FIRST_SHAPE +
			(legacy_u8)byte_44346],
		word_40DF2[player_index], word_40DF6[player_index]);
	word_40DF6[player_index] = 0;
	return 1;
}

static void dashboard_set_viewport(void)
{
	sprite_set_1_size(0, DASHBOARD_VIEWPORT_WIDTH, 0,
		height_above_replaybar);
}

void setup_car_shapes(legacy_s16 operation)
{
	struct SHAPE2D far* shape;
	struct SHAPE2D far* dashboard_shape;
	struct SHAPE2D far* gearbox_shape;
	legacy_u8* steering_dots;
	legacy_u16 player_index;
	legacy_u16 speed_index;
	legacy_u16 rpm_index;
	legacy_u16 digit;
	legacy_u16 digit_group;
	legacy_u16 dot_index;
	legacy_s16 steering_position;
	legacy_s16 dot_x;
	legacy_s16 dot_y;
	legacy_u8 wheel_state;
	legacy_u8 wheel_redrawn;
	legacy_u8 steering_dot_cleared;
	legacy_u8 gauge_mode;
	legacy_u8 digit_started;
	legacy_u16 index;

	if (operation == DASHBOARD_OPERATION_LOAD) {
		for (index = 0; index < DASHBOARD_CAR_ID_LENGTH; index++) {
			aStdaxxxx[index + DASHBOARD_CAR_ID_RESOURCE_OFFSET] =
				gameconfig.game_playercarid[index];
			aStdbxxxx[index + DASHBOARD_CAR_ID_RESOURCE_OFFSET] =
				gameconfig.game_playercarid[index];
		}
		stdaresptr = (legacy_s8 far*)file_load_resource(
			FILE_RESOURCE_SHAPE2D_COLLECTION, aStdaxxxx);
		stdbresptr = (legacy_s8 far*)file_load_resource(
			FILE_RESOURCE_SHAPE2D, aStdbxxxx);
		locate_many_resources(stdaresptr, aWhl1whl2whl3ins2gboxins1i,
			(legacy_s8 far**)whlshapes);
		locate_many_resources(stdbresptr, aGnobgnabdotDotadot1dot2,
			(legacy_s8 far**)gnobshapes);
		if (simd_player.spdcenter.py == 0) {
			locate_many_resources(stdbresptr,
				aDig0dig1dig2dig3dig4dig5d, (legacy_s8 far**)digshapes);
		}

		whlsprite1 = sprite_make_wnd(
			LEGACY_U16_WRAP_MUL(shape2d_get_width(
				whlshapes[DASHBOARD_INSTRUMENT_PANEL_SHAPE]),
				(legacy_u16)video_flag1_is1),
			shape2d_get_height(
				whlshapes[DASHBOARD_INSTRUMENT_PANEL_SHAPE]),
			DASHBOARD_SPRITE_WINDOW_LEGACY_ARGUMENT);
		whlsprite2 = sprite_make_wnd(
			LEGACY_U16_WRAP_MUL(shape2d_get_width(
				whlshapes[DASHBOARD_GEARBOX_SHAPE]),
				(legacy_u16)video_flag1_is1),
			shape2d_get_height(whlshapes[DASHBOARD_GEARBOX_SHAPE]),
			DASHBOARD_SPRITE_WINDOW_LEGACY_ARGUMENT);
		whlsprite3 = sprite_make_wnd(
			LEGACY_U16_WRAP_MUL(shape2d_get_width(
				whlshapes[DASHBOARD_GEARBOX_SHAPE]),
				(legacy_u16)video_flag1_is1),
			shape2d_get_height(whlshapes[DASHBOARD_GEARBOX_SHAPE]),
			DASHBOARD_SPRITE_WINDOW_LEGACY_ARGUMENT);

		dashboard_shape = (struct SHAPE2D far*)
			locate_shape_fatal(stdaresptr, aDash);
		gearbox_shape = whlshapes[DASHBOARD_GEARBOX_SHAPE];
		sprite_set_1_from_argptr(whlsprite3);
		shape2d_op_unk2(dashboard_shape,
			LEGACY_S16_WRAP_SUB(
				(legacy_s16)shape2d_get_pos_x(dashboard_shape),
				(legacy_s16)shape2d_get_pos_x(gearbox_shape)),
			LEGACY_S16_WRAP_SUB(
				(legacy_s16)shape2d_get_pos_y(dashboard_shape),
				(legacy_s16)shape2d_get_pos_y(gearbox_shape)));
		sprite_copy_2_to_1();
		dashbmp_y = shape2d_get_pos_y(dashboard_shape);

		shape = (struct SHAPE2D far*)locate_shape_nofatal(stdaresptr, aRoof);
		if (shape != 0) {
			shape = (struct SHAPE2D far*)locate_shape_fatal(stdaresptr, aRoof);
			roofbmpheight = shape2d_get_height(shape);
		} else {
			roofbmpheight = 0;
		}

		shape = (struct SHAPE2D far*)locate_shape_nofatal(stdaresptr, aDast);
		if (shape != 0) {
			dastbmp_y = shape2d_get_pos_y(shape);
			dastbmp_y2 = dos_memory_pointer_offset(shape);
			dastseg = dos_memory_pointer_segment(shape);
			dasmshapeptr = locate_shape_fatal(stdaresptr, aDasm);
		} else {
			dastbmp_y = 0;
		}
		return;
	}

	if (operation == DASHBOARD_OPERATION_REDRAW_STATIC) {
		mouse_draw_opaque_check();
		shape = (struct SHAPE2D far*)locate_shape_nofatal(stdaresptr, aRoof);
		if (shape != 0)
			shape2d_op_unk((struct SHAPE2D far*)
				locate_shape_fatal(stdaresptr, aRoof));
		shape2d_op_unk3((struct SHAPE2D far*)
			locate_shape_fatal(stdaresptr, aDash));
		shape2d_op_unk3(whlshapes[DASHBOARD_WHEEL_CENTER_SHAPE]);
		mouse_draw_transparent_check();

		player_index = (legacy_u8)byte_4432A;
		byte_449D8[player_index] = 0;
		byte_40DFA[player_index] = 0;
		word_40DF6[player_index] = 0;
		byte_40DF0[player_index] = 0;
		word_40E00[player_index] = DASHBOARD_CACHE_VALUE_INVALID;
		word_40D78[player_index] = DASHBOARD_CACHE_VALUE_INVALID;
		word_40D6C[player_index] = DASHBOARD_CACHE_VALUE_INVALID;
		return;
	}

	if (operation == DASHBOARD_OPERATION_UNLOAD) {
		sprite_free_wnd(whlsprite3);
		sprite_free_wnd(whlsprite2);
		sprite_free_wnd(whlsprite1);
		mmgr_free(stdbresptr);
		mmgr_free(stdaresptr);
		return;
	}
	if (operation != DASHBOARD_OPERATION_UPDATE)
		return;

	player_index = (legacy_u8)byte_4432A;
	steering_dot_cleared = 0;
	if (state.playerstate.car_fpsmul2 == GEAR_CHANGE_DELAY_EXPIRED &&
		state.playerstate.car_changing_gear == CAR_GEAR_CHANGE_INACTIVE &&
		byte_40DFA[player_index] != 0) {
		if (video_flag5_is0 == 0)
			mouse_draw_opaque_check();
		dashboard_set_viewport();
		sprite_putimage_and_alt(whlsprite3->sprite_bitmapptr,
			shape2d_get_pos_x(whlshapes[DASHBOARD_GEARBOX_SHAPE]),
			shape2d_get_pos_y(whlshapes[DASHBOARD_GEARBOX_SHAPE]));
		byte_40DFA[player_index] = 0;
	} else if (byte_40DFA[player_index] !=
			(legacy_u8)state.playerstate.car_changing_gear ||
		word_40D70[player_index] != state.playerstate.car_knob_x ||
		word_40D74[player_index] != state.playerstate.car_knob_y ||
		(state.playerstate.car_fpsmul2 != GEAR_CHANGE_DELAY_EXPIRED &&
			byte_40DFA[player_index] == 0)) {
		sprite_set_1_from_argptr(whlsprite2);
		byte_40DFA[player_index] = 1;
		shape2d_op_unk2(whlshapes[DASHBOARD_GEARBOX_SHAPE], 0, 0);
		word_40D70[player_index] = state.playerstate.car_knob_x;
		word_40D74[player_index] = state.playerstate.car_knob_y;
		sprite_putimage_and_alt2(
			gnobshapes[DASHBOARD_GEAR_KNOB_BACKGROUND_SHAPE],
			state.playerstate.car_knob_x, state.playerstate.car_knob_y);
		sprite_putimage_or_alt(gnobshapes[DASHBOARD_GEAR_KNOB_SHAPE],
			state.playerstate.car_knob_x, state.playerstate.car_knob_y);
		if (video_flag5_is0 != 0) {
			setup_mcgawnd2();
		} else {
			sprite_copy_2_to_1_2();
			mouse_draw_opaque_check();
		}
		dashboard_set_viewport();
		sprite_putimage_and_alt(whlsprite2->sprite_bitmapptr,
			shape2d_get_pos_x(whlshapes[DASHBOARD_GEARBOX_SHAPE]),
			shape2d_get_pos_y(whlshapes[DASHBOARD_GEARBOX_SHAPE]));
	}

	steering_position = dashboard_steering_position(
		state.playerstate.car_steeringAngle);
	wheel_state = DASHBOARD_WHEEL_STATE_CENTER;
	if (steering_position < DASHBOARD_STEERING_LEFT_LIMIT)
		wheel_state = DASHBOARD_WHEEL_STATE_LEFT;
	else if (steering_position > DASHBOARD_STEERING_RIGHT_LIMIT)
		wheel_state = DASHBOARD_WHEEL_STATE_RIGHT;
	if (byte_40DF0[player_index] != wheel_state || byte_454A4 != 0) {
		if (video_flag5_is0 == 0)
			mouse_draw_opaque_check();
		steering_dot_cleared = dashboard_clear_steering_dot(player_index);
		shape2d_op_unk3(whlshapes[wheel_state]);
		byte_40DF0[player_index] = wheel_state;
		wheel_redrawn = 1;
	} else {
		wheel_redrawn = 0;
	}

	if (simd_player.spdcenter.py == DASHBOARD_GAUGE_HIDDEN_CENTER_Y) {
		speed_index = 0;
		gauge_mode = DASHBOARD_GAUGE_HIDDEN;
	} else if (simd_player.spdcenter.py == DASHBOARD_GAUGE_DIGITAL_CENTER_Y) {
		speed_index = (legacy_u16)state.playerstate.car_speed >>
			DASHBOARD_DIGITAL_SPEED_SHIFT;
		gauge_mode = DASHBOARD_GAUGE_DIGITAL;
	} else {
		speed_index = LEGACY_U16_DIV_OR_ZERO(
			state.playerstate.car_speed, DASHBOARD_ANALOG_SPEED_DIVISOR);
		if ((legacy_s16)speed_index >= simd_player.spdnumpoints)
			speed_index = (legacy_u16)(simd_player.spdnumpoints - 1);
		gauge_mode = DASHBOARD_GAUGE_ANALOG;
	}
	rpm_index = (legacy_u16)state.playerstate.car_currpm >>
		DASHBOARD_RPM_INDEX_SHIFT;
	if ((legacy_s16)rpm_index >= simd_player.revnumpoints)
		rpm_index = (legacy_u16)(simd_player.revnumpoints - 1);

	if (wheel_redrawn != 0 || byte_454A4 != 0 ||
		word_40D78[player_index] != (legacy_s16)speed_index ||
		word_40D6C[player_index] != (legacy_s16)rpm_index) {
		if (video_flag5_is0 == 0)
			mouse_draw_opaque_check();
		if (dashboard_clear_steering_dot(player_index) != 0)
			steering_dot_cleared = 1;
		sprite_set_1_from_argptr(whlsprite1);
		shape2d_op_unk5(
			whlshapes[DASHBOARD_INSTRUMENT_PANEL_SHAPE], 0, 0);
		word_40D78[player_index] = (legacy_s16)speed_index;
		word_40D6C[player_index] = (legacy_s16)rpm_index;

		if (gauge_mode == DASHBOARD_GAUGE_DIGITAL) {
			digit_started = 0;
			digit_group = 0;
			if (speed_index >= DASHBOARD_TWO_HUNDRED) {
				digit_group = DASHBOARD_TWO_HUNDRED_DIGIT;
				speed_index -= DASHBOARD_TWO_HUNDRED;
			} else if (speed_index >= DASHBOARD_ONE_HUNDRED) {
				digit_group = DASHBOARD_ONE_HUNDRED_DIGIT;
				speed_index -= DASHBOARD_ONE_HUNDRED;
			}
			if (digit_group != 0) {
				sprite_putimage_or(digshapes[digit_group],
					(legacy_u8)simd_player.spdpoints[
						DASHBOARD_SPEED_HUNDREDS_X],
					(legacy_u8)simd_player.spdpoints[
						DASHBOARD_SPEED_HUNDREDS_Y]);
				digit_started = 1;
			}
			digit = LEGACY_U16_DIV_OR_ZERO(
				speed_index, DASHBOARD_DECIMAL_BASE);
			if (digit != 0 || digit_started != 0) {
				sprite_putimage_or(digshapes[digit],
					(legacy_u8)simd_player.spdpoints[DASHBOARD_SPEED_TENS_X],
					(legacy_u8)simd_player.spdpoints[DASHBOARD_SPEED_TENS_Y]);
				speed_index -= digit * DASHBOARD_DECIMAL_BASE;
			}
			sprite_putimage_or(digshapes[speed_index],
				(legacy_u8)simd_player.spdpoints[DASHBOARD_SPEED_UNITS_X],
				(legacy_u8)simd_player.spdpoints[DASHBOARD_SPEED_UNITS_Y]);
		} else if (gauge_mode == DASHBOARD_GAUGE_ANALOG) {
			dot_index = speed_index * DASHBOARD_POINT_COORDINATE_STRIDE;
			preRender_line(simd_player.spdcenter.px,
				simd_player.spdcenter.py,
				(legacy_u8)simd_player.spdpoints[dot_index],
				(legacy_u8)simd_player.spdpoints[
					dot_index + DASHBOARD_POINT_Y_OFFSET],
				meter_needle_color);
		}

		dot_index = rpm_index * DASHBOARD_POINT_COORDINATE_STRIDE;
		preRender_line(simd_player.revcenter.px, simd_player.revcenter.py,
			(legacy_u8)simd_player.revpoints[dot_index],
			(legacy_u8)simd_player.revpoints[
				dot_index + DASHBOARD_POINT_Y_OFFSET],
			meter_needle_color);
		if (wheel_state == DASHBOARD_WHEEL_STATE_LEFT) {
			shape2d_render_bmp_as_mask(
				whlshapes[DASHBOARD_LEFT_INSTRUMENT_MASK_SHAPE]);
			shape2d_op_unk4(dos_memory_pointer_offset(
				whlshapes[DASHBOARD_LEFT_INSTRUMENT_SOURCE_SHAPE]),
				dos_memory_pointer_segment(
					whlshapes[DASHBOARD_LEFT_INSTRUMENT_SOURCE_SHAPE]));
		} else if (wheel_state == DASHBOARD_WHEEL_STATE_RIGHT) {
			shape2d_render_bmp_as_mask(
				whlshapes[DASHBOARD_RIGHT_INSTRUMENT_MASK_SHAPE]);
			shape2d_op_unk4(dos_memory_pointer_offset(
				whlshapes[DASHBOARD_RIGHT_INSTRUMENT_SOURCE_SHAPE]),
				dos_memory_pointer_segment(
					whlshapes[DASHBOARD_RIGHT_INSTRUMENT_SOURCE_SHAPE]));
		}
		if (video_flag5_is0 != 0)
			setup_mcgawnd2();
		else
			sprite_copy_2_to_1_2();
		dashboard_set_viewport();
		sprite_putimage_and_alt(whlsprite1->sprite_bitmapptr,
			shape2d_get_pos_x(
				whlshapes[DASHBOARD_INSTRUMENT_PANEL_SHAPE]),
			shape2d_get_pos_y(
				whlshapes[DASHBOARD_INSTRUMENT_PANEL_SHAPE]));
	}

	if (word_40E00[player_index] != steering_position ||
		byte_454A4 != 0 || steering_dot_cleared != 0) {
		if (video_flag5_is0 == 0)
			mouse_draw_opaque_check();
		dashboard_set_viewport();
		(void)dashboard_clear_steering_dot(player_index);
		steering_dots = (legacy_u8*)simd_player.steeringdots;
		dot_index = (legacy_u16)(steering_position < 0 ?
			LEGACY_S16_WRAP_NEGATE(steering_position) :
			steering_position) * DASHBOARD_POINT_COORDINATE_STRIDE;
		dot_x = steering_dots[dot_index];
		dot_y = steering_dots[dot_index + DASHBOARD_POINT_Y_OFFSET];
		if (steering_position < 0) {
			dot_x = (legacy_u8)(dot_x -
				(legacy_u8)((legacy_u8)(dot_x - steering_dots[0]) <<
					DASHBOARD_STEERING_MIRROR_SHIFT));
		}
		word_40DF2[player_index] = LEGACY_S16_FROM_BITS(
			((legacy_u16)((legacy_u8)dot_x -
				shape2d_get_unk1(
					gnobshapes[DASHBOARD_STEERING_DOT_SHAPE]))) &
			(legacy_u16)video_flag3_isFFFF);
		word_40DF6[player_index] = LEGACY_S16_FROM_BITS(
			LEGACY_U16_WRAP_SUB((legacy_u8)dot_y,
				shape2d_get_unk2(
					gnobshapes[DASHBOARD_STEERING_DOT_SHAPE])));
		sprite_clear_shape_alt(
			gnobshapes[DASHBOARD_STEERING_DOT_BUFFER_FIRST_SHAPE +
				(legacy_u8)byte_44346],
			word_40DF2[player_index], word_40DF6[player_index]);
		sprite_putimage_and_alt2(
			gnobshapes[DASHBOARD_STEERING_DOT_BACKGROUND_SHAPE], dot_x, dot_y);
		sprite_putimage_or_alt(
			gnobshapes[DASHBOARD_STEERING_DOT_SHAPE], dot_x, dot_y);
		word_40E00[player_index] = steering_position;
	}
	mouse_draw_transparent_check();
}
