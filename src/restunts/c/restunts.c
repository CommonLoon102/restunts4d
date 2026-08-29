#include <dos.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include "restunts.h"
#include "fileio.h"
#include "keyboard.h"
#include "legacy.h"
#include "math.h"
#include "memmgr.h"
#include "shape2d.h"
#include "shape3d.h"

extern legacy_s16 __stbuf(FILE* stream);
extern void __ftbuf(legacy_s16 buffer_state, FILE* stream);
extern legacy_s16 __output(FILE* stream, const legacy_s8* format, void* arguments);

// Entries in the CVX gamestate buffer.
#define RST_CVX_NUM 20

// ASCII code properties map.
#define RST_ASC_CHAR_UPPER 0x01
#define RST_ASC_CHAR_LOWER 0x02
#define RST_ASC_NUMBER     0x04
#define RST_ASC_WHITESPACE 0x08
#define RST_ASC_PUNCTATION 0x10
#define RST_ASC_CONTROL    0x20
#define RST_ASC_SPACE      0x40
#define RST_ASC_HEX        0x80

// Use the Stunts' data for now.
extern const legacy_u8* g_ascii_props;
extern legacy_u8 dos_joystick_enabled;
extern legacy_u16 dos_joystick_axis1;
extern legacy_u16 dos_joystick_axis2;
extern legacy_u16 dos_joystick_axis1_min;
extern legacy_u16 dos_joystick_axis1_max;
extern legacy_u16 dos_joystick_axis2_min;
extern legacy_u16 dos_joystick_axis2_max;
extern legacy_u16 dos_joystick_axis1_scale;
extern legacy_u16 dos_joystick_axis2_scale;
extern legacy_u8 byte_3FB38[];
extern legacy_u8 byte_449CE;
extern legacy_u8 byte_3BD34[];
extern legacy_s16 word_46170[7];
extern legacy_u8 byte_44292[64];
extern legacy_u8 byte_442EA[64];
extern legacy_u8 far* pboxshape;
extern legacy_s16 dos_mouse_button_count;
extern void (far* dos_timer_callbacks[6])(void);
extern legacy_u32 dos_timer_counter;
extern legacy_s16 dos_timer_callbacks_suspended;
legacy_u32 timer_get_counter(void);
legacy_u32 timer_get_delta(void);
legacy_u32 timer_get_slow_counter(void);
void dos_timer_setup_interrupt(void);
void dos_timer_shutdown(void);
legacy_s16 dos_video_get_status(void);
void dos_video_set_palette(legacy_u16 start, legacy_u16 count,
	legacy_u8* palette);
void dos_video_set_mode_13h(void);
void dos_video_set_mode4(void);
void dos_video_set_mode7(void);
void dos_set_critical_error_handler(legacy_s16 (far* callback)(void));
void dos_process_exit(legacy_s16 status);

static legacy_u32 timer_wait_target;

typedef legacy_s16 (far* readchar_callback_type)(void);

static legacy_u8 input_callback_flags[128];
static legacy_u8 input_extended_callback_flags[133];
static void (far* input_callbacks[64])(void);
static legacy_u8 input_callback_dispatching;
static readchar_callback_type input_readchar_callback = kb_read_char;
static legacy_s8 input_callback_overflow_message[] =
	"NO ROOM LEFT ON TIMER INTERRUPT ROUTINE LIST\r";
/*
unsigned const char g_ascii_props[256] = {
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x28, 0x28, 0x28, 0x28, 0x28, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	0x48, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x10, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x10, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x10, 0x10, 0x10, 0x10, 0x20,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
*/

legacy_s16 get_0(void)
{
	return 0;
}

void kb_reg_callback(legacy_s16 code, void (far* callback)(void))
{
	legacy_u16 code_bits;
	legacy_u16 callback_index;
	legacy_u16 key_index;

	for (callback_index = 0; callback_index < 64U; callback_index++) {
		if (input_callbacks[callback_index] == callback)
			break;
		if (FP_SEG(input_callbacks[callback_index]) == 0U) {
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
	disable();
	if (input_callback_dispatching != 0) {
		enable();
		return LEGACY_S16_FROM_BITS(code_bits);
	}
	input_callback_dispatching = 1;
	enable();

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

void timer_reg_callback(void (far* callback)(void))
{
	legacy_u16 callback_index;
	legacy_u16* callback_words;

	for (callback_index = 0; callback_index < 5U; callback_index++) {
		if (FP_SEG(dos_timer_callbacks[callback_index]) == 0U)
			break;
	}
	if (callback_index == 5U) {
		fatal_error(input_callback_overflow_message);
		return;
	}

	callback_words = (legacy_u16*)dos_timer_callbacks + callback_index * 2U;
	callback_words[0] = FP_OFF(callback);
	callback_words[1] = 0;
	callback_words[1] = FP_SEG(callback);
	callback_words[3] = 0;
}

void timer_remove_callback(void (far* callback)(void))
{
	legacy_u16 callback_index;

	for (callback_index = 0; callback_index < 5U; callback_index++) {
		if (dos_timer_callbacks[callback_index] == callback)
			break;
	}
	if (callback_index == 5U)
		return;

	disable();
	while (callback_index < 4U) {
		dos_timer_callbacks[callback_index] = dos_timer_callbacks[callback_index + 1U];
		callback_index++;
	}
	dos_timer_callbacks[4] = 0;
	enable();
}

void sub_307B4(void)
{
	dos_joystick_enabled = 1;
	dos_joystick_axis1_min = 0x50U;
	dos_joystick_axis1_max = 0;
	dos_joystick_axis2_min = 0x50U;
	dos_joystick_axis2_max = 0;
}

legacy_s16 sub_307D2(legacy_s16 index)
{
	return byte_3FB38[(legacy_u16)index & 0x0FU];
}

legacy_s16 sub_307E3(void)
{
	legacy_u16 difference;
	legacy_u32 scaled;

	if (LEGACY_S16_FROM_BITS(dos_joystick_axis1) <
		LEGACY_S16_FROM_BITS(dos_joystick_axis1_min))
		difference = 0;
	else
		difference = LEGACY_U16_WRAP_SUB(dos_joystick_axis1, dos_joystick_axis1_min);
	scaled = (legacy_u32)difference * dos_joystick_axis1_scale;
	return (legacy_u16)((legacy_u16)(scaled >> 8) - 0x1FU);
}

legacy_s16 nopsub_307FA(void)
{
	legacy_u16 difference;
	legacy_u32 scaled;

	if (LEGACY_S16_FROM_BITS(dos_joystick_axis2) <
		LEGACY_S16_FROM_BITS(dos_joystick_axis2_min))
		difference = 0;
	else
		difference = LEGACY_U16_WRAP_SUB(dos_joystick_axis2, dos_joystick_axis2_min);
	scaled = (legacy_u32)difference * dos_joystick_axis2_scale;
	return (legacy_u16)((legacy_u16)(scaled >> 8) - 0x1FU);
}

legacy_s16 nopsub_30A77(void)
{
	legacy_s16 key;

	do {
		key = kb_call_readchar_callback();
		if (key != 0)
			return key;
	} while (timer_get_counter() < timer_wait_target);
	return 0;
}

legacy_s16 nopsub_30A97(legacy_u32 ticks)
{
	legacy_u32 target;
	legacy_s16 key;

	target = (legacy_u32)(timer_get_counter() + ticks);
	do {
		key = kb_call_readchar_callback();
		if (key != 0)
			return key;
	} while ((legacy_u32)timer_get_counter() < target);
	return 0;
}

legacy_u32 timer_get_delta_alt(void)
{
	return timer_get_delta();
}

legacy_u32 timer_custom_delta(legacy_u32 ticks)
{
	return timer_get_counter() - ticks;
}

void timer_reset()
{
	dos_timer_counter = 0;
}

legacy_u32 timer_copy_counter(legacy_u32 ticks)
{
	timer_wait_target = timer_get_counter() + ticks;
	return timer_wait_target;
}

legacy_u32 timer_wait_for_dx(void)
{
	legacy_u32 res;
	do {
		res = timer_get_counter();
	} while (res < timer_wait_target);
	
	return res;
}

legacy_s16 timer_compare_dx(void)
{
	return timer_get_counter() >= timer_wait_target;
}

legacy_u32 timer_get_counter_unk(legacy_u32 ticks)
{
	legacy_u32 target, res;
	target = timer_get_counter() + ticks;
	
	do {
		res = timer_get_counter();
	} while (res < target);
	
	return res;
}

extern legacy_u16 word_46468;
extern legacy_u8 byte_442E4;
extern legacy_s16 word_44D1E;
extern legacy_s16 word_44D20;
extern legacy_s16 word_449E4;
extern legacy_s16 word_443F4;
extern legacy_u8 unk_44F4C[];
extern legacy_u8 byte_3BE02;
extern legacy_s8 byte_3E85C[];
extern legacy_s8 byte_40D6A;
extern void far frame_callback(void);
extern void replay_unk2(legacy_s16 mode);
extern void timer_reg_callback(void (far* callback)(void));
extern void timer_remove_callback(void (far* callback)(void));

void set_frame_callback(void)
{
	word_46468 = 0;
	timer_reg_callback(&frame_callback);
	byte_442E4 = 0;
}

void remove_frame_callback(void)
{
	timer_get_counter_unk(10UL);
	timer_remove_callback(&frame_callback);
}

void do_opponent_op(void)
{
	opponent_op();
}

legacy_s16 get_super_random(void)
{
	legacy_s16 val = rand() + get_kevinrandom() + timer_get_counter() + gState_frame;
	return val < 0 ? -val : val;
}

legacy_s16 random_wait(void)
{
	legacy_s16 status1, i;
	
	status1 = dos_video_get_status();
	
	for (i = 0; status1 == dos_video_get_status() && i < 12000; ++i);
	
	if (i == 1024) {
		i = aMisc_1[0];
	}
	
	while (i--) {
		rand();
		get_kevinrandom();
	}
	
	i &= 0xFF;
	
	while (i--) {
		get_kevinrandom();
		rand();
	}
	
	return 0;
}

legacy_s16 toupper(legacy_s16 ch)
{
	if (ch >= 'a' && ch <= 'z') {
		ch -= ' ';
	}
	
	return ch;
}

void set_default_car(void) {
	gameconfig.game_playercarid[0]     = 'C';
	gameconfig.game_playercarid[1]     = 'O';
	gameconfig.game_playercarid[2]     = 'U';
	gameconfig.game_playercarid[3]     = 'N';
	gameconfig.game_playermaterial     = 0;
	gameconfig.game_playertransmission = 1;
	gameconfig.game_opponenttype       = 0;
	gameconfig.game_opponentmaterial   = 0;
	gameconfig.game_opponentcarid[0]   = 0xFF;
}

extern struct SHAPE3D game3dshapes[];

extern legacy_u16 select_cliprect_rotate(legacy_s16 angX, legacy_s16 angY, legacy_s16 angZ, struct RECTANGLE* cliprect, legacy_s16 unk);
//extern void transformed_shape_op(struct TRANSFORMSHAPE3D* shape);
extern void sub_29772(void);
extern void set_projection(legacy_s16, legacy_s16, legacy_s16, legacy_s16);

struct RECTANGLE shaperect = { 0, 320, 0, 200 };
struct TRANSFORMEDSHAPE3D transshape;
struct RECTANGLE cliprect = { 0, 0x140, 0, 0x5F };
struct VECTOR carpos = { 0, 0x0FCB8, 0x0B40 }; // from the original
//struct VECTOR carpos = { 0, 0, 320 };

extern struct SPRITE far* wndsprite;
extern struct SPRITE far* smouspriteptr;
extern struct SPRITE far* mmouspriteptr;
extern struct SPRITE far* mouseunkspriteptr;
extern void far* tempdataptr;
extern struct RECTANGLE cliprect_unk;
//cliprect_unk    RECTANGLE <270Fh, 0FFFFh, 270Fh, 0FFFFh>

extern legacy_s16 polyinfonumpolys;
extern legacy_u8 far* polyinfoptrs[]; // array size = 0x190 
extern legacy_u16 poly_linked_list_40ED6[]; // array size = 0x190

extern void preRender_default(legacy_s16 color, legacy_s16 vertlinecount, legacy_s16* vertlines);
extern legacy_s8 byte_3B8F6;
extern legacy_s8 far* skybox_res_ofs;
extern legacy_s8 far* sdgame2ptr;
extern legacy_s16 sdgame2_widths[];
extern legacy_s8 far* sdgame2shapes[];
extern legacy_s8 byte_46167;
extern legacy_u16 skybox_ptr1;
extern legacy_u16 skybox_ptr2;
extern legacy_u16 skybox_ptr3;
extern legacy_u16 skybox_ptr4;
extern legacy_u16 skybox_current;
extern legacy_u16 word_454CE;
extern legacy_s16 skybox_sky_color;
extern legacy_s16 skybox_grd_color;
extern legacy_s16 skybox_wat_color;
extern legacy_s16 dialog_fnt_colour;
extern legacy_s16 meter_needle_color;
extern legacy_s8 far* stdaresptr;
extern legacy_s8 far* stdbresptr;
extern struct SHAPE2D far* whlshapes[];
extern struct SHAPE2D far* gnobshapes[];
extern struct SHAPE2D far* digshapes[];
extern struct SPRITE far* whlsprite1;
extern struct SPRITE far* whlsprite2;
extern struct SPRITE far* whlsprite3;
extern legacy_s16 word_40D6C[];
extern legacy_s16 word_40D70[];
extern legacy_s16 word_40D74[];
extern legacy_s16 word_40D78[];
extern legacy_s16 word_40DF2[];
extern legacy_s16 word_40DF6[];
extern legacy_s16 word_40E00[];
extern legacy_u8 byte_40DF0[];
extern legacy_u8 byte_40DFA[];
extern legacy_s8 aWhl1whl2whl3ins2gboxins1i[];
extern legacy_s8 aGnobgnabdotDotadot1dot2[];
extern legacy_s8 aDig0dig1dig2dig3dig4dig5d[];
extern legacy_s8 aDash[];
extern legacy_s8 aRoof[];
extern legacy_s8 aDast[];
extern legacy_s8 aDasm[];
extern legacy_s8 aStdaxxxx[];
extern legacy_s8 aStdbxxxx[];
extern legacy_s8 far* skyboxes[];
extern legacy_s16 word_45D1C;
extern legacy_s16 word_45D06;
extern legacy_s16 idle_counter;
extern legacy_s8 byte_3B8F7;
extern legacy_s8 mouse_isdirty;
extern legacy_u8 HKeyFlag;

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
	mouse_width = (legacy_u16)(mouse_shape->s2d_width * video_flag2_is1);
	mouse_height = mouse_shape->s2d_height;
	mmgr_free(resource);

	smouspriteptr = sprite_make_wnd(mouse_width, mouse_height, 0x0F);
	mmouspriteptr = sprite_make_wnd(mouse_width, mouse_height, 0x0F);
	mouseunkspriteptr = sprite_make_wnd(
		mouse_width + video_flag2_is1, mouse_height, 0x0F);

	resource = (legacy_s8 far*)file_load_shape2d_fatal("sdmain");
	sprite_set_1_from_argptr(smouspriteptr);
	mouse_shape = (struct SHAPE2D far*)locate_shape_fatal(resource, "smou");
	sprite_shape_to_1(mouse_shape, 0, 0);

	sprite_set_1_from_argptr(mmouspriteptr);
	mouse_shape = (struct SHAPE2D far*)locate_shape_fatal(resource, "mmou");
	sprite_shape_to_1(mouse_shape, 0, 0);

	mmgr_free(resource);
	sprite_copy_2_to_1_2();
}

static legacy_s8 skybox_resource_names[5][9] = {
	"desert",
	"tropical",
	"alpine",
	"city",
	"country"
};

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
		HKeyFlag ^= 1;
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
	framespersec = framespersec2;
	*(legacy_u8*)&gameconfig.game_framespersec = (legacy_u8)framespersec2;
	init_game_state(-1);
	return 1;
}

void sub_29772(void)
{
	word_45D1C = 0;
	word_45D06 = 0;
	idle_counter = 0;
}

void mouse_draw_transparent_check(void)
{
	byte_3B8F7 = 1;
	if (kbormouse != 0 && mouse_isdirty == 0)
		mouse_draw_transparent();
}

void mouse_draw_opaque_check(void)
{
	byte_3B8F7 = 0;
	if (mouse_isdirty != 0)
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

extern legacy_s16 dos_get_joy_flags(void);
extern legacy_u8 dos_kb_input[];
extern legacy_s16 dos_mouse_init(legacy_s16 width, legacy_s16 height);
extern void dos_mouse_set_minmax(legacy_s16 minimum_x, legacy_s16 minimum_y,
	legacy_s16 maximum_x, legacy_s16 maximum_y);
extern void dos_mouse_set_position(legacy_s16 x, legacy_s16 y);
extern void dos_mouse_get_state(legacy_s16* buttons, legacy_s16* x,
	legacy_s16* y);

static legacy_s16 input_elapsed_frames;
static legacy_s16 input_mouse_repeat_at;
static legacy_s16 input_joystick_repeat_at;
static legacy_s16 input_mouse_idle_frames;
static legacy_s16 input_combined_flags;
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
		if (dos_kb_input[input_key_scancodes[index]] != 0)
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
		if (byte_3B8F7 != 0) {
			if (mouse_isdirty != 0)
				mouse_draw_opaque();
			mouse_draw_transparent();
		}
	} else if (kbormouse != 0) {
		input_mouse_idle_frames = LEGACY_U16_WRAP_ADD(
			input_mouse_idle_frames, frame_delta);
		if (LEGACY_S16_FROM_BITS(input_mouse_idle_frames) > 500) {
			input_mouse_idle_frames = 0;
			kbormouse = 0;
			if (mouse_isdirty != 0)
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
	return (legacy_s16)((legacy_s32)numerator / (legacy_s32)denominator);
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
		quotient = (legacy_s16)(quotient >> 1);
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

	input_draw_mode_stack[index] = byte_3B8F7;
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
	byte_3B8F7 = input_draw_mode_stack[index];
	kbormouse = input_device_mode_stack[index];
	if (kbormouse == 0)
		mouse_draw_opaque_check();
}

extern legacy_s16 font_op2(const legacy_s8* text);
extern legacy_s16 font_op(const legacy_s8* text, legacy_s16 count);
extern legacy_u16 word_42A16;
extern legacy_u16 word_42A18;
extern legacy_u16 word_42A1A;
extern legacy_u16 word_42A1C;
extern legacy_s8* off_42A1E;
extern legacy_u16 word_42A20;
extern legacy_u16 word_42A22;
extern legacy_u8 far* word_405FE;
extern void sub_345BC(const legacy_s8* text, legacy_s16 x, legacy_s16 y);
extern void sprite_1_unk2(legacy_s16 x, legacy_s16 y, legacy_s16 width, legacy_s16 height, legacy_s16 color);

legacy_s16 font_op2_alt(const legacy_s8* text)
{
	legacy_s16 centered;

	centered = LEGACY_S16_WRAP_NEGATE(
		LEGACY_S16_WRAP_SUB(font_op2(text), 0x140));
	return (legacy_s16)((legacy_s32)centered / 2L);
}

void unload_skybox(void)
{
	if (byte_3B8F6 != 0)
		mmgr_free(skybox_res_ofs);
	byte_3B8F6 = 0;
}

void free_sdgame2(void)
{
	mmgr_free(sdgame2ptr);
}

void audio_fade_out(legacy_s16 delay_ticks);
extern legacy_s8 audio_music_enabled;
extern legacy_s8 audio_effects_enabled;
extern legacy_s16 audio_update_lock;
extern legacy_s16 word_4063C;
extern legacy_u8 dos_audio_special_mode;
extern legacy_s8 audiodriverstring2[];
extern legacy_u8 byte_44290;
extern legacy_u8 byte_40630;
extern legacy_u8 byte_40632;
extern legacy_u8 byte_45950;
extern legacy_u8 dos_audio_context_count;
extern legacy_s8 byte_459D8;
extern legacy_s8 byte_42D26;
extern legacy_s8 byte_42D2A;
extern legacy_u8 byte_428BE[];
extern legacy_u8 byte_428D6[];
extern legacy_u8 audio_channels[];
extern legacy_u8* audio_sfx_channels;
extern legacy_u8 byte_45948;
extern legacy_u8 byte_45D9A[];
extern legacy_u8 byte_44D06[];
extern legacy_u8 byte_44ACA[];
extern legacy_u8 dos_audio_contexts[];
extern legacy_u8 audio_timers[];
extern void far* basdres;
extern void far* snarres;
extern void far* tommres;
extern void far* rideres;
extern void far* crshres;
extern void far* chhtres;
extern void far* ohhtres;
extern legacy_s16 word_43964;
extern legacy_s16 word_4408C;
extern legacy_u16 word_42240;
extern legacy_u16 word_42242;
extern legacy_u16 word_42244;
extern legacy_u8 byte_42246;
extern legacy_s16 word_3EB2A;
extern legacy_u8 dos_audio_uses_direct_channels;
extern legacy_u8 dos_audio_master_state[];
extern legacy_u8 dos_audio_master_volume;
extern void far* dos_audio_driver_binary;
extern legacy_u16 word_44D48;
extern legacy_u16 word_454BA;
extern legacy_s8 audio_filetemp[];
legacy_s16 dos_data_stack_segments_match(void);
void audio_driver_timer(void);
extern void dos_audio_bind_channel_context(legacy_s16 channel,
	void far* resource);
extern void audio_map_song_instruments(void far* song,
	void far* instruments);
extern void audio_map_song_tracks(void far* song);
legacy_s16 audio_start_sample(legacy_u16 value, legacy_s16 handle);
extern void dos_audio_set_context_pitch(legacy_s16 context_index,
	legacy_s16 pitch);
extern void sub_35B76(legacy_s16 x, legacy_s16 y, legacy_s16 width, legacy_s16 height, legacy_s16 color);
void audio_release_channel_range(legacy_s16 first_channel,
	legacy_s16 last_channel);
extern void dos_audio_set_channel_volume(legacy_s16 channel,
	legacy_s16 volume);
extern void audio_op_unk3(legacy_s16 channel);
extern void audio_op_unk4(legacy_s16 channel);
extern void dos_audio_driver_release_channel(legacy_s16 driver_channel);
extern void dos_audio_driver_start_context(legacy_s16 driver_channel,
	legacy_u8* driver_context);
extern void dos_audio_driver_end_context(legacy_s16 driver_channel,
	legacy_u8* driver_context);
extern void dos_audio_driver_prepare_context(legacy_s16 driver_channel,
	legacy_u8* driver_context, legacy_u8* timer, void far* resource);
extern void dos_audio_driver_set_context_value(legacy_s16 driver_channel,
	legacy_u8* driver_context, legacy_u16 value);
extern void dos_audio_driver_activate_context(legacy_s16 driver_channel,
	legacy_u8* driver_context, legacy_u8* timer, legacy_s16 pitch,
	legacy_u16 parameter, void far* resource);
extern void dos_audio_driver_reset(void);
extern void dos_audio_driver_start(void);
extern void dos_audio_driver_suspend_context(legacy_s16 driver_channel,
	legacy_u8* driver_context, legacy_u16 value, void far* resource);
extern void dos_audio_driver_suspend_all(legacy_u8* contexts);
extern void dos_audio_driver_set_master_state(legacy_s16 operation,
	void far* state);
extern legacy_u8 dos_audio_driver_initialize(void);
extern void dos_audio_driver_load_bank(void far* bank);
extern void dos_audio_shutdown(void);
void add_exit_handler(void (far* exit_handler)(void));
void audio_update_driver_contexts(void);
legacy_s16 sub_37470(legacy_s16 channel, legacy_u8 priority);
void sub_374DE(legacy_s16 channel);
legacy_s16 sub_3771E(legacy_s16 channel);
void audio_init_chunk2(legacy_s16 channel);
void sub_38156(legacy_s16 index);
extern legacy_s16 audio_check_flag(void far* resource, legacy_s16 channel,
	legacy_u8 priority, legacy_u16 rate);
extern void audio_init_chunk(legacy_s16 first_channel, legacy_s16 last_channel,
	void far* resource, legacy_u16 resource_data_offset,
	legacy_u16 rate, legacy_u8 priority);

static void far* audio_read_far_pointer(const legacy_u8* source)
{
	return MK_FP(LEGACY_READ_U16_LE(source + 2),
		LEGACY_READ_U16_LE(source));
}

static void audio_write_far_pointer(legacy_u8* destination,
	const void far* value)
{
	LEGACY_WRITE_U16_LE(destination, FP_OFF(value));
	LEGACY_WRITE_U16_LE(destination + 2, FP_SEG(value));
}

void audio_add_driver_timer(void)
{
	legacy_u16 index;

	for (index = 0; index < 25U; index++)
		audio_timers[index * 0x4CU] = 0;
	word_42240 = 0x16U;
	timer_reg_callback(&audio_driver_timer);
}

void audio_remove_driver_timer(void)
{
	legacy_u16 index;
	legacy_u16 offset;
	legacy_s16 channel;

	for (index = 0; index < 25U; index++) {
		offset = index * 0x4CU;
		if (audio_timers[offset] == 1) {
			channel = LEGACY_S16_FROM_BITS(
				LEGACY_READ_U16_LE(audio_timers + offset + 2U));
			sub_374DE(channel);
		}
		audio_timers[offset] = 0;
	}
	timer_remove_callback(&audio_driver_timer);
}

legacy_s8* pad_id(const legacy_s8 far* source)
{
	legacy_u8* destination;
	legacy_u16 index;

	destination = (legacy_u8*)&word_42242;
	for (index = 0; index < 4U; index++) {
		destination[index] = (legacy_u8)source[index];
		if (destination[index] == 0)
			destination[index] = ' ';
	}
	byte_42246 = 0;
	return (legacy_s8*)destination;
}

legacy_s16 audio_init_engine(legacy_s16 unused_type, void far* source_pointer,
	void far* shape_resources, void far* audio_resources)
{
	const legacy_u8 far* source;
	legacy_u8* timer;
	legacy_u8* context;
	const legacy_u8 far* definition;
	void far* resource;
	legacy_u16 source_offset;
	legacy_u16 source_segment;
	legacy_u16 rate;
	legacy_u16 divisor;
	legacy_u16 index;
	legacy_u16 field;
	legacy_s16 channel;

	(void)unused_type;
	for (index = 0; index < 25U; index++) {
		if (audio_timers[index * 0x4CU] == 0)
			break;
	}
	if (index == 25U) {
		fatal_error("InitEngine: All handles used.");
		return -1;
	}

	timer = audio_timers + index * 0x4CU;
	context = timer + 0x1CU;
	source_offset = (legacy_u16)FP_OFF(source_pointer);
	source_segment = (legacy_u16)FP_SEG(source_pointer);
	for (field = 0; field < 0x30U; field++) {
		source = (const legacy_u8 far*)MK_FP(
			source_segment, source_offset);
		context[field] = *source;
		source_offset++;
		if (source_offset == 0)
			source_segment = LEGACY_U16_WRAP_ADD(
				source_segment, 0x1000U);
	}

	if (context[6] == 0) {
		resource = locate_shape_fatal((legacy_s8 far*)shape_resources,
			pad_id((const legacy_s8 far*)audio_read_far_pointer(
				context + 8U)));
		audio_write_far_pointer(context + 8U, resource);
		for (field = 0x10U; field <= 0x2CU; field += 4U) {
			resource = init_audio_resources(audio_resources,
				shape_resources,
				pad_id((const legacy_s8 far*)audio_read_far_pointer(
					context + field)));
			audio_write_far_pointer(context + field, resource);
		}
		context[6] = 1;
	}

	channel = sub_37470(-1, 0x7FU);
	LEGACY_WRITE_U16_LE(timer + 2U, channel);
	timer[1] = 0;
	LEGACY_WRITE_U16_LE(timer + 4U, 0);
	LEGACY_WRITE_U16_LE(timer + 6U, 0);
	LEGACY_WRITE_U16_LE(timer + 8U, 0);
	timer[0x0AU] = 0;
	definition = (const legacy_u8 far*)audio_read_far_pointer(context + 8U);
	divisor = definition[0x0EU];
	rate = (legacy_u16)(LEGACY_READ_U16_LE(context) / divisor);
	rate = LEGACY_U16_WRAP_ADD(rate,
		(legacy_u16)((legacy_u16)definition[0x0FU] << 4));
	LEGACY_WRITE_U16_LE(timer + 0x0CU, rate);
	timer[0x0EU] = 0xFFU;
	LEGACY_WRITE_U16_LE(timer + 0x10U, 0xFFFFU);
	LEGACY_WRITE_U16_LE(timer + 0x12U, 0xFFFFU);
	LEGACY_WRITE_U16_LE(timer + 0x14U, 0xFFFFU);
	LEGACY_WRITE_U16_LE(timer + 0x16U, 0xFFFFU);
	LEGACY_WRITE_U16_LE(timer + 0x18U,
		LEGACY_READ_U16_LE(context));
	timer[0x1AU] = 0;
	timer[0x1BU] = 0;
	timer[0] = 1;
	return (legacy_s16)index;
}

void audio_op_unk(legacy_s16 index)
{
	legacy_u8* timer;
	legacy_u8* context;
	const legacy_u8 far* definition;
	legacy_u16 offset;
	legacy_u16 sample_count;
	legacy_u16 value;
	legacy_u16 divisor;
	legacy_s16 handle;
	legacy_s16 channel;

	offset = LEGACY_U16_WRAP_MUL(index, 0x4CU);
	timer = audio_timers + offset;
	if (timer[0] != 1 || timer[1] != 0)
		return;

	handle = LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(timer + 2U));
	context = timer + 0x1CU;
	dos_audio_bind_channel_context(handle,
		audio_read_far_pointer(context + 8U));
	sample_count = LEGACY_READ_U16_LE(context);
	definition = (const legacy_u8 far*)audio_read_far_pointer(context + 8U);
	divisor = definition[0x0EU];
	value = (legacy_u16)(sample_count / divisor);
	value = LEGACY_U16_WRAP_ADD(value,
		(legacy_u16)((legacy_u16)definition[0x0FU] << 4));
	LEGACY_WRITE_U16_LE(timer + 0x0CU, value);
	channel = audio_start_sample(value, handle);
	LEGACY_WRITE_U16_LE(timer + 0x12U, channel);
	timer[1] = 1;
	timer[0x1AU] = 1;
	dos_audio_set_channel_volume(handle, 0);
}

void audio_op_unk2(legacy_s16 index, legacy_s16 base_value,
	legacy_s16 first_x, legacy_s16 first_y, legacy_s16 first_z,
	legacy_s16 second_x, legacy_s16 second_y, legacy_s16 second_z,
	legacy_s16 interval)
{
	legacy_u8* timer;
	legacy_u8* context;
	const legacy_u8 far* definition;
	legacy_u16 offset;
	legacy_u16 first_distance;
	legacy_u16 second_distance;
	legacy_u16 distance_delta;
	legacy_u16 scaled_delta;
	legacy_u16 volume;
	legacy_u16 base_rate;
	legacy_u16 denominator;
	legacy_u16 divisor;
	legacy_u16 quotient;

	offset = LEGACY_U16_WRAP_MUL(index, 0x4CU);
	timer = audio_timers + offset;
	second_distance = (legacy_u16)polarRadius2D(
		polarRadius2D(second_x, second_z), second_y);
	if (LEGACY_S16_FROM_BITS(second_distance) > 0x1770) {
		timer[0x0AU] = 0;
		return;
	}

	first_distance = (legacy_u16)polarRadius2D(
		polarRadius2D(first_x, first_z), first_y);
	distance_delta = LEGACY_U16_WRAP_SUB(
		first_distance, second_distance);
	quotient = (legacy_u16)(100U / (legacy_u16)interval);
	scaled_delta = LEGACY_U16_WRAP_MUL(quotient, distance_delta);
	quotient = (legacy_u16)(((legacy_u32)0x7FU * second_distance) /
		0x1770UL);
	volume = LEGACY_U16_WRAP_SUB(0x7FU, quotient);
	if (LEGACY_S16_FROM_BITS(scaled_delta) > 0)
		volume = LEGACY_U16_WRAP_SUB(volume, volume >> 4);

	context = timer + 0x1CU;
	definition = (const legacy_u8 far*)audio_read_far_pointer(context + 8U);
	divisor = definition[0x0EU];
	base_rate = (legacy_u16)((legacy_u16)base_value / divisor);
	base_rate = LEGACY_U16_WRAP_ADD(base_rate,
		(legacy_u16)((legacy_u16)definition[0x0FU] << 4));
	denominator = LEGACY_U16_WRAP_SUB(0x1770U, scaled_delta);
	if (denominator != 0) {
		base_rate = (legacy_u16)(((legacy_u32)0x1770U * base_rate) /
			denominator);
		LEGACY_WRITE_U16_LE(timer + 0x0CU, base_rate);
	}
	timer[0x0AU] = (legacy_u8)volume;
}

void sub_18D06(const legacy_u8* sample, legacy_s16 interval)
{
	audio_op_unk2(word_43964,
		LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(sample + 0x1EU)),
		LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(sample + 6U)),
		LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(sample + 8U)),
		LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(sample + 0x0AU)),
		LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(sample + 0x0CU)),
		LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(sample + 0x0EU)),
		LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(sample + 0x10U)),
		interval);
	if (gameconfig.game_opponenttype != 0) {
		audio_op_unk2(word_4408C,
			LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(sample + 0x20U)),
			LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(sample + 0x12U)),
			LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(sample + 0x14U)),
			LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(sample + 0x16U)),
			LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(sample + 0x18U)),
			LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(sample + 0x1AU)),
			LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(sample + 0x1CU)),
			interval);
	}
}

void frame_callback(void)
{
	if (dos_data_stack_segments_match() == 0 || byte_442E4 != 0)
		return;

	byte_442E4 = 1;
	word_443F4 = LEGACY_S16_WRAP_ADD(word_443F4, 1);
	if (word_443F4 >= word_4499C && word_44D1E != word_449E4) {
		sub_18D06(unk_44F4C + 0x22U * (legacy_u16)word_44D1E,
			word_443F4);
		word_443F4 = 0;
		word_44D1E = LEGACY_S16_WRAP_ADD(word_44D1E, 1);
		if (word_44D1E == 0x28)
			word_44D1E = 0;
	}

	if (byte_449DA == 0 && byte_46467 == 0 &&
		(is_in_replay == 0 || game_replay_mode != 2)) {
		if (game_replay_mode == 0 &&
			LEGACY_S16_FROM_BITS(state.game_frame_in_sec) >=
			LEGACY_S16_FROM_BITS(state.game_frames_per_sec)) {
			is_in_replay = 1;
			audio_carstate();
		} else {
			byte_44A8A = (legacy_u8)(byte_44A8A - 1U);
			if (byte_44A8A == 0) {
				byte_44A8A = (legacy_u8)word_4499C;
				word_46468 = LEGACY_U16_WRAP_ADD(word_46468, 1U);
				if (game_replay_mode == 2 &&
					LEGACY_S8_FROM_BITS(byte_449E6) == 2) {
					byte_4552F = (legacy_u8)(byte_4552F - 1U);
					if (byte_4552F == 0) {
						replay_unk2(0);
						byte_4552F = 2;
					}
				} else {
					if (game_replay_mode == 2 &&
						LEGACY_S8_FROM_BITS(byte_449E6) == 3)
						replay_unk2(0);
					replay_unk2(0);
				}
			}
		}
	}

	byte_442E4--;
}

void replay_unk2(legacy_s16 mode)
{
	legacy_s16 input_flags;
	legacy_s16 steering;
	legacy_s16 snapshot_index;
	legacy_u16 history_index;
	legacy_u16 recording_chunk;
	legacy_u16 recording_limit;
	legacy_u16 elapsed_total;
	legacy_u16 input_index;
	legacy_s8 mapped_steering;

	if (mode != 0) {
		input_flags = 0;
	} else if (game_replay_mode == 2) {
		if (gameconfig.game_recordedframes > elapsed_time2) {
			elapsed_time2++;
			return;
		}
		if (byte_449DA != 0)
			return;
		is_in_replay = 1;
		audio_carstate();
		byte_449DA = 1;
		return;
	} else if (byte_449DA == 0 &&
		state.game_3F6autoLoadEvalFlag == 0 &&
		game_replay_mode != 1) {
		if (passed_security == 0 && byte_4393C == 0 &&
			(legacy_u16)state.game_frame >
				LEGACY_U16_WRAP_MUL(framespersec, 4U))
			update_crash_state(1, 0);

		if (byte_3B8F2 != 0 || dos_joystick_enabled != 0) {
			if (byte_3B8F2 != 0) {
				dos_mouse_get_state(
					&mouse_butstate, &mouse_xpos, &mouse_ypos);
				steering = LEGACY_S16_WRAP_SUB(mouse_xpos, 0xA0);
				if (steering > -0x12 && steering < 0x12) {
					steering = 0;
				} else if (steering > 0) {
					steering = LEGACY_S16_WRAP_SUB(steering, 0x12);
				} else {
					steering = LEGACY_S16_WRAP_ADD(steering, 0x12);
				}
				byte_40D6A = LEGACY_S8_FROM_BITS(steering);
				if (((legacy_u16)mouse_butstate & 1U) != 0)
					input_flags = 2;
				else if (((legacy_u16)mouse_butstate & 2U) != 0)
					input_flags = 1;
				else
					input_flags = 0;
			} else {
				mapped_steering = LEGACY_S8_FROM_BITS(sub_307E3());
				byte_40D6A = mapped_steering;
				if (mapped_steering > 0) {
					byte_40D6A = byte_3E85C[
						(legacy_u8)mapped_steering];
				} else if (mapped_steering < 0) {
					byte_40D6A = LEGACY_S8_FROM_BITS(
						(legacy_u8)(0U - (legacy_u8)byte_3E85C[
							(legacy_u8)(0U -
								(legacy_u8)mapped_steering)]));
				}
				input_flags = (legacy_s16)
					((legacy_u16)get_kb_or_joy_flags() & 0x33U);
			}
			history_index = (legacy_u16)elapsed_time2 & 0x3FU;
			byte_44292[history_index] = (legacy_u8)byte_40D6A;
			byte_442EA[history_index] = 1;
		} else {
			input_flags = get_kb_or_joy_flags();
		}

		if (kb_get_key_state(0x1E) != 0)
			input_flags = (legacy_s16)
				((legacy_u16)input_flags | 0x10U);
		if (kb_get_key_state(0x2C) != 0)
			input_flags = (legacy_s16)
				((legacy_u16)input_flags | 0x20U);
	} else {
		input_flags = 0;
	}

	recording_limit = LEGACY_U16_WRAP_MUL(0x5DCU, framespersec);
	elapsed_total = LEGACY_U16_WRAP_ADD(elapsed_time2, elapsed_time1);
	if (recording_limit <= elapsed_total) {
		update_crash_state(4, 0);
		byte_449DA = 1;
		return;
	}

	if (elapsed_time2 == 0x2EE0U) {
		if (elapsed_time1 == 0 &&
			*(legacy_u8*)&word_45D3E == 0) {
			*(legacy_u8*)&word_45D3E = 1;
			byte_46467 = 1;
			return;
		}

		recording_chunk = LEGACY_U16_WRAP_MUL(0x1EU, framespersec);
		for (snapshot_index = 0;
			snapshot_index <
				(legacy_s16)(0x2EE0 / recording_chunk) - 1;
			snapshot_index++) {
			cvxptr[snapshot_index + 1].game_frame =
				LEGACY_S16_WRAP_SUB(
					cvxptr[snapshot_index + 1].game_frame,
					recording_chunk);
			fmemcpy(&cvxptr[snapshot_index],
				&cvxptr[snapshot_index + 1],
				sizeof(struct GAMESTATE));
		}
		for (input_index = 0;
			input_index < (legacy_u16)(0x2EE0U - recording_chunk);
			input_index++)
			td16_rpl_buffer[input_index] =
				td16_rpl_buffer[input_index + recording_chunk];
		elapsed_time2 = LEGACY_U16_WRAP_SUB(
			elapsed_time2, recording_chunk);
		gameconfig.game_recordedframes = LEGACY_U16_WRAP_SUB(
			gameconfig.game_recordedframes, recording_chunk);
		elapsed_time1 = LEGACY_U16_WRAP_ADD(
			elapsed_time1, recording_chunk);
		state.game_frame = LEGACY_S16_WRAP_SUB(
			state.game_frame, recording_chunk);
	}

	td16_rpl_buffer[elapsed_time2] = (legacy_u8)input_flags;
	elapsed_time2++;
	gameconfig.game_recordedframes++;
}

void audio_driver_timer(void)
{
	legacy_u8* timer;
	legacy_u32 accumulator;
	legacy_u16 volume_accumulator;
	legacy_u16 pitch;
	legacy_u16 index;
	legacy_u8 volume;
	legacy_u8 secondary_volume;
	legacy_s16 channel;

	if (dos_data_stack_segments_match() == 0)
		return;

	word_3EB2A = LEGACY_S16_WRAP_ADD(word_3EB2A, 1);
	if (word_3EB2A < 2 && dos_audio_uses_direct_channels != 0)
		return;

	for (index = 0; index < 25U; index++) {
		timer = audio_timers + index * 0x4CU;
		if (timer[0] == 0 || audio_effects_enabled == 0)
			continue;

		volume_accumulator = LEGACY_U16_WRAP_ADD(
			(legacy_u16)((legacy_u16)timer[0x0AU] << 4),
			LEGACY_U16_WRAP_MUL(
				LEGACY_READ_U16_LE(timer + 4U), 7U));
		volume_accumulator >>= 3;
		LEGACY_WRITE_U16_LE(timer + 4U, volume_accumulator);
		volume = (legacy_u8)(volume_accumulator >> 4);
		if (volume != timer[0x0EU] || timer[0x1AU] != 0) {
			channel = LEGACY_S16_FROM_BITS(
				LEGACY_READ_U16_LE(timer + 2U));
			dos_audio_set_channel_volume(channel, volume);
			secondary_volume = volume >= 10U ?
				(legacy_u8)(volume - 10U) : 0;
			channel = LEGACY_S16_FROM_BITS(
				LEGACY_READ_U16_LE(timer + 0x14U));
			if (channel != -1)
				dos_audio_set_channel_volume(channel, secondary_volume);
			channel = LEGACY_S16_FROM_BITS(
				LEGACY_READ_U16_LE(timer + 0x16U));
			if (channel != -1)
				dos_audio_set_channel_volume(channel, secondary_volume);
			timer[0x0EU] = volume;
		}

		accumulator = (legacy_u32)LEGACY_READ_U16_LE(timer + 6U) |
			((legacy_u32)LEGACY_READ_U16_LE(timer + 8U) << 16);
		accumulator = accumulator * 7UL +
			((legacy_u32)LEGACY_READ_U16_LE(timer + 0x0CU) << 4);
		accumulator >>= 3;
		LEGACY_WRITE_U16_LE(timer + 6U, (legacy_u16)accumulator);
		LEGACY_WRITE_U16_LE(timer + 8U,
			(legacy_u16)(accumulator >> 16));
		pitch = (legacy_u16)(accumulator >> 4);
		if (pitch != LEGACY_READ_U16_LE(timer + 0x10U) ||
			timer[0x1AU] != 0) {
			channel = LEGACY_S16_FROM_BITS(
				LEGACY_READ_U16_LE(timer + 0x12U));
			if (channel != -1) {
				dos_audio_set_context_pitch(channel, pitch);
				LEGACY_WRITE_U16_LE(timer + 0x10U, pitch);
			}
		}

		timer[0x1AU] = 0;
		if (timer[0x1BU] != 0) {
			channel = LEGACY_S16_FROM_BITS(
				LEGACY_READ_U16_LE(timer + 0x14U));
			if (timer[1] != 0) {
				audio_init_chunk2(channel);
				timer[0x1BU] = 0;
			} else if (sub_3771E(channel) != 0) {
				audio_op_unk(index);
				timer[0x1BU] = 0;
			}
		}
	}

	if (word_3EB2A >= 2)
		word_3EB2A = 0;
}

void audio_unload(void)
{
	audio_fade_out(2);
	mmgr_free(songfileptr);
	mmgr_free(voicefileptr);
	is_audioloaded = 0;
}

void audio_enable_flag2(void)
{
	audio_music_enabled = 1;
}

void audio_disable_flag2(void)
{
	audio_music_enabled = 0;
	audio_update_lock = 1;
	if (byte_44290 != 0)
		audio_release_channel_range(
			0, (legacy_u16)byte_44290 - 1U);
	audio_update_driver_contexts();
	audio_update_lock = 0;
}

legacy_s16 audio_toggle_flag2(void)
{
	if (audio_music_enabled == 1) {
		audio_disable_flag2();
		return 0;
	}

	audio_enable_flag2();
	return 1;
}

legacy_s16 nopsub_373FE(void)
{
	legacy_u16 offset;
	legacy_u16 channel;

	if (byte_40630 == 1 || audio_music_enabled == 0)
		return 1;

	for (channel = 0; channel < (legacy_u16)byte_44290; channel++) {
		offset = (channel + 0x10U) * 0x4CU;
		if ((LEGACY_READ_U16_LE(audio_channels + offset) |
			LEGACY_READ_U16_LE(audio_channels + offset + 2)) != 0)
			return 0;
	}

	return 1;
}

void sub_3736A(void)
{
	audio_update_lock = 1;
	byte_40632 = 0;
	audio_release_channel_range(0, 0x0F);
	audio_init_chunk(0, 0x0F, 0, 0, byte_45950, 0);
	byte_44290 = 0;
	audio_update_driver_contexts();
	audio_update_lock = 0;
}

void audio_enable_flag6(void)
{
	legacy_s16 channel;

	if (audio_effects_enabled == 1)
		return;

	for (channel = 0x10; channel < 0x18; channel++)
		dos_audio_set_channel_volume(channel, byte_428D6[channel]);
	audio_effects_enabled = 1;
}

void audio_disable_flag6(void)
{
	legacy_s16 channel;

	if (audio_effects_enabled == 0)
		return;

	for (channel = 0x10; channel < 0x18; channel++) {
		byte_428D6[channel] =
			audio_sfx_channels[(channel - 0x10) * 0x4C + 0x28];
		dos_audio_set_channel_volume(channel, 0);
	}
	audio_effects_enabled = 0;
}

legacy_s16 audio_toggle_flag6(void)
{
	if (audio_effects_enabled == 1) {
		audio_disable_flag6();
		return 0;
	}

	audio_enable_flag6();
	return 1;
}

legacy_s16 sub_3771E(legacy_s16 channel)
{
	legacy_u16 offset;

	if (audio_effects_enabled == 0 || channel < 0x10 || channel > 0x17)
		return 1;

	offset = (legacy_u16)channel * 0x4CU;
	return (LEGACY_READ_U16_LE(audio_channels + offset) |
		LEGACY_READ_U16_LE(audio_channels + offset + 2)) == 0;
}

void nopsub_37750(legacy_s16 channel, void far* value)
{
	void far* *field;

	field = (void far* *)(audio_channels +
		(legacy_u16)channel * 0x4CU + 0x48U);
	*field = value;
}

legacy_u32 audioresource_get_dword(const legacy_u8 far* source)
{
	return (legacy_u32)source[0] |
		((legacy_u32)source[1] << 8) |
		((legacy_u32)source[2] << 16) |
		((legacy_u32)source[3] << 24);
}

legacy_u16 audioresource_get_word(const legacy_u8 far* source)
{
	return (legacy_u16)((legacy_u16)source[0] |
		((legacy_u16)source[1] << 8));
}

void audioresource_copy_4_bytes(legacy_u8 far* destination,
	const legacy_u8 far* source)
{
	destination[0] = source[0];
	destination[1] = source[1];
	destination[2] = source[2];
	destination[3] = source[3];
}

void audio_init_chunk(legacy_s16 first_channel, legacy_s16 last_channel,
	void far* resource, legacy_u16 resource_data_offset,
	legacy_u16 rate, legacy_u8 priority)
{
	const legacy_u8 far* resource_data;
	legacy_u8* chunk;
	legacy_u32 pointer_value;
	legacy_s16 channel;
	legacy_s16 last;
	legacy_u16 chunk_offset;
	legacy_u16 pointer_offset;
	legacy_u16 pointer_segment;
	legacy_u16 resource_offset;
	legacy_u16 resource_segment;

	channel = (legacy_s16)first_channel;
	last = (legacy_s16)last_channel;
	if (channel > last)
		return;

	chunk_offset = LEGACY_U16_WRAP_MUL(channel, 0x4CU);
	resource_offset = (legacy_u16)FP_OFF(resource);
	resource_segment = (legacy_u16)FP_SEG(resource);
	do {
		chunk = audio_channels + chunk_offset;
		LEGACY_WRITE_U16_LE(chunk + 0x48U, 0);
		LEGACY_WRITE_U16_LE(chunk + 0x4AU, 0);
		chunk[0x22] = 0x7F;
		chunk[0x23] = (legacy_u8)channel;
		chunk[0x16] = 0x0F;
		byte_44D06[(legacy_u16)channel] = 0;
		byte_44ACA[(legacy_u16)channel] = 0;
		chunk[0x32] = 0;
		chunk[4] = 0;
		chunk[0x24] = priority;
		chunk[0x15] = 0;
		LEGACY_WRITE_U16_LE(chunk + 0x1AU, 0);
		LEGACY_WRITE_U16_LE(chunk + 0x18U, 0);
		chunk[0x1C] = 0;
		LEGACY_WRITE_U16_LE(chunk + 0x20U, 0);
		LEGACY_WRITE_U16_LE(chunk + 0x1EU, 0);
		chunk[0x28] = (legacy_u8)rate;
		chunk[0x25] = 0;
		LEGACY_WRITE_U16_LE(chunk + 0x26U, 0);
		chunk[0x29] = 0;
		chunk[0x2A] = 0;
		chunk[0x2B] = 0;
		chunk[0x2C] = 0;
		chunk[0x47] = 0xFF;

		if (resource != 0) {
			resource_data = (const legacy_u8 far*)MK_FP(
				resource_segment,
				LEGACY_U16_WRAP_ADD(resource_offset,
					resource_data_offset));
			pointer_value = audioresource_get_dword(resource_data);
			pointer_offset = LEGACY_U16_WRAP_ADD(
				(legacy_u16)pointer_value, 4U);
			pointer_segment = (legacy_u16)(pointer_value >> 16);
			LEGACY_WRITE_U16_LE(chunk + 5U, pointer_offset);
			LEGACY_WRITE_U16_LE(chunk + 7U, pointer_segment);
			pointer_value = audioresource_get_dword(resource_data);
			pointer_offset = LEGACY_U16_WRAP_ADD(
				(legacy_u16)pointer_value, 4U);
			pointer_segment = (legacy_u16)(pointer_value >> 16);
			LEGACY_WRITE_U16_LE(chunk, pointer_offset);
			LEGACY_WRITE_U16_LE(chunk + 2U, pointer_segment);
			resource_data_offset = LEGACY_U16_WRAP_ADD(
				resource_data_offset, 5U);
			LEGACY_WRITE_U16_LE(chunk + 0x2EU,
				LEGACY_U16_WRAP_ADD(resource_offset, 7U));
			LEGACY_WRITE_U16_LE(chunk + 0x30U, resource_segment);
		} else {
			LEGACY_WRITE_U16_LE(chunk, 0);
			LEGACY_WRITE_U16_LE(chunk + 2U, 0);
		}

		chunk_offset = LEGACY_U16_WRAP_ADD(chunk_offset, 0x4CU);
		channel = LEGACY_S16_WRAP_ADD(channel, 1);
	} while (channel <= last);
}

void audio_reset_channels(void)
{
	legacy_u8* context;
	legacy_u16 context_index;

	audio_update_lock = 1;
	audio_init_chunk(0, 0x17, 0, 0, 0x7FU, 0);
	context = dos_audio_contexts;
	for (context_index = 0; context_index < dos_audio_context_count;
		context_index++) {
		dos_audio_driver_release_channel((legacy_s16)context_index);
		context[0] = 0xFFU;
		context[1] = 0;
		context[2] = 0;
		LEGACY_WRITE_U16_LE(context + 0x10U, 0);
		LEGACY_WRITE_U16_LE(context + 0x12U, 0);
		context[0x2CU] = 0xFFU;
		context += 0x2EU;
	}
	dos_audio_driver_reset();
	dos_audio_driver_start();
	audio_update_lock = 0;
}

static void audio_clear_driver_context(legacy_u8* context)
{
	context[0] = 0xFFU;
	context[1] = 0;
	context[2] = 0;
	LEGACY_WRITE_U16_LE(context + 0x10U, 0);
	LEGACY_WRITE_U16_LE(context + 0x12U, 0);
}

void audio_release_channel_range(legacy_s16 first_channel,
	legacy_s16 last_channel)
{
	legacy_u8* chunk;
	legacy_u8* context;
	legacy_u16 context_index;
	legacy_u16 channel_bits;
	legacy_s16 channel;

	if (dos_audio_uses_direct_channels == 0) {
		context = dos_audio_contexts;
		for (context_index = 0; context_index < dos_audio_context_count;
			context_index++) {
			channel_bits = context[0];
			if (channel_bits >= (legacy_u16)first_channel &&
				channel_bits <= (legacy_u16)last_channel) {
				dos_audio_driver_release_channel(
					(legacy_s16)context_index);
				audio_clear_driver_context(context);
			}
			context += 0x2EU;
		}
	} else {
		channel = first_channel;
		while (channel <= last_channel) {
			chunk = audio_channels + LEGACY_U16_WRAP_MUL(
				(legacy_u16)channel, 0x4CU);
			if (chunk[0x47U] < 0x10U)
				dos_audio_driver_release_channel(chunk[0x47U]);

			context = dos_audio_contexts;
			for (context_index = 0; context_index < 0x10U;
				context_index++) {
				if ((legacy_u16)context[0] == (legacy_u16)channel)
					audio_clear_driver_context(context);
				context += 0x2EU;
			}
			channel = LEGACY_S16_WRAP_ADD(channel, 1);
		}
	}

	channel = first_channel;
	while (channel <= last_channel) {
		chunk = audio_channels + LEGACY_U16_WRAP_MUL(
			(legacy_u16)channel, 0x4CU);
		chunk[0x15U] = 0;
		channel = LEGACY_S16_WRAP_ADD(channel, 1);
	}
}

static legacy_u16 audio_far_read_u16(const legacy_u8 far* source)
{
	return (legacy_u16)((legacy_u16)source[0] |
		((legacy_u16)source[1] << 8));
}

static legacy_u32 audio_context_read_u32(const legacy_u8* context,
	legacy_u16 offset)
{
	return (legacy_u32)LEGACY_READ_U16_LE(context + offset) |
		((legacy_u32)LEGACY_READ_U16_LE(context + offset + 2U) << 16);
}

static void audio_context_write_u32(legacy_u8* context, legacy_u16 offset,
	legacy_u32 value)
{
	LEGACY_WRITE_U16_LE(context + offset, (legacy_u16)value);
	LEGACY_WRITE_U16_LE(context + offset + 2U,
		(legacy_u16)(value >> 16));
}

static void far* audio_select_sample_resource(void far* original_resource,
	legacy_u8 note)
{
	static const legacy_u8 percussion_resource_indices[16] = {
		0U, 2U, 1U, 2U, 2U, 2U, 5U, 2U,
		6U, 2U, 6U, 2U, 2U, 3U, 2U, 4U
	};
	void far* resources[7];
	legacy_u16 note_index;

	if (((legacy_u8 far*)original_resource)[5] != 5U)
		return original_resource;
	resources[0] = basdres;
	resources[1] = snarres;
	resources[2] = tommres;
	resources[3] = rideres;
	resources[4] = crshres;
	resources[5] = chhtres;
	resources[6] = ohhtres;
	note_index = LEGACY_U16_WRAP_SUB(note, 0x18U);
	if (note_index >= 16U)
		return tommres;
	return resources[percussion_resource_indices[note_index]];
}

static legacy_s16 audio_find_driver_context(legacy_u8 far* resource,
	legacy_u8* timer)
{
	legacy_u8* context;
	legacy_u8* old_timer;
	legacy_u32 oldest_state1_age;
	legacy_u32 oldest_state2_age;
	legacy_u32 age;
	legacy_u16 resource_mask;
	legacy_u16 context_mask;
	legacy_u16 context_index;
	legacy_u16 context_count;
	legacy_s16 oldest_state1;
	legacy_s16 oldest_state2;
	legacy_s16 selected;
	legacy_s16 restrict_to_timer;

	resource_mask = audio_far_read_u16(resource + 0x0CU);
	if (resource_mask == 0)
		return -1;
	oldest_state1 = -1;
	oldest_state2 = -1;
	oldest_state1_age = 0;
	oldest_state2_age = 0;

	if (dos_audio_uses_direct_channels != 0) {
		context_count = 0x10U;
		restrict_to_timer = 0;
	} else {
		context_count = dos_audio_context_count;
		restrict_to_timer = timer[0x15U] < timer[0x16U];
	}

	context = dos_audio_contexts;
	for (context_index = 0; context_index < context_count;
		context_index++) {
		if (dos_audio_uses_direct_channels == 0) {
			context_mask = context_index < 16U ?
				(legacy_u16)(1U << context_index) : 0;
			if ((resource_mask & context_mask) == 0 ||
				(restrict_to_timer != 0 &&
					timer[0x23U] != context[0])) {
				context += 0x2EU;
				continue;
			}
		}

		if (context[1] == 0) {
			if (dos_audio_uses_direct_channels == 0)
				timer[0x15U]++;
			return (legacy_s16)context_index;
		}
		if (dos_audio_uses_direct_channels == 0 && timer[0x24U] < context[2]) {
			context += 0x2EU;
			continue;
		}

		age = audio_context_read_u32(context, 8U);
		if (context[1] == 1U && age > oldest_state1_age) {
			oldest_state1_age = age;
			oldest_state1 = (legacy_s16)context_index;
		}
		if (context[1] == 2U && age > oldest_state2_age) {
			oldest_state2_age = age;
			oldest_state2 = (legacy_s16)context_index;
		}
		context += 0x2EU;
	}

	selected = oldest_state2 != -1 ? oldest_state2 : oldest_state1;
	if (selected == -1)
		return -1;
	context = dos_audio_contexts +
		LEGACY_U16_WRAP_MUL((legacy_u16)selected, 0x2EU);
	if (dos_audio_uses_direct_channels == 0 && restrict_to_timer == 0) {
		old_timer = audio_timers + LEGACY_U16_WRAP_SUB(
			LEGACY_READ_U16_LE(context + 0x2AU),
			FP_OFF(audio_timers));
		if (old_timer != timer) {
			old_timer[0x15U]--;
			timer[0x15U]++;
		}
	}
	dos_audio_driver_start_context(context[0x2CU], context);
	dos_audio_driver_end_context(context[0x2CU], context);
	return selected;
}

legacy_s16 audio_start_sample(legacy_u16 value, legacy_s16 handle)
{
	legacy_u8* timer;
	legacy_u8* context;
	legacy_u8 far* resource;
	legacy_u16 timer_offset;
	legacy_u16 context_offset;
	legacy_u8 note;
	legacy_s16 driver_channel;
	legacy_s16 pitch;
	legacy_s16 context_index;

	timer_offset = LEGACY_U16_WRAP_MUL((legacy_u16)handle, 0x4CU);
	timer = audio_timers + timer_offset;
	resource = (legacy_u8 far*)audio_read_far_pointer(timer + 0x1EU);
	note = 0xFFU;
	resource = (legacy_u8 far*)audio_select_sample_resource(resource, note);
	if (resource == 0)
		return -1;

	context_index = audio_find_driver_context(resource, timer);
	if (context_index == -1)
		return -1;
	context_offset = LEGACY_U16_WRAP_MUL(
		(legacy_u16)context_index, 0x2EU);
	context = dos_audio_contexts + context_offset;
	if (LEGACY_READ_U16_LE(context + 0x10U) != FP_OFF(resource) ||
		LEGACY_READ_U16_LE(context + 0x12U) != FP_SEG(resource)) {
		audio_write_far_pointer(context + 0x10U, resource);
		if (dos_audio_uses_direct_channels == 0)
			dos_audio_driver_prepare_context(context_index,
				context, timer, resource);
	}

	context[0] = (legacy_u8)handle;
	context[1] = 1U;
	context[2] = timer[0x24U];
	LEGACY_WRITE_U16_LE(context + 8U, 0);
	LEGACY_WRITE_U16_LE(context + 0x0AU, 0);
	audio_context_write_u32(context, 0x0CU, 0xFFFFFFDFUL);
	LEGACY_WRITE_U16_LE(context + 0x14U,
		audio_far_read_u16(resource + 0x1CU));
	context[0x16U] = 1U;
	LEGACY_WRITE_U16_LE(context + 0x18U,
		audio_far_read_u16(resource + 0x2AU));
	LEGACY_WRITE_U16_LE(context + 0x1AU,
		audio_far_read_u16(resource + 0x2CU));
	LEGACY_WRITE_U16_LE(context + 0x1CU, 0);
	LEGACY_WRITE_U16_LE(context + 0x1EU,
		audio_far_read_u16(resource + 0x36U));
	LEGACY_WRITE_U16_LE(context + 0x20U,
		audio_far_read_u16(resource + 0x38U));
	context[0x22U] = 0;
	LEGACY_WRITE_U16_LE(context + 0x24U,
		audio_far_read_u16(resource + 0x30U));
	context[0x26U] = resource[0x34U];
	context[0x27U] = 0;
	context[0x28U] = 0;
	context[0x29U] = 0;
	LEGACY_WRITE_U16_LE(context + 0x2AU, FP_OFF(timer));
	driver_channel = dos_audio_uses_direct_channels == 0 ? context_index : timer[0x47U];
	context[0x2CU] = (legacy_u8)driver_channel;

	if (note == 0xFFU) {
		dos_audio_driver_set_context_value(driver_channel, context, value);
		if (dos_audio_uses_direct_channels != 0)
			note = 0x3CU;
	}
	pitch = LEGACY_S16_WRAP_ADD(
		(legacy_s8)resource[0x10U], (legacy_s8)note);
	dos_audio_driver_activate_context(driver_channel, context, timer,
		pitch, 0, resource);
	byte_44ACA[(legacy_u16)handle] = note;
	return context_index;
}

static void audio_advance_driver_context(legacy_u8* context)
{
	legacy_u8* chunk;
	legacy_u32 value;

	value = LEGACY_U32_WRAP_ADD(audio_context_read_u32(context, 8U), 1UL);
	audio_context_write_u32(context, 8U, value);
	value = audio_context_read_u32(context, 0x0CU);
	if (value != 0) {
		audio_context_write_u32(context, 0x0CU,
			LEGACY_U32_WRAP_SUB(value, 1UL));
		return;
	}

	dos_audio_driver_start_context(context[0x2CU], context);
	context[1] = 2U;
	chunk = audio_channels + LEGACY_U16_WRAP_MUL(context[0], 0x4CU);
	context[0x16U] = chunk[0x15U] != 0 ? 3U : 4U;
}

static legacy_u16 audio_absolute_word(legacy_s16 value)
{
	if (value < 0)
		return (legacy_u16)LEGACY_S16_WRAP_NEGATE(value);
	return (legacy_u16)value;
}

void audio_update_driver_contexts(void)
{
	legacy_u8* context;
	legacy_u8* chunk;
	legacy_u8 far* resource;
	legacy_s16 level;
	legacy_s16 modulation;
	legacy_u16 magnitude;
	legacy_u16 threshold;
	legacy_u16 value;
	legacy_u16 context_index;
	legacy_u8 sequence_index;

	context = dos_audio_contexts;
	for (context_index = 0; context_index < dos_audio_context_count;
		context_index++) {
		if (context[1] == 0) {
			context += 0x2EU;
			continue;
		}
		if (context[0] > 0x0FU)
			audio_advance_driver_context(context);

		resource = (legacy_u8 far*)
			audio_read_far_pointer(context + 0x10U);
		if (context[0x16U] == 1U) {
			level = LEGACY_S16_WRAP_ADD(
				LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(
					context + 0x14U)),
				LEGACY_S16_FROM_BITS(audio_far_read_u16(
					resource + 0x20U)));
			LEGACY_WRITE_U16_LE(context + 0x14U, level);
			value = audio_far_read_u16(resource + 0x1EU);
			if (level >= LEGACY_S16_FROM_BITS(value)) {
				LEGACY_WRITE_U16_LE(context + 0x14U, value);
				context[0x16U] =
					LEGACY_S16_FROM_BITS(audio_far_read_u16(
						resource + 0x24U)) >=
					LEGACY_S16_FROM_BITS(value) ? 3U : 2U;
			}
		}
		if (context[0x16U] == 2U) {
			level = LEGACY_S16_WRAP_SUB(
				LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(
					context + 0x14U)),
				LEGACY_S16_FROM_BITS(audio_far_read_u16(
					resource + 0x22U)));
			LEGACY_WRITE_U16_LE(context + 0x14U, level);
			value = audio_far_read_u16(resource + 0x24U);
			if (level <= LEGACY_S16_FROM_BITS(value)) {
				context[0x16U] = 3U;
				LEGACY_WRITE_U16_LE(context + 0x14U, value);
			}
		}
		if (context[0x16U] == 3U &&
			audio_far_read_u16(resource + 0x24U) == 0)
			context[0x16U] = 4U;
		if (context[0x16U] == 4U) {
			level = LEGACY_S16_WRAP_SUB(
				LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(
					context + 0x14U)),
				LEGACY_S16_FROM_BITS(audio_far_read_u16(
					resource + 0x26U)));
			LEGACY_WRITE_U16_LE(context + 0x14U, level);
			if (level <= 0) {
				LEGACY_WRITE_U16_LE(context + 0x14U, 0);
				context[0x16U] = 0;
				context[1] = 0;
				chunk = audio_channels + LEGACY_U16_WRAP_MUL(
					context[0], 0x4CU);
				chunk[0x15U]--;
				dos_audio_driver_end_context(context[0x2CU], context);
				byte_44ACA[context[0]] = 0;
			}
		}

		if (resource[0x28U] != 0) {
			value = LEGACY_READ_U16_LE(context + 0x18U);
			if (value != 0) {
				LEGACY_WRITE_U16_LE(context + 0x18U,
					LEGACY_U16_WRAP_SUB(value, 1U));
			} else {
				value = LEGACY_READ_U16_LE(context + 0x1AU);
				if (value != 0) {
					if (value != 0x7FFFU)
						LEGACY_WRITE_U16_LE(context + 0x1AU,
							LEGACY_U16_WRAP_SUB(value, 1U));
					if (context[0x27U] != 0) {
						context[0x27U]--;
					} else {
						context[0x27U] = resource[0x29U];
						modulation = LEGACY_S16_FROM_BITS(
							LEGACY_READ_U16_LE(context + 0x1CU));
						if (context[0x26U] == 2U)
							modulation = LEGACY_S16_WRAP_SUB(modulation,
								LEGACY_READ_U16_LE(context + 0x24U));
						else
							modulation = LEGACY_S16_WRAP_ADD(modulation,
								LEGACY_READ_U16_LE(context + 0x24U));
						LEGACY_WRITE_U16_LE(context + 0x1CU, modulation);
						magnitude = audio_absolute_word(modulation);
						threshold = audio_far_read_u16(resource + 0x2EU);
						if (magnitude >= threshold) {
							if (context[0x26U] == 2U &&
								(resource[0x34U] & 1U) != 0)
								context[0x26U] = 1U;
							else if (context[0x26U] != 2U &&
								(resource[0x34U] & 2U) != 0)
								context[0x26U] = 2U;
							else
								LEGACY_WRITE_U16_LE(context + 0x1CU, 0);
						}
					}
				}
			}
		}

		if (resource[0x35U] != 0) {
			value = LEGACY_READ_U16_LE(context + 0x1EU);
			if (value != 0) {
				LEGACY_WRITE_U16_LE(context + 0x1EU,
					LEGACY_U16_WRAP_SUB(value, 1U));
			} else {
				value = LEGACY_READ_U16_LE(context + 0x20U);
				if (value != 0) {
					LEGACY_WRITE_U16_LE(context + 0x20U,
						LEGACY_U16_WRAP_SUB(value, 1U));
					if (context[0x28U] != 0) {
						context[0x28U]--;
					} else {
						context[0x28U] = resource[0x3AU];
						sequence_index = context[0x29U]++;
						context[0x22U] =
							resource[0x3BU + (sequence_index & 7U)];
					}
				}
			}
		}

		dos_audio_driver_suspend_context(context[0x2CU], context,
			LEGACY_READ_U16_LE(context + 0x2AU), resource);
		context += 0x2EU;
	}
	dos_audio_driver_suspend_all(dos_audio_contexts);
}

void audio_suspend(void)
{
	legacy_u8* chunk;
	legacy_u8* context;
	legacy_u16 channel;
	legacy_u16 context_index;

	byte_40630 = 1;
	audio_update_lock = 1;
	if (dos_audio_uses_direct_channels != 0) {
		dos_audio_master_volume = 0;
		dos_audio_driver_set_master_state(4, (void far*)dos_audio_master_state);
		audio_update_lock = 0;
		return;
	}

	chunk = audio_channels;
	for (channel = 0; channel < 0x18U; channel++) {
		if (audio_effects_enabled == 1 || channel < 0x10U) {
			byte_428BE[channel] = chunk[0x28U];
			dos_audio_set_channel_volume((legacy_s16)channel, 0);
		}
		chunk += 0x4CU;
	}

	context = dos_audio_contexts;
	for (context_index = 0; context_index < 0x10U;
		context_index++) {
		dos_audio_driver_suspend_context(context[0x2CU], context,
			LEGACY_READ_U16_LE(context + 0x2AU),
			audio_read_far_pointer(context + 0x10U));
		context += 0x2EU;
	}
	dos_audio_driver_suspend_all(dos_audio_contexts);
	audio_update_lock = 0;
}

void audio_resume(void)
{
	legacy_u16 channel;

	byte_40630 = 1;
	audio_update_lock = 1;
	if (dos_audio_uses_direct_channels != 0) {
		dos_audio_master_volume = 0x64U;
		dos_audio_driver_set_master_state(4, (void far*)dos_audio_master_state);
	} else {
		for (channel = 0; channel < 0x18U; channel++) {
			if (audio_effects_enabled == 1 || channel < 0x10U)
				dos_audio_set_channel_volume((legacy_s16)channel,
					byte_428BE[channel]);
		}
	}
	audio_update_lock = 0;
	byte_40630 = 0;
}

legacy_s16 audio_check_flag(void far* resource, legacy_s16 channel,
	legacy_u8 priority, legacy_u16 rate)
{
	const legacy_u8 far* bytes;
	legacy_u16 scaled_rate;
	legacy_u16 offset;
	legacy_u16 resource_data_offset;
	legacy_s16 candidate;

	bytes = (const legacy_u8 far*)resource;
	if (audio_effects_enabled == 0 || resource == 0 || bytes[5] != 1)
		return -1;

	if (byte_45948 != 0) {
		scaled_rate = (legacy_u16)((legacy_u32)(legacy_u16)rate *
			0x80UL);
		rate = (legacy_u16)((legacy_u16)(scaled_rate /
			(legacy_u16)byte_45948) - 1U);
	} else {
		rate = 0;
	}

	if (channel == -1) {
		for (candidate = 0x10; candidate <= 0x17; candidate++) {
			offset = (legacy_u16)(candidate - 0x10) * 0x4CU;
			if ((LEGACY_READ_U16_LE(audio_sfx_channels + offset) |
				LEGACY_READ_U16_LE(audio_sfx_channels + offset + 2)) == 0 &&
				byte_45D9A[candidate] == 0) {
				channel = candidate;
				break;
			}
		}
	}

	if (channel == -1)
		return -1;

	resource_data_offset = (legacy_u16)bytes[6] * 4U + 8U;
	audio_init_chunk(channel, channel, resource, resource_data_offset,
		rate, priority);
	return channel;
}

legacy_s16 audio_check_flag2(void far* resource, legacy_s16 channel,
	legacy_u8 priority)
{
	return audio_check_flag(resource, channel, priority,
		(legacy_u16)byte_45948);
}

legacy_s16 nopsub_37456(void far* resource)
{
	return audio_check_flag2(resource, -1, 0x40U);
}

legacy_s16 sub_37470(legacy_s16 channel, legacy_u8 priority)
{
	legacy_u16 offset;
	legacy_s16 candidate;

	if (channel == -1) {
		for (candidate = 0x10; candidate <= 0x17; candidate++) {
			offset = (legacy_u16)(candidate - 0x10) * 0x4CU;
			if ((LEGACY_READ_U16_LE(audio_sfx_channels + offset) |
				LEGACY_READ_U16_LE(audio_sfx_channels + offset + 2)) == 0 &&
				byte_45D9A[candidate] == 0) {
				channel = candidate;
				break;
			}
		}
	}

	if (channel != -1) {
		byte_45D9A[channel] = 1;
		offset = (legacy_u16)channel * 0x4CU;
		audio_channels[offset + 0x24U] = priority;
	}

	return channel;
}

void audio_init_chunk2(legacy_s16 channel)
{
	legacy_u16 offset;

	if (channel < 0x10 || channel > 0x17)
		return;

	offset = (legacy_u16)channel * 0x4CU;
	LEGACY_WRITE_U16_LE(audio_channels + offset, 0);
	LEGACY_WRITE_U16_LE(audio_channels + offset + 2, 0);
	audio_release_channel_range(channel, channel);
	audio_init_chunk(channel, channel, 0, 0, byte_45948, 0);
}

void audio_op_unk7(legacy_s16 index)
{
	legacy_u16 offset;
	legacy_s16 channel;

	offset = LEGACY_U16_WRAP_MUL(index, 0x4CU);
	channel = LEGACY_S16_FROM_BITS(
		LEGACY_READ_U16_LE(audio_timers + offset + 0x16U));
	audio_init_chunk2(channel);
	LEGACY_WRITE_U16_LE(audio_timers + offset + 0x16U, 0xFFFFU);
}

legacy_s16 nopsub_27489(legacy_s16 index)
{
	legacy_u16 offset;
	legacy_s16 channel;

	offset = LEGACY_U16_WRAP_MUL(index, 0x4CU);
	channel = LEGACY_S16_FROM_BITS(
		LEGACY_READ_U16_LE(audio_timers + offset + 0x14U));
	if (channel < 0)
		return 1;

	return sub_3771E(channel);
}

void audio_function2(legacy_s16 index)
{
	legacy_u16 offset;
	legacy_s16 channel;

	offset = LEGACY_U16_WRAP_MUL(index, 0x4CU);
	if (audio_timers[offset] != 1 || audio_timers[offset + 1U] != 1)
		return;

	channel = LEGACY_S16_FROM_BITS(
		LEGACY_READ_U16_LE(audio_timers + offset + 0x12U));
	sub_38156(channel);
	LEGACY_WRITE_U16_LE(audio_timers + offset + 0x12U, 0xFFFFU);
	audio_timers[offset + 1U] = 0;
	audio_timers[offset + 0x1AU] = 1;
}

static legacy_s16 audio_start_indexed_event(legacy_s16 index,
	legacy_u16 resource_field, legacy_u8 priority)
{
	legacy_u16 offset;
	legacy_u16 rate;
	legacy_s16 channel;
	void far* resource;

	offset = LEGACY_U16_WRAP_MUL(index, 0x4CU);
	rate = LEGACY_READ_U16_LE(audio_timers + offset + 4U) >> 4;
	resource = audio_read_far_pointer(
		audio_timers + offset + resource_field);
	channel = audio_check_flag(resource, -1, priority, rate);
	LEGACY_WRITE_U16_LE(audio_timers + offset + 0x14U, channel);
	audio_timers[offset + 0x1AU] = 1;
	return channel;
}

void nopsub_27220(legacy_s16 index)
{
	legacy_u16 offset;
	legacy_u16 rate;
	legacy_s16 channel;
	void far* resource;

	offset = LEGACY_U16_WRAP_MUL(index, 0x4CU);
	rate = LEGACY_READ_U16_LE(audio_timers + offset + 4U) >> 4;
	resource = audio_read_far_pointer(audio_timers + offset + 0x2CU);
	channel = audio_check_flag(resource, -1, 0x40U, rate);
	LEGACY_WRITE_U16_LE(audio_timers + offset + 0x14U, channel);
	audio_timers[offset + 0x1AU] = 1;
	audio_timers[offset + 0x1BU] = 1;
}

static void audio_append_filename_part(legacy_s8* destination,
	const legacy_s8* source)
{
	while (*destination != 0)
		destination++;
	do {
		*destination++ = *source;
	} while (*source++ != 0);
}

static const legacy_s8* audio_find_last_backslash(const legacy_s8* text)
{
	const legacy_s8* match;

	match = 0;
	while (*text != 0) {
		if (*text == '\\')
			match = text;
		text++;
	}
	return match;
}

legacy_s8* audio_make_filename(const legacy_s8* filename, const legacy_s8* extension,
	const legacy_s8* inserted_path)
{
	const legacy_s8* basename;
	const legacy_s8* source;
	legacy_s8* separator;
	legacy_u16 length;

	separator = audio_filetemp;
	source = filename;
	do {
		*separator++ = *source;
	} while (*source++ != 0);
	separator = (legacy_s8*)audio_find_last_backslash(audio_filetemp);
	if (separator != 0)
		separator[1] = 0;
	else
		audio_filetemp[0] = 0;

	audio_append_filename_part(audio_filetemp, inserted_path);
	basename = audio_find_last_backslash(filename);
	if (basename != 0)
		basename++;
	else
		basename = filename;
	audio_append_filename_part(audio_filetemp, basename);

	length = 0;
	while (audio_filetemp[length] != 0)
		length++;
	if (length <= 4U || audio_filetemp[length - 4U] != '.') {
		audio_append_filename_part(audio_filetemp, ".");
		audio_append_filename_part(audio_filetemp, extension);
	}
	return audio_filetemp;
}

legacy_s16 audio_load_dos_driver(const legacy_s8* driver,
	legacy_s16 unused, legacy_s16 mode)
{
	static const legacy_s8 driver_extension[] = "drv";
	static const legacy_s8 empty_path[] = "";
	static const legacy_s8 mt32_bank_filename[] = "mt32.plb";
	static const legacy_s8 missing_driver_message[] =
		"Can't find driver!\n";
	void far* bank;
	legacy_u16 driver_length;
	legacy_u16 basename_offset;
	legacy_u16 scan_offset;
	legacy_u8 channel_count;

	(void)unused;
	if (mode == 0x473A)
		dos_audio_special_mode = 1;
	if (dos_audio_driver_binary != 0)
		dos_audio_shutdown();
	else
		add_exit_handler(dos_audio_shutdown);
	dos_audio_driver_binary = 0;

	driver_length = 0;
	while (driver[driver_length] != 0)
		driver_length++;
	basename_offset = 0;
	for (scan_offset = driver_length; scan_offset != 0; scan_offset--) {
		if (driver[scan_offset] == '\\' || driver[scan_offset] == ':') {
			basename_offset = LEGACY_U16_WRAP_ADD(scan_offset, 1U);
			break;
		}
	}
	audiodriverstring2[0] = driver[basename_offset];
	audiodriverstring2[1] = driver[basename_offset + 1U];
	audiodriverstring2[2] = 0;

	dos_audio_driver_binary = file_load_binary_nofatal(audio_make_filename(
		driver, driver_extension, empty_path));
	byte_45950 = 0x7FU;
	byte_45948 = 0x7FU;
	if (dos_audio_driver_binary == 0) {
		fatal_error(missing_driver_message);
		return 2;
	}

	channel_count = dos_audio_driver_initialize();
	dos_audio_context_count = channel_count;
	if (channel_count == 0 || channel_count == 0xFFU)
		return 2;
	if (channel_count > 0x7FU) {
		dos_audio_context_count = 0x10U;
		dos_audio_uses_direct_channels = 1;
		dos_audio_special_mode = 0;
	}

	audio_reset_channels();
	timer_reg_callback(audio_driver_timer);
	if (dos_audio_uses_direct_channels != 0) {
		bank = file_load_binary_nofatal(mt32_bank_filename);
		if (bank != 0) {
			dos_audio_driver_load_bank(bank);
			mmgr_release((legacy_s8 far*)bank);
			dos_audio_master_volume = 0x64U;
			dos_audio_driver_set_master_state(
				4, (void far*)dos_audio_master_state);
		}
	}

	byte_40630 = 0;
	audio_music_enabled = 1;
	byte_40632 = 0;
	audio_effects_enabled = 1;
	return 0;
}

void far* load_sfx_ge(const legacy_s8* filename, const legacy_s8* extension,
	const legacy_s8* inserted_path)
{
	legacy_s8 compressed_extension[4];
	void far* result;

	result = file_load_binary_nofatal(audio_make_filename(
		filename, extension, inserted_path));
	if (result != 0)
		return result;

	compressed_extension[0] = 'P';
	compressed_extension[1] = extension[0];
	compressed_extension[2] = extension[1];
	compressed_extension[3] = 0;
	result = file_decomp_nofatal(audio_make_filename(
		filename, compressed_extension, inserted_path));
	if (result != 0)
		return result;

	result = file_load_binary_nofatal(audio_make_filename(
		filename, extension, "ge"));
	if (result != 0)
		return result;

	result = file_decomp_nofatal(audio_make_filename(
		filename, compressed_extension, "ge"));
	if (result != 0)
		return result;

	result = file_load_binary_nofatal(audio_make_filename(
		filename, extension, ""));
	if (result != 0)
		return result;

	result = file_decomp_nofatal(audio_make_filename(
		filename, compressed_extension, ""));
	if (result != 0)
		return result;

	return file_load_binary_nofatal(filename);
}

void far* load_sfx_file(const legacy_s8* filename)
{
	void far* result;

	result = 0;
	if (dos_audio_special_mode != 0)
		result = load_sfx_ge(filename, "dsf", audiodriverstring2);
	if (result == 0)
		result = load_sfx_ge(filename, "sfx", audiodriverstring2);
	if (result == 0 && word_4063C != 0)
		fatal_error("cannot load sfx file %s", filename);
	return result;
}

void far* load_song_file(const legacy_s8* filename)
{
	void far* result;

	result = load_sfx_ge(filename, "kms", audiodriverstring2);
	if (result == 0 && word_4063C != 0)
		fatal_error("cannot load song file %s", filename);
	return result;
}

void far* load_voice_file(const legacy_s8* filename)
{
	void far* result;

	result = 0;
	if (dos_audio_special_mode != 0)
		result = load_sfx_ge(filename, "dvc", audiodriverstring2);
	if (result == 0)
		result = load_sfx_ge(filename, "vce", audiodriverstring2);
	if (result == 0 && word_4063C != 0)
		fatal_error("cannot load voice file %s", filename);
	return result;
}

legacy_s16 audioresource_compare_chunknames(legacy_s16 case_sensitive,
	const legacy_s8 far* first_name, const legacy_s8 far* second_name, legacy_s16 count)
{
	legacy_u16 first_offset;
	legacy_u16 first_segment;
	legacy_u16 second_offset;
	legacy_u16 second_segment;
	legacy_u16 remaining;
	legacy_u8 first;
	legacy_u8 second;

	remaining = (legacy_u16)count;
	if (remaining == 0)
		return 1;
	first_offset = (legacy_u16)FP_OFF(first_name);
	first_segment = (legacy_u16)FP_SEG(first_name);
	second_offset = (legacy_u16)FP_OFF(second_name);
	second_segment = (legacy_u16)FP_SEG(second_name);
	do {
		first = *(const legacy_u8 far*)MK_FP(
			first_segment, first_offset);
		second = *(const legacy_u8 far*)MK_FP(
			second_segment, second_offset);
		if (first == 0 || second == 0)
			return 1;
		if (case_sensitive != 0) {
			if (first != second)
				return 0;
		} else if (toupper(second) != toupper(first)) {
			return 0;
		}
		first_offset = LEGACY_U16_WRAP_ADD(first_offset, 1U);
		second_offset = LEGACY_U16_WRAP_ADD(second_offset, 1U);
		remaining--;
	} while (remaining != 0);
	return 1;
}

void sub_3702E(legacy_s16 left, legacy_s16 top, legacy_s16 right, legacy_s16 bottom, legacy_s16 color)
{
	legacy_s16 x;
	legacy_s16 y;
	legacy_s16 width;
	legacy_s16 height;

	x = LEGACY_S16_FROM_BITS(left);
	y = LEGACY_S16_FROM_BITS(top);
	width = LEGACY_S16_WRAP_ADD(
		LEGACY_S16_WRAP_SUB(right, left), 1);
	height = LEGACY_S16_WRAP_SUB(
		LEGACY_S16_WRAP_SUB(bottom, top), 1);
	if (width > 0) {
		sub_35B76(x, y, width, 1, color);
		sub_35B76(x, LEGACY_S16_FROM_BITS(bottom), width, 1, color);
	}
	if (height > 0) {
		y = LEGACY_S16_WRAP_ADD(y, 1);
		sub_35B76(x, y, 1, height, color);
		sub_35B76(LEGACY_S16_FROM_BITS(right), y,
			1, height, color);
	}
}

static legacy_u16 legacy_near_string_length(const legacy_s8* text)
{
	legacy_u16 length;

	length = 0;
	while (*text++ != 0)
		length = LEGACY_U16_WRAP_ADD(length, 1U);
	return length;
}

void print_int_as_string_maybe(legacy_s8* destination, legacy_s16 value, legacy_s16 zero_pad,
	legacy_s16 width)
{
	legacy_s8 digits[5];
	legacy_s16 signed_value;
	legacy_u16 magnitude;
	legacy_u16 digit_count;
	legacy_u16 length;
	legacy_u16 index;

	signed_value = LEGACY_S16_FROM_BITS((legacy_u16)value);
	magnitude = signed_value < 0 ?
		(legacy_u16)(0U - (legacy_u16)signed_value) :
		(legacy_u16)signed_value;
	digit_count = 0;
	do {
		digits[digit_count++] = (legacy_s8)('0' + magnitude % 10U);
		magnitude /= 10U;
	} while (magnitude != 0);

	index = 0;
	if (signed_value < 0)
		destination[index++] = '-';
	while (digit_count != 0)
		destination[index++] = digits[--digit_count];
	destination[index] = 0;
	length = index;

	if (width != 0) {
		while (LEGACY_S16_FROM_BITS((legacy_u16)width) <
			LEGACY_S16_FROM_BITS(length)) {
			for (index = 0; index < length; index++)
				destination[index] = destination[index + 1U];
			length--;
		}
		while (LEGACY_S16_FROM_BITS((legacy_u16)width) >
			LEGACY_S16_FROM_BITS(length)) {
			index = length;
			do {
				destination[index + 1U] = destination[index];
			} while (index-- != 0);
			destination[0] = ' ';
			length++;
		}
	}
	if (zero_pad != 0) {
		index = 0;
		while (destination[index] == ' ')
			destination[index++] = '0';
	}
}

static legacy_s8* legacy_near_string_copy(legacy_s8* destination, const legacy_s8* source)
{
	while ((*destination = *source) != 0) {
		destination++;
		source++;
	}
	return destination;
}

void format_frame_as_string(legacy_s8* destination, legacy_s16 frame_count,
	legacy_s16 include_hundredths)
{
	legacy_s8 number[18];
	legacy_s8* output;
	legacy_u16 frames;
	legacy_u16 frame_rate;
	legacy_u16 frames_per_minute;
	legacy_u16 minutes;
	legacy_u16 seconds;
	legacy_u16 hundredths;

	frames = (legacy_u16)frame_count;
	frame_rate = (legacy_u16)framespersec;
	frames_per_minute = LEGACY_U16_WRAP_MUL(60U, frame_rate);
	minutes = (legacy_u16)(frames / frames_per_minute);
	frames = LEGACY_U16_WRAP_SUB(frames,
		LEGACY_U16_WRAP_MUL(frames_per_minute, minutes));
	seconds = (legacy_u16)(frames / frame_rate);
	frames = LEGACY_U16_WRAP_SUB(frames,
		LEGACY_U16_WRAP_MUL(frame_rate, seconds));

	print_int_as_string_maybe(number, minutes, 0, 2);
	output = legacy_near_string_copy(destination, number);
	*output++ = ':';
	print_int_as_string_maybe(number, seconds, 1, 2);
	output = legacy_near_string_copy(output, number);
	if (include_hundredths != 0) {
		*output++ = '.';
		hundredths = LEGACY_U16_WRAP_MUL(
			(legacy_u16)(100 / LEGACY_S16_FROM_BITS(frame_rate)),
			frames);
		print_int_as_string_maybe(number, hundredths, 1, 2);
		legacy_near_string_copy(output, number);
	}
}

void parse_filepath_separators(legacy_s8* destination, const legacy_s8* path)
{
	legacy_u16 path_index;
	legacy_u16 output_index;
	legacy_s8 current;

	path_index = legacy_near_string_length(path);
	while (path_index != 0) {
		current = path[path_index - 1U];
		if (current == '\\' || current == ':')
			break;
		path_index--;
	}
	output_index = 0;
	do {
		current = path[path_index++];
		destination[output_index++] = current;
	} while (current != '.');
	destination[output_index - 1U] = 0;
}

extern legacy_s8 aId1[];
extern legacy_s8 aId2[];
extern legacy_s8 aId3[];
extern legacy_s8 aId4[];
extern legacy_s8 aDos_0[];
extern legacy_s8 aDea[];
extern legacy_s8 aDer[];
extern legacy_s8 aKey[];
extern legacy_s8 aMer[];
extern legacy_s8 aMof[];
extern legacy_s8 aMon[];
extern legacy_s8 aMrl[];
extern legacy_s8 aMrs[];
extern legacy_s8 aMou[];
extern legacy_s8 aPau[];
extern legacy_s8 aSof[];
extern legacy_s8 aSon[];
extern legacy_s8 aSav[];
extern legacy_s8 aWai[];
extern legacy_s8 aDefault_1[];
extern legacy_s8 aLoa[];
extern legacy_s8 aLsu[];
extern legacy_s8 aLsd[];
extern legacy_s8 unk_463EA[];
extern legacy_s8* findfilenames[];
extern void far* miscptr;
extern legacy_s16 word_407FA;
extern struct TRACKOBJECT trkObjectList[];
extern struct SHAPE2D far* tracksmenushapes1[];
extern struct SHAPE2D far* tracksmenushape2dunk[];
extern struct SHAPE2D far* tracksmenushape2dunk2[];
extern void call_exitlist2(void);
extern legacy_s16 word_3EB90;
extern legacy_s16 fontdef_unk_0E;
void font_set_unk(legacy_s16 color, legacy_s16 unknown);
legacy_s16 call_read_line(legacy_s8* text, legacy_s16 max_characters, legacy_s16 x, legacy_s16 y,
	legacy_u32 timeout);
legacy_s8 do_fileselect_dialog(legacy_s8* directory, legacy_s8* filename,
	legacy_s8* extension, legacy_s8 far* prompt);
legacy_u32 sub_2EB1E(legacy_u32 ticks);
void preRender_line(legacy_u16 x1, legacy_u16 y1,
	legacy_u16 x2, legacy_u16 y2, legacy_s16 color);
struct RECTANGLE* intro_draw_text(legacy_s8* text, legacy_s16 x, legacy_s16 y, legacy_s16 color,
	legacy_s16 shadow_color);
legacy_u8 subst_hillroad_track(legacy_u8 terrain, legacy_u8 track);

static legacy_u16 dialog_ascii_lower(legacy_u16 character)
{
	if (character < 256U &&
		(g_ascii_props[character] & RST_ASC_CHAR_UPPER) != 0)
		character = LEGACY_U16_WRAP_ADD(character, 0x20U);
	return character;
}

legacy_u16 show_dialog(
	legacy_s16 dialog_type,
	legacy_s16 save_background,
	void far* text_resource,
	legacy_u16 x_argument,
	legacy_u16 y_argument,
	legacy_s16 border_color,
	void* disabled_choices_argument,
	legacy_s16 initial_choice
) {
	legacy_s8 line_buffer[128];
	legacy_s8 choice_buffer[80];
	legacy_s8 far* choice_texts[20];
	legacy_u8 choice_lengths[20];
	legacy_s16 choice_left[20];
	legacy_s16 choice_right[20];
	legacy_s16 choice_top[20];
	legacy_s16 choice_bottom[20];
	legacy_s16* disabled_choices;
	legacy_s8 far* cursor;
	legacy_s16 line_height;
	legacy_s16 dialog_width;
	legacy_s16 dialog_height;
	legacy_s16 measured_width;
	legacy_s16 x;
	legacy_s16 y;
	legacy_s16 left;
	legacy_s16 right;
	legacy_s16 top;
	legacy_s16 bottom;
	legacy_s16 result;
	legacy_u16 line_length;
	legacy_u16 choice_width;
	legacy_u16 character_count;
	legacy_u16 input;
	legacy_u16 first_hotkey;
	legacy_u16 second_hotkey;
	legacy_u16 index;
	legacy_u16 copied;
	legacy_s16 hit;
	legacy_u8 character;
	legacy_u8 choice_count;
	legacy_u8 placeholder_index;
	legacy_u8 selected;
	legacy_u8 previous;
	legacy_u8 active;

	disabled_choices = (legacy_s16*)disabled_choices_argument;
	line_height = LEGACY_S16_WRAP_ADD(fontdef_unk_0E, 2);
	dialog_height = 0;
	dialog_width = 0x20;
	mouse_draw_opaque_check();

	cursor = (legacy_s8 far*)text_resource;
	line_length = 0;
	while ((character = (legacy_u8)*cursor) != 0) {
		if (character == ']' || character == '}') {
			line_buffer[line_length] = 0;
			measured_width = (legacy_s16)font_op2(line_buffer);
			if (measured_width > dialog_width)
				dialog_width = measured_width;
			line_length = 0;
			if (character == ']')
				dialog_height = LEGACY_S16_WRAP_ADD(
					dialog_height, line_height);
			else
				dialog_height = LEGACY_S16_WRAP_ADD(dialog_height, 4);
		} else {
			line_buffer[line_length++] = (legacy_s8)character;
		}
		cursor++;
	}

	dialog_width = LEGACY_S16_FROM_BITS(
		LEGACY_U16_WRAP_ADD((legacy_u16)dialog_width, 0x18U) & 0xFFF8U);
	x = LEGACY_S16_FROM_BITS(x_argument);
	y = LEGACY_S16_FROM_BITS(y_argument);
	if (x == -1) {
		x = LEGACY_S16_FROM_BITS((legacy_u16)((0x140 - dialog_width) / 2));
		x = LEGACY_S16_FROM_BITS((legacy_u16)x & 0xFFF8U);
	}
	if (y == -1)
		y = LEGACY_S16_FROM_BITS((legacy_u16)((0xC8 - dialog_height) / 2));

	left = x;
	right = LEGACY_S16_WRAP_ADD(x, dialog_width);
	top = LEGACY_S16_WRAP_SUB(y, 8);
	bottom = LEGACY_S16_WRAP_ADD(
		LEGACY_S16_WRAP_ADD(y, dialog_height), 8);
	x = LEGACY_S16_WRAP_ADD(x, 8);
	dialog_width = LEGACY_S16_WRAP_SUB(dialog_width, 0x10);
	if (save_background != 0 &&
		sub_274B0(left, right, top, bottom) == 0)
		return 0xFFFFU;

	sprite_copy_2_to_1();
	sprite_set_1_size(left, right, top, bottom);
	sprite_clear_1_color(0);
	sprite_1_unk4(LEGACY_S16_WRAP_SUB(x, 4),
		LEGACY_S16_WRAP_SUB(y, 4),
		LEGACY_S16_WRAP_ADD(
			LEGACY_S16_WRAP_ADD(x, dialog_width), 4),
		LEGACY_S16_WRAP_ADD(
			LEGACY_S16_WRAP_ADD(y, dialog_height), 4),
		border_color);
	font_set_unk(dialog_fnt_colour, 0);
	word_3EB90 = 0;
	font_set_unk(dialog_fnt_colour, 0);

	cursor = (legacy_s8 far*)text_resource;
	line_length = 0;
	placeholder_index = 0;
	dialog_height = 1;
	while ((character = (legacy_u8)*cursor) != 0 && character != '[') {
		if (character == ']' || character == '}') {
			line_buffer[line_length] = 0;
			sub_345BC(line_buffer, x,
				LEGACY_S16_WRAP_ADD(y, dialog_height));
			line_length = 0;
			if (character == ']')
				dialog_height = LEGACY_S16_WRAP_ADD(
					dialog_height, line_height);
			else
				dialog_height = LEGACY_S16_WRAP_ADD(dialog_height, 4);
		} else if (character == '@') {
			if (dialog_type == 3) {
				line_buffer[line_length] = 0;
				disabled_choices[placeholder_index] =
					LEGACY_S16_WRAP_ADD(x,
						(legacy_s16)font_op2(line_buffer));
				disabled_choices[placeholder_index + 1U] =
					LEGACY_S16_WRAP_ADD(y, dialog_height);
				placeholder_index = (legacy_u8)(placeholder_index + 2U);
			}
			line_buffer[line_length++] = ' ';
		} else {
			line_buffer[line_length++] = (legacy_s8)character;
		}
		cursor++;
	}

	choice_count = 0;
	while ((legacy_u8)*cursor == '[') {
		cursor++;
		choice_texts[choice_count] = cursor;
		line_buffer[line_length] = 0;
		choice_left[choice_count] = LEGACY_S16_WRAP_ADD(
			x, (legacy_s16)font_op2(line_buffer));
		choice_top[choice_count] = LEGACY_S16_WRAP_ADD(y, dialog_height);
		choice_bottom[choice_count] = LEGACY_S16_WRAP_ADD(
			choice_top[choice_count], line_height);
		line_buffer[line_length++] = ' ';
		choice_width = 0;
		character_count = 0;
		while ((character = (legacy_u8)*cursor) != 0 && character != '[') {
			if (character == ']' || character == '}') {
				line_buffer[line_length] = 0;
				choice_width = (legacy_u16)font_op2(line_buffer);
				line_length = 0;
				if (character == ']')
					dialog_height = LEGACY_S16_WRAP_ADD(
						dialog_height, line_height);
				else
					dialog_height = LEGACY_S16_WRAP_ADD(dialog_height, 3);
			} else {
				line_buffer[line_length++] = (legacy_s8)character;
				character_count++;
			}
			cursor++;
		}
		choice_lengths[choice_count] = (legacy_u8)character_count;
		line_buffer[line_length] = 0;
		if (choice_width == 0)
			choice_width = (legacy_u16)font_op2(line_buffer);
		choice_right[choice_count] = LEGACY_S16_WRAP_ADD(
			choice_left[choice_count], (legacy_s16)choice_width);
		choice_count++;
	}

	if (choice_count > 2U &&
		choice_left[0] == choice_left[1] &&
		choice_left[1] == choice_left[2]) {
		for (index = 0; index < choice_count; index++) {
			choice_right[index] = LEGACY_S16_WRAP_ADD(
				choice_left[index], dialog_width);
		}
	}
	mouse_draw_transparent_check();

	result = 1;
	if (dialog_type == 0)
		return 0;
	if (dialog_type == 1) {
		do {
			input = (legacy_u16)input_checking(
				(legacy_s16)timer_get_delta_alt());
		} while (input == 0);
		if (input == 0x1BU)
			result = 0;
		check_input();
		goto dialog_done;
	}
	if (dialog_type == 3)
		return (legacy_u16)(placeholder_index / 2U);
	if (dialog_type == 4) {
		(void)sub_2EB1E(8UL);
		goto dialog_done;
	}
	if (dialog_type != 2)
		goto dialog_done;

	selected = (legacy_u8)initial_choice;
	previous = 0xFFU;
	(void)timer_get_delta_alt();
	mouse_draw_opaque_check();
	first_hotkey = 0;
	second_hotkey = 0;
	if (choice_count == 2U) {
		cursor = choice_texts[0];
		do {
			first_hotkey = (legacy_u8)*cursor++;
		} while (first_hotkey == ' ');
		first_hotkey = dialog_ascii_lower(first_hotkey);
		cursor = choice_texts[1];
		do {
			second_hotkey = (legacy_u8)*cursor++;
		} while (second_hotkey == ' ');
		second_hotkey = dialog_ascii_lower(second_hotkey);
	}

	active = 1;
	while (active != 0) {
		if (selected != previous) {
			mouse_draw_opaque_check();
			for (index = 0; index < choice_count; index++) {
				if (selected == (legacy_u8)index)
					font_set_unk(word_3EB90, dialog_fnt_colour);
				else
					font_set_unk(dialog_fnt_colour, word_3EB90);
				if (disabled_choices != 0 && disabled_choices[index] != 0)
					font_set_unk(performGraphColor, word_3EB90);
				for (copied = 0; copied < choice_lengths[index]; copied++)
					choice_buffer[copied] = choice_texts[index][copied];
				choice_buffer[copied] = 0;
				sub_345BC(choice_buffer, choice_left[index],
					choice_top[index]);
			}
			mouse_draw_transparent_check();
			if (previous == 0xFFU)
				check_input();
			previous = selected;
		}

		input = (legacy_u16)input_checking((legacy_s16)timer_get_delta_alt());
		hit = (legacy_s16)mouse_multi_hittest(choice_count,
			choice_left, choice_right, choice_top, choice_bottom);
		if (hit != -1 &&
			(disabled_choices == 0 || disabled_choices[hit] == 0))
			selected = (legacy_u8)hit;

		if (choice_count == 2U && input != 0) {
			input = dialog_ascii_lower(input);
			if (input == first_hotkey) {
				selected = 0;
				input = 0x0DU;
			} else if (input == second_hotkey) {
				selected = 1;
				input = 0x0DU;
			}
		}

		if (input == 0)
			continue;
		if (input == 0x20U || input == 0x0DU) {
			active = 0;
			check_input();
			continue;
		}
		if (input == 0x1BU) {
			selected = 0xFFU;
			active = 0;
			check_input();
			continue;
		}
		if (input == 0x4800U || input == 0x4B00U) {
			do {
				selected = selected == 0 ?
					(legacy_u8)(choice_count - 1U) :
					(legacy_u8)(selected - 1U);
			} while (disabled_choices != 0 &&
				disabled_choices[selected] != 0);
			continue;
		}
		if (input == 0x4D00U || input == 0x5000U) {
			do {
				selected = (legacy_u8)(selected + 1U);
				if (selected >= choice_count)
					selected = 0;
			} while (disabled_choices != 0 &&
				disabled_choices[selected] != 0);
		}
	}
	result = LEGACY_S8_FROM_BITS(selected);

dialog_done:
	if (save_background != 0)
		sub_275C6();
	return (legacy_u16)result;
}

legacy_s8 do_fileselect_dialog(
	legacy_s8* directory,
	legacy_s8* filename,
	legacy_s8* extension,
	legacy_s8 far* prompt
) {
	legacy_s16 positions[40];
	legacy_s16 hit_left[10];
	legacy_s16 hit_right[10];
	legacy_s16 hit_top[10];
	legacy_s16 hit_bottom[10];
	legacy_s8 filenames[128][13];
	const legacy_s8* found_path;
	legacy_u16 index;
	legacy_u16 compare_index;
	legacy_u16 visible_row;
	legacy_u16 text_width;
	legacy_u16 key;
	legacy_s16 dialog_result;
	legacy_s16 hit;
	legacy_s16 candidate;
	legacy_s8 selected;
	legacy_s8 scroll;
	legacy_s8 previous_selected;
	legacy_s8 previous_scroll;
	legacy_s8 result;
	legacy_u8 file_count;
	legacy_u8 saved_busy;
	legacy_u8 character;

	dialog_result = LEGACY_S16_FROM_BITS(show_dialog(3, 1,
		locate_text_res(mainresptr, aLoa), 0xFFFFU, 0xFFFFU,
		dialogarg2, positions, 0));
	if (dialog_result < 0)
		return 0;

	saved_busy = g_is_busy;
	g_is_busy = 1;
	preRender_line(positions[4] - 4, positions[5] + 4,
		positions[4] + 0xAB, positions[5] + 4, dialogarg2);
	font_set_unk(dialog_fnt_colour, word_3EB90);
	copy_string(&resID_byte1, prompt);
	sub_345BC(&resID_byte1, positions[0], positions[1]);

	for (index = 0; index < 10U; index++) {
		hit_left[index] = positions[2];
		hit_right[index] = positions[2] + 0xA2;
		if (index == 9U)
			hit_top[index] = hit_top[index - 1U] + 10;
		else
			hit_top[index] = positions[3U + index * 2U];
		hit_bottom[index] = hit_top[index] + 10;
	}
	font_set_unk(dialog_fnt_colour, word_3EB90);
	sub_345BC(directory, positions[2], positions[3]);

restart_search:
	mouse_draw_transparent_check();
	file_count = 0;
	found_path = file_combine_and_find(directory, "*", extension);
	if (found_path == 0) {
		font_set_unk(dialog_fnt_colour, word_3EB90);
		key = (legacy_u16)call_read_line(directory, 0x12,
			positions[2], positions[3], 0x7530UL);
		if (key == 0x1BU) {
			result = 0;
			goto file_dialog_done;
		}
		goto restart_search;
	}

	parse_filepath_separators(filenames[file_count++], found_path);
	while (file_count < 128U && (found_path = file_find_next_alt()) != 0)
		parse_filepath_separators(filenames[file_count++], found_path);

	for (index = 0; index + 1U < file_count; index++) {
		for (compare_index = index + 1U;
			compare_index < file_count; compare_index++) {
			if (strcmp(filenames[index], filenames[compare_index]) > 0) {
				strcpy(&resID_byte1, filenames[index]);
				strcpy(filenames[index], filenames[compare_index]);
				strcpy(filenames[compare_index], &resID_byte1);
			}
		}
	}

	if (file_count > 7U) {
		copy_string(&resID_byte1, locate_text_res(mainresptr, aLsu));
		sub_345BC(&resID_byte1, font_op2_alt(&resID_byte1), positions[25]);
		copy_string(&resID_byte1, locate_text_res(mainresptr, aLsd));
		sub_345BC(&resID_byte1, font_op2_alt(&resID_byte1),
			positions[33] - 1);
	}

	selected = 0;
	scroll = 0;
	previous_selected = -1;
	previous_scroll = -1;
	(void)timer_get_delta_alt();
	result = 0;
	for (;;) {
		if (selected != previous_selected || scroll != previous_scroll) {
			previous_selected = selected;
			previous_scroll = scroll;
			mouse_draw_opaque_check();
			for (visible_row = 0; visible_row < 7U; visible_row++) {
				candidate = (legacy_s16)(scroll + (legacy_s16)visible_row);
				if (candidate == selected)
					font_set_unk(word_3EB90, dialog_fnt_colour);
				else
					font_set_unk(dialog_fnt_colour, word_3EB90);
				if (candidate < (legacy_s16)file_count) {
					strcpy(&resID_byte1, filenames[(legacy_u8)candidate]);
					sub_345BC(&resID_byte1, positions[2],
						hit_top[visible_row + 2U]);
				} else {
					sub_345BC("        ", positions[2],
						hit_top[visible_row + 2U]);
				}
				text_width = (legacy_u16)font_op2(&resID_byte1);
				sprite_1_unk(positions[2] + text_width,
					hit_top[visible_row + 2U],
					positions[2] + 0xA2 - text_width - positions[2],
					8, word_3EB90);
			}
			mouse_draw_transparent_check();
		}

		key = (legacy_u16)input_checking((legacy_s16)timer_get_delta_alt());
		hit = (legacy_s16)mouse_multi_hittest(10,
			hit_left, hit_right, hit_top, hit_bottom);
		if (hit != -1) {
			if (hit == 0) {
				if ((mouse_butstate & 3U) != 0) {
					selected = 0;
					scroll = -1;
					key = 0;
				}
			} else if (hit == 1) {
				if ((mouse_butstate & 3U) != 0) {
					if ((legacy_s16)selected + scroll != 0)
						selected--;
					if (selected < scroll)
						scroll = selected;
					key = 0;
				}
			} else if (hit == 9) {
				if ((mouse_butstate & 3U) != 0) {
					if (selected != (legacy_s8)(file_count - 1U))
						selected++;
					key = 0;
				}
			} else {
				candidate = (legacy_s16)(scroll + hit - 2);
				if (candidate < (legacy_s16)file_count)
					selected = (legacy_s8)candidate;
			}
		}

		if (key == 0x0DU || key == 0x20U) {
			result = 1;
		} else if (key == 0x1BU) {
			result = -1;
		} else if (key == 0x4800U) {
			selected--;
		} else if (key == 0x5000U) {
			if (selected != (legacy_s8)(file_count - 1U))
				selected++;
		} else if (key < 256U &&
			(g_ascii_props[key] &
				(RST_ASC_CHAR_UPPER | RST_ASC_CHAR_LOWER)) != 0) {
			character = (legacy_u8)dialog_ascii_lower(key);
			for (index = 0; index < file_count; index++) {
				if ((legacy_u8)dialog_ascii_lower(
					(legacy_u8)filenames[index][0]) == character) {
					selected = (legacy_s8)index;
					break;
				}
			}
		}

		if (selected < scroll)
			scroll = selected;
		if (scroll < 0) {
			font_set_unk(dialog_fnt_colour, word_3EB90);
			key = (legacy_u16)call_read_line(directory, 0x12,
				positions[2], positions[3], 0x7530UL);
			if (key == 0x1BU) {
				result = 0;
				goto file_dialog_done;
			}
			goto restart_search;
		}
		while ((legacy_s16)(scroll + 6) < selected)
			scroll++;

		if (result == 0)
			continue;
		if (result < 0) {
			result = 0;
			goto file_dialog_done;
		}
		strcpy(filename, filenames[(legacy_u8)selected]);
		result = 1;
		goto file_dialog_done;
	}

file_dialog_done:
	sub_275C6();
	g_is_busy = saved_busy;
	return result;
}

void ensure_file_exists(legacy_s16 file_index)
{
	static legacy_s8* const message_ids[] = { aId1, aId2, aId3, aId4 };
	legacy_s8* message_id;

	message_id = message_ids[file_index - 1];
	while (file_find(findfilenames[file_index]) == 0) {
		show_dialog(1, 1, locate_text_res(mainresptr, message_id),
			-1, -1, dialogarg2, 0, 0);
		mouse_draw_opaque_check();
		kbormouse = 0;
	}
}

void show_waiting(void)
{
	show_dialog(0, 0, locate_text_res(mainresptr, aWai),
		-1, waitflag, dialogarg2, 0, 0);
	mouse_draw_opaque_check();
}

void do_mer_restext(void)
{
	show_dialog(1, 1, locate_text_res(mainresptr, aMer),
		-1, -1, dialogarg2, 0, 0);
}

void do_key_restext(void)
{
	input_push_status();
	dos_timer_callbacks_suspended = 1;
	audio_suspend();
	show_dialog(4, 1, locate_text_res(mainresptr, aKey),
		-1, -1, dialogarg2, 0, 0);
	dos_joystick_enabled = 0;
	byte_3B8F2 = 0;
	dos_timer_callbacks_suspended = 0;
	audio_resume();
	input_pop_status();
}

void do_joy_restext(void)
{
	legacy_s16 positions[15];
	legacy_s16 button_x[9];
	legacy_s16 button_y[9];
	legacy_u8 visited[9];
	legacy_s16 button_width;
	legacy_s16 button_height;
	legacy_s16 line_width;
	legacy_s16 line_height;
	legacy_s16 selected;
	legacy_s16 next_selected;
	legacy_u16 joy_flags;
	legacy_u16 i;

	input_push_status();
	dos_timer_callbacks_suspended = 1;
	audio_suspend();
	if (LEGACY_S16_FROM_BITS(show_dialog(3, 1,
		locate_text_res(mainresptr, "joy"), 0xFFFFU, 0xFFFFU,
		dialogarg2, positions, 0)) <= 0) {
		dos_joystick_enabled = 0;
		goto joy_dialog_done;
	}

	for (i = 0; i < 9U; i++)
		visited[i] = 0;
	dos_joystick_enabled = 1;
	mouse_draw_opaque_check();
	line_height = LEGACY_S16_WRAP_SUB(
		LEGACY_S16_WRAP_SUB(positions[13], positions[3]), 8);
	sprite_1_unk(LEGACY_S16_WRAP_SUB(positions[2], 4), positions[3],
		1, line_height, dialogarg2);
	sprite_1_unk(LEGACY_S16_WRAP_SUB(positions[4], 4), positions[5],
		1, line_height, dialogarg2);
	line_width = LEGACY_S16_WRAP_SUB(positions[6], positions[0]);
	sprite_1_unk(positions[0], LEGACY_S16_WRAP_SUB(positions[9], 4),
		line_width, 1, dialogarg2);
	sprite_1_unk(positions[0], LEGACY_S16_WRAP_SUB(positions[11], 4),
		line_width, 1, dialogarg2);

	button_x[0] = positions[2];
	button_x[1] = positions[2];
	button_x[5] = positions[2];
	button_x[2] = positions[4];
	button_x[3] = positions[4];
	button_x[4] = positions[4];
	button_x[6] = positions[0];
	button_x[7] = positions[0];
	button_x[8] = positions[0];
	button_y[0] = positions[9];
	button_y[3] = positions[9];
	button_y[7] = positions[9];
	button_y[1] = positions[3];
	button_y[2] = positions[3];
	button_y[8] = positions[3];
	button_y[4] = positions[11];
	button_y[5] = positions[11];
	button_y[6] = positions[11];
	button_width = LEGACY_S16_WRAP_SUB(
		LEGACY_S16_WRAP_SUB(positions[2], positions[0]), 8);
	button_height = LEGACY_S16_WRAP_SUB(
		LEGACY_S16_WRAP_SUB(positions[9], positions[1]), 8);

	selected = -1;
	sub_307B4();
	for (;;) {
		if (kb_read_char() != 0)
			break;
		joy_flags = (legacy_u16)dos_get_joy_flags();
		if ((joy_flags & 0x30U) != 0)
			break;
		next_selected = (legacy_s16)sub_307D2(joy_flags);
		if (next_selected == selected)
			continue;
		for (i = 0; i < 9U; i++)
			sprite_1_unk(button_x[i], button_y[i], button_width,
				button_height, word_3EB90);
		sprite_1_unk(button_x[next_selected], button_y[next_selected],
			button_width, button_height, dialog_fnt_colour);
		selected = next_selected;
		visited[next_selected] = 1;
	}

	for (i = 0; i < 9U; i++)
		dos_joystick_enabled = (legacy_u8)dos_joystick_enabled & visited[i];
	sub_275C6();
	if (dos_joystick_enabled == 0)
		show_dialog(1, 1, locate_text_res(mainresptr, "jox"),
			0xFFFFU, 0xFFFFU, dialogarg2, 0, 0);

joy_dialog_done:
	kb_check();
	byte_3B8F2 = 0;
	audio_resume();
	dos_timer_callbacks_suspended = 0;
	input_pop_status();
}

void do_mou_restext(void)
{
	input_push_status();
	dos_timer_callbacks_suspended = 1;
	audio_suspend();
	byte_3B8F2 = 1;
	show_dialog(4, 1, locate_text_res(mainresptr, aMou),
		-1, -1, dialogarg2, 0, 0);
	dos_timer_callbacks_suspended = 0;
	audio_resume();
	input_pop_status();
}

void do_pau_restext(void)
{
	input_push_status();
	dos_timer_callbacks_suspended = 1;
	audio_suspend();
	show_dialog(0, 1, locate_text_res(mainresptr, aPau),
		-1, -1, dialogarg2, 0, 0);
	dos_timer_callbacks_suspended = 0;
	audio_resume();
	input_pop_status();
}

void do_mof_restext(void)
{
	legacy_s8* message_id;

	input_push_status();
	dos_timer_callbacks_suspended = 1;
	message_id = audio_toggle_flag2() != 0 ? aMon : aMof;
	show_dialog(4, 1, locate_text_res(mainresptr, message_id),
		-1, -1, dialogarg2, 0, 0);
	dos_timer_callbacks_suspended = 0;
	input_pop_status();
}

void do_sonsof_restext(void)
{
	legacy_s8* message_id;

	input_push_status();
	dos_timer_callbacks_suspended = 1;
	message_id = audio_toggle_flag6() != 0 ? aSon : aSof;
	show_dialog(4, 1, locate_text_res(mainresptr, message_id),
		-1, -1, dialogarg2, 0, 0);
	dos_timer_callbacks_suspended = 0;
	input_pop_status();
}

void do_dos_restext(void)
{
	legacy_s16 result;

	input_push_status();
	dos_timer_callbacks_suspended = 1;
	audio_suspend();
	result = show_dialog(2, 1, locate_text_res(mainresptr, aDos_0),
		-1, -1, dialogarg2, 0, 0);
	if (result == 1)
		call_exitlist2();
	dos_timer_callbacks_suspended = 0;
	audio_resume();
	input_pop_status();
}

legacy_s16 do_savefile_dialog(legacy_s8* primary, legacy_s8* secondary, legacy_s8 far* prompt)
{
	legacy_s16 positions[6];
	legacy_s16 character_index;
	legacy_s16 key;
	legacy_s16 result;

	result = LEGACY_S16_FROM_BITS(show_dialog(3, 1,
		locate_text_res(mainresptr, aSav), -1, -1, dialogarg2,
		positions, 0));
	if (result < 0)
		return 0;

	font_set_unk(dialog_fnt_colour, word_3EB90);
	copy_string(&resID_byte1, prompt);
	sub_345BC(&resID_byte1, positions[0], positions[1]);
	font_set_unk(dialog_fnt_colour, word_3EB90);
	sub_345BC(primary, positions[2], positions[3]);
	sub_345BC(secondary, positions[4], positions[5]);
	mouse_draw_transparent_check();

	result = 0;
	for (;;) {
		key = LEGACY_S16_FROM_BITS(call_read_line(secondary, 8,
			positions[4], positions[5], 0x7530UL));
		for (character_index = 0; secondary[character_index] != 0;
			character_index++) {
			if (secondary[character_index] == ' ')
				secondary[character_index] = '_';
		}
		if (key == 0x1B)
			break;
		if (key == 0x0D) {
			result = 1;
			break;
		}
		key = LEGACY_S16_FROM_BITS(call_read_line(primary, 0x12,
			positions[2], positions[3], 0x7530UL));
		if (key == 0x1B)
			break;
	}

	sub_275C6();
	return result;
}

void show_graphic_levels_menu(void)
{
	legacy_s8 selected_options[9];
	legacy_s8 menu_text[512];
	legacy_u16 original_frame_rate;
	legacy_u16 option_index;
	legacy_u16 text_index;
	legacy_s8 selected;

	input_push_status();
	dos_timer_callbacks_suspended = 1;
	audio_suspend();
	original_frame_rate = framespersec2;
	selected = 0;
	for (;;) {
		copy_string(menu_text, locate_text_res(mainresptr, aMrl));
		for (option_index = 0; option_index < 9U; option_index++)
			selected_options[option_index] = 0;
		selected_options[detail_level] = 1;
		selected_options[5U + slow_video_mgmt] = 1;
		selected_options[framespersec2 == 10U ? 7 : 8] = 1;

		text_index = 0;
		for (option_index = 0; option_index < 9U; option_index++) {
			while (menu_text[text_index] != '[')
				text_index++;
			if (selected_options[option_index] != 0)
				menu_text[text_index + 1U] = '*';
			text_index++;
		}

		selected = LEGACY_S8_FROM_BITS(show_dialog(2, 1,
			(void far*)menu_text, -1, -1, performGraphColor, 0,
			(legacy_s16)selected));
		if (selected == -1 || selected == 9)
			break;
		switch (selected) {
		case 5:
			slow_video_mgmt = 0;
			break;
		case 6:
			slow_video_mgmt = 1;
			break;
		case 7:
			framespersec2 = 10;
			break;
		case 8:
			framespersec2 = 20;
			break;
		default:
			detail_level = (legacy_u8)selected;
			break;
		}
	}

	if (original_frame_rate != framespersec2)
		show_dialog(1, 1, locate_text_res(mainresptr, aMrs),
			-1, -1, dialogarg2, 0, 0);
	dos_timer_callbacks_suspended = 0;
	audio_resume();
	input_pop_status();
}

legacy_u16 run_option_menu(void)
{
	legacy_s8 selected;
	legacy_s8 initial_input;
	legacy_u8 menu_active;
	legacy_s8 far* prompt;

	miscptr = file_load_resfile("misc");
	sprite_copy_2_to_1_2();
	sprite_clear_1_color((legacy_u8)word_407FA);
	copy_string(&resID_byte1, locate_shape_alt(miscptr, "gstu"));
	intro_draw_text(&resID_byte1, font_op2_alt(&resID_byte1), 6,
		dialog_fnt_colour, 0);
	copy_string(&resID_byte1, locate_shape_alt(miscptr, "gver"));
	intro_draw_text(&resID_byte1, font_op2_alt(&resID_byte1), 0x10,
		dialog_fnt_colour, 0);

	menu_active = 1;
	while (menu_active != 0) {
		selected = LEGACY_S8_FROM_BITS(show_dialog(2, 1,
			locate_text_res(miscptr, "mop"), 0xFFFFU, 0xFFFFU,
			dialogarg2, 0, 0));
		switch (selected) {
		case -1:
		case 6:
			menu_active = 0;
			break;

		case 0:
			if (byte_3B8F2 != 0)
				initial_input = 2;
			else if (dos_joystick_enabled != 0)
				initial_input = 1;
			else
				initial_input = 0;
			selected = LEGACY_S8_FROM_BITS(show_dialog(2, 1,
				locate_text_res(miscptr, "mid"), 0xFFFFU,
				0xFFFFU, performGraphColor, 0, initial_input));
			if (selected == 0)
				do_key_restext();
			else if (selected == 1)
				do_joy_restext();
			else if (selected == 2)
				do_mou_restext();
			break;

		case 1:
			do_mof_restext();
			break;

		case 2:
			do_sonsof_restext();
			break;

		case 3:
			prompt = locate_text_res(mainresptr, "rep");
			if (do_fileselect_dialog(byte_3B85E, aDefault_1,
				".rpl", prompt) != 0) {
				waitflag = 0x96;
				show_waiting();
				file_load_replay(byte_3B85E, aDefault_1);
				menu_active = 1;
				goto option_menu_done;
			}
			break;

		case 4:
			show_graphic_levels_menu();
			break;

		case 5:
			do_dos_restext();
			break;
		}
	}

option_menu_done:
	unload_resource(miscptr);
	return menu_active;
}

void preRender_icons(legacy_u8 page)
{
	legacy_u16 row;
	legacy_u16 column;
	legacy_u16 x;
	legacy_u16 y;
	legacy_u8 tile;
	legacy_u8 multi_tile;

	for (row = 0; row < 6U; row++) {
		for (column = 0; column < 6U; column++) {
			tile = pboxshape[(legacy_u16)page * 36U + row * 6U +
				column];
			x = LEGACY_U16_WRAP_ADD(0x00DCU,
				LEGACY_U16_WRAP_MUL(column, 16U));
			y = LEGACY_U16_WRAP_ADD(0x0024U,
				LEGACY_U16_WRAP_MUL(row, 16U));
			if (page == 0) {
				sprite_shape_to_1(tracksmenushapes1[tile], x, y);
				continue;
			}
			if (tile >= 0xFDU)
				continue;

			sprite_shape_to_1(tracksmenushapes1[0], x, y);
			multi_tile = trkObjectList[tile].ss_multiTileFlag;
			if (multi_tile == 1U || multi_tile == 3U)
				sprite_shape_to_1(tracksmenushapes1[0], x,
					LEGACY_U16_WRAP_ADD(y, 16U));
			if (multi_tile == 2U || multi_tile == 3U)
				sprite_shape_to_1(tracksmenushapes1[0],
					LEGACY_U16_WRAP_ADD(x, 16U), y);
			if (multi_tile == 3U)
				sprite_shape_to_1(tracksmenushapes1[0],
					LEGACY_U16_WRAP_ADD(x, 16U),
					LEGACY_U16_WRAP_ADD(y, 16U));
			putpixel_iconMask(tracksmenushape2dunk2[tile], x, y);
			putpixel_iconFillings(tracksmenushape2dunk[tile], x, y);
		}
	}
}

static legacy_u16 track_menu_previous_row(legacy_u16 row);

void draw_2DtrackMap(
	legacy_u8 column_offset,
	legacy_u8 row_offset,
	legacy_u8* cached_track,
	legacy_u8* cached_terrain
) {
	legacy_u16 map_row;
	legacy_u16 map_column;
	legacy_u16 source_row;
	legacy_u16 source_column;
	legacy_u16 source_index;
	legacy_u16 cache_index;
	legacy_s16 x;
	legacy_s16 y;
	legacy_u8 tile;
	legacy_u8 terrain;
	legacy_u8 neighbor_tile;
	legacy_u8 multi_tile;

	for (map_row = 0; map_row < 11U; map_row++) {
		for (map_column = 0; map_column < 12U; map_column++) {
			source_row = LEGACY_U16_WRAP_ADD(row_offset, map_row);
			source_column = LEGACY_U16_WRAP_ADD(
				column_offset, map_column);
			source_index = LEGACY_U16_WRAP_ADD(
				(legacy_u16)trackrows[source_row], source_column);
			tile = td14_elem_map_main[source_index];
			terrain = td15_terr_map_main[LEGACY_U16_WRAP_ADD(
				(legacy_u16)terrainrows[source_row], source_column)];
			cache_index = LEGACY_U16_WRAP_ADD(
				LEGACY_U16_WRAP_MUL(map_row, 12U), map_column);
			x = LEGACY_S16_WRAP_ADD(
				LEGACY_S16_WRAP_MUL((legacy_s16)map_column, 16), 8);
			y = LEGACY_S16_WRAP_ADD(
				LEGACY_S16_WRAP_MUL((legacy_s16)map_row, 16), 4);

			if (tile < 0xFDU) {
				if (tile == 0) {
					if (cached_track[cache_index] == 0 &&
						cached_terrain[cache_index] == terrain)
						continue;
					sprite_shape_to_1(tracksmenushapes1[terrain], x, y);
					cached_track[cache_index] = 0;
					cached_terrain[cache_index] = terrain;
					continue;
				}

				if (cached_track[cache_index] == tile &&
					cached_terrain[cache_index] == terrain)
					continue;
				cached_track[cache_index] = tile;
				cached_terrain[cache_index] = terrain;
				sprite_shape_to_1(tracksmenushapes1[terrain], x, y);
				multi_tile = (legacy_u8)
					trkObjectList[tile].ss_multiTileFlag;
				switch (multi_tile) {
				case 0:
					putpixel_iconMask(tracksmenushape2dunk2[tile], x, y);
					putpixel_iconFillings(tracksmenushape2dunk[tile], x, y);
					break;

				case 1:
					terrain = td15_terr_map_main[LEGACY_U16_WRAP_ADD(
						(legacy_u16)terrainrows[source_row + 1U],
						source_column)];
					sprite_putimage_and_alt(tracksmenushapes1[terrain],
						x, LEGACY_S16_WRAP_ADD(y, 16));
					sprite_putimage_and(tracksmenushape2dunk2[tile], x, y);
					sprite_putimage_or(tracksmenushape2dunk[tile], x, y);
					break;

				case 2:
					terrain = td15_terr_map_main[LEGACY_U16_WRAP_ADD(
						(legacy_u16)terrainrows[source_row],
						source_column + 1U)];
					sprite_putimage_and_alt(tracksmenushapes1[terrain],
						LEGACY_S16_WRAP_ADD(x, 16), y);
					sprite_putimage_and(tracksmenushape2dunk2[tile], x, y);
					sprite_putimage_or(tracksmenushape2dunk[tile], x, y);
					break;

				case 3:
					terrain = td15_terr_map_main[LEGACY_U16_WRAP_ADD(
						(legacy_u16)terrainrows[source_row],
						source_column + 1U)];
					sprite_putimage_and_alt(tracksmenushapes1[terrain],
						LEGACY_S16_WRAP_ADD(x, 16), y);
					terrain = td15_terr_map_main[LEGACY_U16_WRAP_ADD(
						(legacy_u16)terrainrows[source_row + 1U],
						source_column)];
					sprite_putimage_and_alt(tracksmenushapes1[terrain],
						x, LEGACY_S16_WRAP_ADD(y, 16));
					terrain = td15_terr_map_main[LEGACY_U16_WRAP_ADD(
						(legacy_u16)terrainrows[source_row + 1U],
						source_column + 1U)];
					sprite_putimage_and_alt(tracksmenushapes1[terrain],
						LEGACY_S16_WRAP_ADD(x, 16),
						LEGACY_S16_WRAP_ADD(y, 16));
					sprite_putimage_and(tracksmenushape2dunk2[tile], x, y);
					sprite_putimage_or(tracksmenushape2dunk[tile], x, y);
					break;
				}
				continue;
			}

			if (map_row != 0 && map_column != 0) {
				cached_track[cache_index] = 0xFFU;
				cached_terrain[cache_index] = 0xFFU;
				continue;
			}
			cached_track[cache_index] = 0xFFU;

			if (tile == 0xFFU && map_column == 0) {
				sprite_putimage_and_alt(tracksmenushapes1[terrain], x, y);
				terrain = td15_terr_map_main[LEGACY_U16_WRAP_ADD(
					(legacy_u16)terrainrows[source_row + 1U],
					source_column)];
				sprite_putimage_and_alt(tracksmenushapes1[terrain],
					x, LEGACY_S16_WRAP_ADD(y, 16));
				neighbor_tile = td14_elem_map_main[
					LEGACY_U16_WRAP_SUB(source_index, 1U)];
				sprite_putimage_and(tracksmenushape2dunk2[neighbor_tile],
					LEGACY_S16_WRAP_SUB(x, 16), y);
				sprite_putimage_or(tracksmenushape2dunk[neighbor_tile],
					LEGACY_S16_WRAP_SUB(x, 16), y);
			} else if (tile == 0xFEU && map_row == 0) {
				sprite_putimage_and_alt(tracksmenushapes1[terrain], x, y);
				terrain = td15_terr_map_main[LEGACY_U16_WRAP_ADD(
					(legacy_u16)terrainrows[source_row],
					source_column + 1U)];
				sprite_putimage_and_alt(tracksmenushapes1[terrain],
					LEGACY_S16_WRAP_ADD(x, 16), y);
				neighbor_tile = td14_elem_map_main[LEGACY_U16_WRAP_ADD(
					track_menu_previous_row(source_row), source_column)];
				sprite_putimage_and(tracksmenushape2dunk2[neighbor_tile],
					x, LEGACY_S16_WRAP_SUB(y, 16));
				sprite_putimage_or(tracksmenushape2dunk[neighbor_tile],
					x, LEGACY_S16_WRAP_SUB(y, 16));
			} else if (tile == 0xFDU && map_row == 0 && map_column == 0) {
				sprite_putimage_and_alt(tracksmenushapes1[terrain], x, y);
				neighbor_tile = td14_elem_map_main[LEGACY_U16_WRAP_SUB(
					LEGACY_U16_WRAP_ADD(
						track_menu_previous_row(source_row), source_column),
					1U)];
				sprite_putimage_and(tracksmenushape2dunk2[neighbor_tile],
					LEGACY_S16_WRAP_SUB(x, 16),
					LEGACY_S16_WRAP_SUB(y, 16));
				sprite_putimage_or(tracksmenushape2dunk[neighbor_tile],
					LEGACY_S16_WRAP_SUB(x, 16),
					LEGACY_S16_WRAP_SUB(y, 16));
			}
		}
	}
}

static legacy_u16 track_menu_next_row(legacy_u16 row)
{
	if (row == 29U)
		return (legacy_u16)dos_mouse_button_count;
	return (legacy_u16)trackrows[row + 1U];
}

static legacy_u16 track_menu_previous_row(legacy_u16 row)
{
	if (row == 0)
		return (legacy_u16)word_45D3E;
	return (legacy_u16)trackrows[row - 1U];
}

void sub_2C9B4(void)
{
	legacy_u8 used[900];
	legacy_u16 row;
	legacy_u16 column;
	legacy_u16 current_index;
	legacy_u16 next_index;
	legacy_u16 east_index;
	legacy_u8 tile;
	legacy_u8 multi_tile;

	for (current_index = 0; current_index < 900U; current_index++)
		used[current_index] = 0;

	for (row = 0; row < 30U; row++) {
		for (column = 0; column < 30U; column++) {
			current_index = LEGACY_U16_WRAP_ADD(trackrows[row],
				column);
			tile = td14_elem_map_main[current_index];
			if (tile == 0)
				continue;
			if (tile >= 0xFDU) {
				if (used[current_index] == 0)
					td14_elem_map_main[current_index] = 0;
				continue;
			}

			multi_tile = trkObjectList[tile].ss_multiTileFlag;
			switch (multi_tile) {
			case 1:
				next_index = LEGACY_U16_WRAP_ADD(
					track_menu_next_row(row), column);
				if (used[next_index] != 0 ||
					td14_elem_map_main[next_index] != 0xFEU)
					td14_elem_map_main[current_index] = 0;
				else
					used[next_index] = 1;
				break;

			case 2:
				east_index = LEGACY_U16_WRAP_ADD(current_index, 1U);
				if (used[east_index] != 0 ||
					td14_elem_map_main[east_index] != 0xFFU)
					td14_elem_map_main[current_index] = 0;
				else
					used[east_index] = 1;
				break;

			case 3:
				east_index = LEGACY_U16_WRAP_ADD(current_index, 1U);
				next_index = LEGACY_U16_WRAP_ADD(
					track_menu_next_row(row), column);
				if (used[east_index] != 0 || used[next_index] != 0 ||
					used[LEGACY_U16_WRAP_ADD(next_index, 1U)] != 0 ||
					td14_elem_map_main[east_index] != 0xFFU ||
					td14_elem_map_main[next_index] != 0xFEU ||
					td14_elem_map_main[
						LEGACY_U16_WRAP_ADD(next_index, 1U)] != 0xFDU) {
					td14_elem_map_main[current_index] = 0;
				} else {
					used[east_index] = 1;
					used[next_index] = 1;
					used[LEGACY_U16_WRAP_ADD(next_index, 1U)] = 1;
				}
				break;
			}
		}
	}
}

legacy_s16 sub_2C81C(void)
{
	legacy_u16 row;
	legacy_u16 column;
	legacy_u16 current_index;
	legacy_u16 source_index;
	legacy_u8 terrain;
	legacy_u8 tile;
	legacy_u8 error;

	sub_2C9B4();
	error = 0;
	for (row = 0; row < 30U; row++) {
		for (column = 0; column < 30U; column++) {
			terrain = td15_terr_map_main[
				LEGACY_U16_WRAP_ADD(terrainrows[row], column)];
			current_index = LEGACY_U16_WRAP_ADD(trackrows[row],
				column);
			tile = td14_elem_map_main[current_index];
			if (tile == 0 || terrain == 0 || terrain == 6U)
				continue;

			if (terrain >= 1U && terrain <= 5U) {
				if (tile == 0xFFU) {
					source_index = LEGACY_U16_WRAP_SUB(current_index, 1U);
					tile = td14_elem_map_main[source_index];
				} else if (tile == 0xFEU) {
					source_index = LEGACY_U16_WRAP_ADD(
						track_menu_previous_row(row), column);
					tile = td14_elem_map_main[source_index];
				} else if (tile == 0xFDU) {
					source_index = LEGACY_U16_WRAP_SUB(
						LEGACY_U16_WRAP_ADD(
							track_menu_previous_row(row), column), 1U);
					tile = td14_elem_map_main[source_index];
				}

				if (!((tile >= 0x22U && tile <= 0x23U) ||
					(tile >= 0x67U && tile <= 0x6CU) ||
					(tile >= 0xABU && tile <= 0xAEU))) {
					td14_elem_map_main[current_index] = 0;
					error = 0x0C;
				}
			} else if (terrain >= 7U && terrain <= 10U) {
				if (subst_hillroad_track(terrain, tile) == 0) {
					td14_elem_map_main[current_index] = 0;
					error = 0x0D;
				}
			} else {
				td14_elem_map_main[current_index] = 0;
				error = 0x0E;
			}
		}
	}
	if (error != 0)
		sub_2C9B4();
	return error;
}

legacy_s16 do_dea_textres(void)
{
	legacy_s16 result;

	input_push_status();
	if (g_is_busy != 0) {
		result = show_dialog(2, 1,
			locate_text_res(mainresptr, aDea),
			-1, -1, dialogarg2, 0, 0) == 0;
	} else {
		show_dialog(0, 1, locate_text_res(mainresptr, aDer),
			-1, -1, dialogarg2, 0, 0);
		result = 1;
	}
	input_pop_status();
	return result;
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

legacy_s16 run_intro(void)
{
	struct SHAPE2D far* shape;
	legacy_s16 result;

	mouse_draw_opaque_check();
	sprite_copy_2_to_1_clear();
	mouse_draw_transparent_check();
	sprite_copy_wnd_to_1_clear();

	shape = (struct SHAPE2D far*)locate_shape_fatal(
		(legacy_s8 far*)tempdataptr, "prod");
	waitflag = shape->s2d_pos_y != 0 ? 0xA0 : 0xB4;

	shape = (struct SHAPE2D far*)locate_shape_fatal(
		(legacy_s8 far*)tempdataptr, "prod");
	sprite_shape_to_1_alt(shape);
	result = sprite_blit_to_video(wndsprite, -1);
	if (result == 0)
		result = input_repeat_check(0x190);

	if (result == 0) {
		sprite_copy_wnd_to_1_clear();
		waitflag = 0xB4;
		shape = (struct SHAPE2D far*)locate_shape_fatal(
			(legacy_s8 far*)tempdataptr, "titl");
		sprite_shape_to_1_alt(shape);
		result = sprite_blit_to_video(wndsprite, -1);
		if (result == 0)
			result = input_repeat_check(0x190);
	}

	return result;
}

legacy_s16 run_intro_looped(void)
{
	legacy_s16 result;

	file_load_audiores("skidtitl", "skidms", "TITL");
	tempdataptr = file_load_resource(2, "sdtitl");
	wndsprite = sprite_make_wnd(0x140, 0xC8, 0x0F);
	result = run_intro();
	sprite_free_wnd(wndsprite);
	mmgr_free((legacy_s8 far*)tempdataptr);

	if (result == 0) {
		result = setup_intro();
		if (result == 0) {
			tempdataptr = file_load_resource(2, "sdcred");
			wndsprite = sprite_make_wnd(0x140, 0xC8, 0x0F);
			sprite_copy_wnd_to_1_clear();
			sprite_blit_to_video(wndsprite, 0);
			result = load_intro_resources();
			sprite_free_wnd(wndsprite);
			mmgr_free((legacy_s8 far*)tempdataptr);
		}
	}

	audio_unload();
	return result;
}

extern void sprite_1_unk4(legacy_s16 x, legacy_s16 y, legacy_s16 width, legacy_s16 height, legacy_s16 color);

legacy_s16 mouse_timer_sprite_unk(legacy_s16 item_index, const legacy_s16* x_values,
	const legacy_s16* width_values, const legacy_s16* y_values, const legacy_s16* height_values,
	legacy_s16 second_state, legacy_s16 first_state)
{
	legacy_u16 delta;
	legacy_u16 animation_counter;
	legacy_s16 selected_state;

	delta = (legacy_u16)timer_get_delta_alt();
	animation_counter = LEGACY_U16_WRAP_ADD(word_45D1C, delta);
	while (LEGACY_S16_FROM_BITS(animation_counter) > 60)
		animation_counter = LEGACY_U16_WRAP_SUB(animation_counter, 60U);
	word_45D1C = animation_counter;
	selected_state = LEGACY_S16_FROM_BITS(animation_counter) > 30 ?
		LEGACY_S16_FROM_BITS((legacy_u16)second_state) :
		LEGACY_S16_FROM_BITS((legacy_u16)first_state);
	if (word_45D06 != selected_state) {
		word_45D06 = selected_state;
		mouse_draw_opaque_check();
		sprite_1_unk4(x_values[item_index], y_values[item_index],
			width_values[item_index], height_values[item_index],
			selected_state);
		mouse_draw_transparent_check();
	}
	return LEGACY_S16_FROM_BITS(delta);
}

extern legacy_s8 aSdmsel[];
extern legacy_s8 aScrn[];
extern legacy_s16 menu_buttons_x1[];
extern legacy_s16 menu_buttons_x2[];
extern legacy_s16 menu_buttons_y1[];
extern legacy_s16 menu_buttons_y2[];
extern legacy_s16 word_407CE;
extern legacy_s16 word_407D0;
extern legacy_s16 word_407F4;
extern legacy_s16 word_407F6;
extern legacy_s16 word_407F8;
extern legacy_s16 trackmenu_buttons_x1[];
extern legacy_s16 trackmenu_buttons_x2[];
extern legacy_s16 trackmenu_buttons_y1[];
extern legacy_s16 trackmenu_buttons_y2[];
extern legacy_s8 aMisc[];
extern legacy_s8 aSdosel[];
extern legacy_s8 aOpp0opp1opp2op[];
extern legacy_s8 aScrn_0[];
extern legacy_s8 aBla[];
extern legacy_s8 aBnx[];
extern legacy_s8 aBcl[];
extern legacy_s8 aBca[];
extern legacy_s8 aBdo[];
extern legacy_s8 aClip[];
extern legacy_s8 aDes_0[];
extern legacy_s8 aRac[];
extern legacy_s8 aOpp1[];
extern legacy_s16 opponentmenu_buttons_x1[];
extern legacy_s16 opponentmenu_buttons_x2[];
extern legacy_s16 opponentmenu_buttons_y1[];
extern legacy_s16 opponentmenu_buttons_y2[];
extern legacy_s8 far* opp_res;
extern legacy_s8 far* oppresources[7];
extern legacy_s8 a_res_0[];
extern legacy_s8 aCar[];
extern legacy_s8 aSdcsel[];
extern legacy_s8 aMisc_0[];
extern legacy_s8 aGrap[];
extern legacy_s8 a150[];
extern legacy_s8 a100[];
extern legacy_s8 a50[];
extern legacy_s8 a0[];
extern legacy_s8 a02040[];
extern legacy_s8 aBdo_0[];
extern legacy_s8 aBnx_0[];
extern legacy_s8 aBla_0[];
extern legacy_s8 aBau[];
extern legacy_s8 aBma[];
extern legacy_s8 aBco[];
extern legacy_s8 aDes_1[];
extern legacy_s8 aStop_1[];
extern legacy_s8 aBau_0[];
extern legacy_s8 aBma_0[];
extern legacy_s16 carmenu_buttons_x1[];
extern legacy_s16 carmenu_buttons_x2[];
extern legacy_s16 carmenu_buttons_y1[];
extern legacy_s16 carmenu_buttons_y2[];
extern struct RECTANGLE carmenu_cliprect;
extern struct RECTANGLE rect_unk16;
extern struct VECTOR carmenu_carpos;
extern legacy_s8 backlights_paint_override;
extern legacy_s8 aMisc_2[];
extern legacy_s8 aElt[];
extern legacy_s8 aCon[];
extern legacy_s8 aPpt[];
extern legacy_s8 aDnf[];
extern legacy_s8 aOlt[];
extern legacy_s8 aDnf_0[];
extern legacy_s8 aOwt[];
extern legacy_s8 aOlt_0[];
extern legacy_s8 aVict[];
extern legacy_s8 aSkidms_1[];
extern legacy_s8 aSkidvict[];
extern legacy_s8 aOver[];
extern legacy_s8 aSkidms_2[];
extern legacy_s8 aSkidover[];
extern legacy_s8 aAvs[];
extern legacy_s8 aMph[];
extern legacy_s8 aImp[];
extern legacy_s8 aMph_0[];
extern legacy_s8 aTop[];
extern legacy_s8 aMph_1[];
extern legacy_s8 aJum[];
extern legacy_s8 aWinn[];
extern legacy_s8 aLose[];
extern legacy_s8 a_trk_5[];
extern legacy_s8 aIhd[];
extern legacy_s8 aD4a[];
extern legacy_s8 aBct[];
extern legacy_s8 aInh[];
extern legacy_s8 aInh_0[];
extern legacy_s8 aHna[];
extern legacy_s8 aBev[];
extern legacy_s8 aBhi[];
extern legacy_s8 aBrp[];
extern legacy_s8 aBra[];
extern legacy_s8 aBdr[];
extern legacy_s8 aBmm_0[];
extern legacy_s8 aOpp2win[];
extern legacy_s8 aOpp2lose[];
extern legacy_s8 aOp01[];
extern legacy_s16 word_3BCDE[];
extern legacy_s16 word_3BCE4[];
extern legacy_s16 word_3BCEC[];
extern legacy_s16 word_3BCF6[];
extern legacy_s16 hiscore_buttons_y1[];
extern legacy_s16 hiscore_buttons_y2[];
extern legacy_s16 word_40D3A;
extern legacy_s16 word_40D3C;
extern legacy_s16 word_40D3E;
extern legacy_s16 word_40D40;
extern legacy_s16 end_hiscore_random;
extern legacy_s16 word_40D44;
extern legacy_s16 word_407D2;
extern legacy_s32 gState_travDist;
extern legacy_s16 gState_total_finish_time;
extern legacy_s16 gState_144;
extern legacy_s16 gState_pEndFrame;
extern legacy_s16 gState_oEndFrame;
extern legacy_s16 gState_penalty;
extern legacy_s16 gState_impactSpeed;
extern legacy_s16 gState_topSpeed;
extern legacy_s16 gState_jumpCount;
extern legacy_s8 aCred[];
extern legacy_s8 aArowarrwarw1ar[];
extern legacy_s8 aCre[];
extern legacy_s8 aGds0[];
extern legacy_s8 aGds1[];
extern legacy_s8 aDes[];
extern legacy_s8 aGdon[];
extern legacy_s8 aGkev[];
extern legacy_s8 aGbra[];
extern legacy_s8 aGrob[];
extern legacy_s8 aGsta[];
extern legacy_s8 aMus[];
extern legacy_s8 aGmsy[];
extern legacy_s8 aGkri[];
extern legacy_s8 aGbri[];
extern legacy_s8 aPro[];
extern legacy_s8 aGkev_0[];
extern legacy_s8 aOpr[];
extern legacy_s8 aGbra_0[];
extern legacy_s8 aGric[];
extern legacy_s8 aArt[];
extern legacy_s8 aGmsm[];
extern legacy_s8 aGdav[];
extern legacy_s8 aGnic[];
extern legacy_s8 aGkev_1[];
extern legacy_s16 word_407D4;
extern legacy_s16 word_407D6;
extern legacy_s16 word_407D8;
extern legacy_s16 word_407DA;
extern legacy_s16 word_407DC;
extern legacy_s16 word_407DE;
extern legacy_s16 word_407E0;
extern legacy_s16 word_407E2;
extern legacy_s16 word_407E4;
extern legacy_s16 word_407E6;
extern legacy_s16 word_407E8;
extern legacy_s16 word_407EA;

void load_skybox(legacy_s8 skybox_index);
void unload_skybox(void);
void draw_track_preview(void);
legacy_s16 track_setup(void);
void load_tracks_menu_shapes(void);
void draw_button(legacy_s8 far* text, legacy_s16 x, legacy_s16 y, legacy_s16 width, legacy_s16 height,
	legacy_s16 top_color, legacy_s16 bottom_color, legacy_s16 fill_color, legacy_s16 font_color);
legacy_s16 highscore_write_a(legacy_s16 create_default);
extern struct SHAPE2D far* tracksmenushapes2[];
extern struct SHAPE2D far* tracksmenushapes3[];
extern legacy_s16 word_407F2;
extern legacy_s16 track_pieces_counter;
extern legacy_u8 byte_45D90;
extern legacy_u8 byte_45E16;

static legacy_u8 track_editor_palette_tile(legacy_u8 page,
	legacy_u8 row, legacy_u8 column)
{
	return pboxshape[(legacy_u16)page * 36U +
		(legacy_u16)row * 6U + column];
}

static legacy_u8 track_editor_map_tile(legacy_u8 column, legacy_u8 row)
{
	legacy_u16 source_index;
	legacy_u8 tile;

	source_index = LEGACY_U16_WRAP_ADD((legacy_u16)trackrows[row], column);
	tile = td14_elem_map_main[source_index];
	if (tile == 0xFDU) {
		source_index = LEGACY_U16_WRAP_SUB(
			LEGACY_U16_WRAP_ADD(track_menu_previous_row(row), column), 1U);
		tile = td14_elem_map_main[source_index];
	} else if (tile == 0xFEU) {
		source_index = LEGACY_U16_WRAP_ADD(
			track_menu_previous_row(row), column);
		tile = td14_elem_map_main[source_index];
	} else if (tile == 0xFFU) {
		tile = td14_elem_map_main[LEGACY_U16_WRAP_SUB(source_index, 1U)];
	}
	return tile;
}

static void track_editor_show_message(legacy_s8 far* text_resource,
	const legacy_s8* resource_id)
{
	show_dialog(1, 1, locate_text_res(text_resource, (legacy_s8*)resource_id),
		0xFFFFU, 0xFFFFU, performGraphColor, 0, 0);
}

void load_tracks_menu_shapes(void)
{
	static legacy_s8 terrain_shape_names[] =
		"flatlakelak1lak2lak3lak4highgoungouwgousgouegou1gou2gou3gou4gou5gou6gou7gou8";
	static legacy_s8 cursor_shape_names[] = "crs0crs1crs2crs3";
	static legacy_s8 under_cursor_shape_names[] = "ucr0ucr1ucr2ucr3";
	static const legacy_s8 error_resource_ids[] =
		"eokenseieemseedewwefuenpestejsejdeteewaefteat";
	static legacy_s16 button_x1[5] = { 9, 202, 220, 8, 220 };
	static legacy_s16 button_x2[5] = { 199, 206, 315, 199, 315 };
	static legacy_s16 button_y1[5] = { 181, 4, 132, 4, 36 };
	static legacy_s16 button_y2[5] = { 187, 179, 139, 179, 187 };
	static const legacy_u16 page_keys[10] = {
		0x3B00U, 0x3C00U, 0x3D00U, 0x3E00U, 0x3F00U,
		0x4000U, 0x4100U, 0x4200U, 0x4300U, 0x4400U
	};
	static const legacy_u8 maximum_columns[2] = { 30, 6 };
	static const legacy_u8 maximum_rows[2] = { 29, 9 };
	legacy_u8 cached_track[132];
	legacy_u8 cached_terrain[132];
	struct SPRITE far* cursor_sprites[4];
	legacy_s8 far* shape_resource;
	legacy_s8 far* text_resource;
	legacy_s8 far* shape_name_resource;
	legacy_s8 far* mask_name_resource;
	legacy_s8 far* text_name_resource;
	legacy_s8 far* text;
	struct SHAPE2D far* terrain_shape;
	legacy_u8 selection_column[2];
	legacy_u8 selection_row[2];
	legacy_u8 map_column_offset;
	legacy_u8 map_row_offset;
	legacy_u8 previous_column_offset;
	legacy_u8 previous_row_offset;
	legacy_u8 page;
	legacy_u8 previous_page;
	legacy_u8 selected_tile;
	legacy_u8 saved_tile;
	legacy_u8 last_column;
	legacy_u8 last_row;
	legacy_u8 hovered_tile;
	legacy_u8 previous_hovered_tile;
	legacy_u8 multi_tile;
	legacy_u8 tile_width;
	legacy_u8 tile_height;
	legacy_u8 cursor_width;
	legacy_u8 cursor_height;
	legacy_u8 map_dirty;
	legacy_u8 palette_dirty;
	legacy_u8 scrollbars_dirty;
	legacy_u8 validate_track;
	legacy_u8 validation_error;
	legacy_u8 track_changed;
	legacy_u8 menu_active;
	legacy_u8 blit_mode;
	legacy_u8 cursor_drawn;
	legacy_u8 focus;
	legacy_u8 blink_focus;
	legacy_u8 hit;
	legacy_u8 clicked_column;
	legacy_u8 clicked_row;
	legacy_u8 save_status;
	legacy_u8 dialog_result;
	legacy_u8 step;
	legacy_u8 value;
	legacy_u8 animation_saved_tile;
	legacy_u16 index;
	legacy_u16 source_index;
	legacy_u16 path_animation_index;
	legacy_u16 blink_timer;
	legacy_u16 delta;
	legacy_u16 key;
	legacy_u16 key_index;
	legacy_s16 label_width;
	legacy_s16 previous_label_width;
	legacy_s16 cursor_x;
	legacy_s16 cursor_y;
	legacy_s16 result;
	legacy_s16 write_result;
	legacy_s8 terrain_id[5];
	legacy_s8* resource_id;

	shape_resource = (legacy_s8 far*)file_load_shape2d_fatal("sdtedit");
	locate_many_resources(shape_resource, terrain_shape_names,
		(legacy_s8 far**)tracksmenushapes1);
	locate_many_resources(shape_resource, cursor_shape_names,
		(legacy_s8 far**)tracksmenushapes2);
	locate_many_resources(shape_resource, under_cursor_shape_names,
		(legacy_s8 far**)tracksmenushapes3);
	for (index = 0; index < 4U; index++) {
		cursor_sprites[index] = sprite_make_wnd(
			LEGACY_U16_WRAP_MUL(tracksmenushapes2[index]->s2d_width,
				(legacy_u16)video_flag1_is1),
			tracksmenushapes2[index]->s2d_height, 0x0FU);
	}

	text_resource = (legacy_s8 far*)file_load_resfile("tedit");
	wndsprite = sprite_make_wnd(0x140U, 0xC8U, 0x0FU);
	pboxshape = (legacy_u8 far*)locate_shape_alt(text_resource, "pbox");
	shape_name_resource = locate_shape_alt(text_resource, "snam");
	mask_name_resource = locate_shape_alt(text_resource, "mnam");
	text_name_resource = locate_shape_alt(text_resource, "tnam");

	for (index = 0; index < 132U; index++) {
		cached_track[index] = 0xFFU;
		cached_terrain[index] = 0xFFU;
	}
	for (index = 0; index < 186U; index++) {
		__fmemcpy(&resID_byte1, shape_name_resource + index * 4U, 4U);
		tracksmenushape2dunk[index] = (struct SHAPE2D far*)
			locate_shape_fatal(shape_resource, &resID_byte1);
		__fmemcpy(&resID_byte1, mask_name_resource + index * 4U, 4U);
		tracksmenushape2dunk2[index] = (struct SHAPE2D far*)
			locate_shape_fatal(shape_resource, &resID_byte1);
	}

	last_column = 0xFFU;
	last_row = 0xFFU;
	saved_tile = 0;
	selected_tile = 0;
	previous_page = 0xFFU;
	map_dirty = 1;
	palette_dirty = 1;
	scrollbars_dirty = 1;
	validate_track = 1;
	validation_error = 0;
	track_changed = 0;
	menu_active = 1;
	blit_mode = 0xFFU;
	page = 1;
	selection_column[0] = byte_45D90;
	selection_row[0] = byte_45E16;
	selection_column[1] = 0;
	selection_row[1] = 7;
	map_column_offset = 0;
	map_row_offset = 0;
	previous_column_offset = 0xFFU;
	previous_row_offset = 0xFFU;
	previous_hovered_tile = 0xFFU;
	previous_label_width = 0;
	focus = 0;
	path_animation_index = 0;

	sprite_copy_wnd_to_1_clear();
	draw_button(locate_text_res(text_resource, "bti"),
		0xD9, 3, 0x66, 0x16, word_407F4, word_407F6, word_407F8, 0);
	draw_lines_unk(5, 0, 0xCE, 0xBE, 0x0B, 9, 1);
	draw_lines_unk(0xD9, 0x20, 0x66, 0x9E, 0x0B, 9, 1);
	draw_button(locate_text_res(text_resource, "bsc"),
		0xDD, 0x8C, 0x5E, 0x0E, word_407F4, word_407F6, word_407F8, 0);
	draw_button(locate_text_res(text_resource, "blo"),
		0xDD, 0x9C, 0x2E, 0x0E, word_407F4, word_407F6, word_407F8, 0);
	draw_button(locate_text_res(text_resource, "bsa"),
		0xDD, 0xAC, 0x2E, 0x0E, word_407F4, word_407F6, word_407F8, 0);
	draw_button(locate_text_res(text_resource, "bcl"),
		0x10D, 0x9C, 0x2E, 0x0E, word_407F4, word_407F6, word_407F8, 0);
	draw_button(locate_text_res(text_resource, "bex"),
		0x10D, 0xAC, 0x2E, 0x0E, word_407F4, word_407F6, word_407F8, 0);

	while (menu_active != 0) {
		if (palette_dirty != 0 || page != previous_page) {
			tile_width = 1;
			tile_height = 1;
			multi_tile = 0;
			if (page != 0) {
				multi_tile = (legacy_u8)
					trkObjectList[selected_tile].ss_multiTileFlag;
				if (multi_tile == 1U) {
					tile_height = 2;
					multi_tile = 1;
				} else if (multi_tile == 2U) {
					tile_width = 2;
					multi_tile = 2;
				} else if (multi_tile == 3U) {
					tile_width = 2;
					tile_height = 2;
					multi_tile = 3;
				}
			}

			if (focus == 0) {
				if (selection_column[0] == 29U && tile_width == 2U)
					selection_column[0]--;
				if (selection_row[0] == 29U && tile_height == 2U)
					selection_row[0]--;
				while ((legacy_s16)(selection_column[0] - map_column_offset +
					tile_width) > 12)
					map_column_offset++;
				while (selection_column[0] < map_column_offset)
					map_column_offset--;
				while ((legacy_s16)(selection_row[0] - map_row_offset +
					tile_height) > 11)
					map_row_offset++;
				while (selection_row[0] < map_row_offset)
					map_row_offset--;
				if (map_column_offset != previous_column_offset ||
					map_row_offset != previous_row_offset) {
					previous_column_offset = map_column_offset;
					previous_row_offset = map_row_offset;
					map_dirty = 1;
					scrollbars_dirty = 1;
				}
			}

			if (page != previous_page) {
				palette_dirty = 1;
				previous_page = page;
				while (selection_row[1] < 6U &&
					track_editor_palette_tile(page,
						selection_row[1], selection_column[1]) >= 0xFEU) {
					value = track_editor_palette_tile(page,
						selection_row[1], selection_column[1]);
					if (value == 0xFFU)
						selection_column[1]--;
					else
						selection_row[1]--;
				}
				sprite_copy_wnd_to_1();
				preRender_icons(page);
				if (page == 0)
					mouse_track_op(0, 0xDD, 0x5F, 0x85, 5, 0, 1, 1);
				else
					mouse_track_op(0, 0xDD, 0x5F, 0x85, 5,
						page - 1U, 1, 10);
			}
		}

		if (validate_track != 0) {
			validate_track = 0;
			validation_error = (legacy_u8)sub_2C81C();
		}

		if (map_dirty != 0 || palette_dirty != 0) {
			sprite_copy_wnd_to_1();
			if (map_dirty != 0) {
				map_dirty = 0;
				if (scrollbars_dirty != 0) {
					scrollbars_dirty = 0;
					mouse_track_op(0, 9, 0xC0, 0xB5, 5,
						map_column_offset, 12, 30);
					mouse_track_op(0, 0xCA, 5, 4, 0xB0,
						map_row_offset, 11, 30);
				}
				sprite_set_1_size(8, 0xC8, 4, 0xB3);
				draw_2DtrackMap(map_column_offset, map_row_offset,
					cached_track, cached_terrain);
				sprite_set_1_size(0, 0x140, 0, 0xC8);
			}

			if (palette_dirty != 0) {
				palette_dirty = 0;
				sprite_set_1_from_argptr(cursor_sprites[multi_tile]);
				if (page == 0) {
					sprite_shape_to_1(tracksmenushapes1[selected_tile], 0, 0);
					preRender_line(1, 0, 0x0F, 0, performGraphColor);
					preRender_line(1, 0x0E, 0x0F, 0x0E, performGraphColor);
					preRender_line(1, 0, 1, 0x0E, performGraphColor);
					preRender_line(0x0F, 0, 0x0F, 0x0E, performGraphColor);
				} else {
					sprite_shape_to_1(tracksmenushapes2[multi_tile], 0, 0);
					if (selected_tile != 0) {
						putpixel_iconMask(tracksmenushape2dunk2[selected_tile], 0, 0);
						putpixel_iconFillings(tracksmenushape2dunk[selected_tile], 0, 0);
					}
				}
			}

			sprite_blit_to_video(wndsprite, LEGACY_S8_FROM_BITS(blit_mode));
			blit_mode = 0xFEU;
			previous_hovered_tile = 0xFFU;
		}

		sprite_copy_2_to_1_2();
		if (focus == 0) {
			cursor_width = (legacy_u8)(tile_width << 4);
			cursor_height = (legacy_u8)(tile_height << 4);
			cursor_x = (legacy_s16)((selection_column[0] -
				map_column_offset) * 16U + 8U);
			cursor_y = (legacy_s16)((selection_row[0] -
				map_row_offset) * 16U + 4U);
			hovered_tile = track_editor_map_tile(
				selection_column[0], selection_row[0]);
		} else {
			cursor_width = 0x10U;
			cursor_height = 0x10U;
			cursor_y = (legacy_s16)(selection_row[1] * 16U + 0x24U);
			if (selection_row[1] == 6U) {
				cursor_x = 0xDC;
				cursor_width = 0x60U;
				cursor_height = 8U;
			} else if (selection_row[1] == 7U) {
				selection_column[1] = 0;
				cursor_x = 0xDC;
				cursor_width = 0x60U;
				cursor_y = LEGACY_S16_WRAP_SUB(cursor_y, 8);
			} else if (selection_row[1] > 7U) {
				cursor_y = LEGACY_S16_WRAP_SUB(cursor_y, 8);
				selection_column[1] = selection_column[1] < 3U ? 0U : 3U;
				cursor_x = (legacy_s16)(selection_column[1] * 16U + 0xDCU);
				cursor_width = 0x30U;
			} else {
				cursor_x = (legacy_s16)(selection_column[1] * 16U + 0xDCU);
				if (track_editor_palette_tile(page, selection_row[1],
					selection_column[1] + 6U) == 0xFEU)
					cursor_height = 0x20U;
				if (selection_column[1] < 5U &&
					track_editor_palette_tile(page, selection_row[1],
						selection_column[1] + 1U) == 0xFFU)
					cursor_width = 0x20U;
			}
			hovered_tile = selection_row[1] < 6U ?
				track_editor_palette_tile(page, selection_row[1],
					selection_column[1]) : 0;
			if (hovered_tile >= 0xFDU || page == 0)
				hovered_tile = 0;
		}

		if (hovered_tile != previous_hovered_tile) {
			mouse_draw_opaque_check();
			font_set_unk(dialog_fnt_colour, 0);
			resource_id = &resID_byte1;
			__fmemcpy(resource_id, text_name_resource +
				(legacy_u16)hovered_tile * 3U, 3U);
			resource_id[3] = 0;
			text = locate_text_res(text_resource, resource_id);
			copy_string(resource_id, text);
			label_width = (legacy_s16)font_op2(resource_id);
			sub_345BC(resource_id, 8, 0xC0);
			if (previous_label_width > label_width) {
				sprite_1_unk(LEGACY_S16_WRAP_ADD(label_width, 8),
					0xC0, LEGACY_S16_WRAP_SUB(previous_label_width,
						label_width), 8, 0);
			}
			mouse_draw_transparent_check();
			previous_label_width = label_width;
			previous_hovered_tile = hovered_tile;
		}

		if (validation_error != 0) {
			resource_id = (legacy_s8*)error_resource_ids +
				(legacy_u16)validation_error * 3U;
			__fmemcpy(&resID_byte1, resource_id, 3U);
			*(&resID_byte1 + 3) = 0;
			track_editor_show_message(text_resource, &resID_byte1);
			validation_error = 0;
		}

		blink_timer = 0x63U;
		cursor_drawn = 0;
		mouse_draw_opaque_check();
		blink_focus = focus;
		if (blink_focus == 0)
			sprite_clear_shape_alt(tracksmenushapes3[multi_tile],
				cursor_x, cursor_y);

		key = 0;
		while (key == 0) {
			if (blink_timer > 0x0FU) {
				mouse_draw_opaque_check();
				if (blink_focus == 0) {
					if (cursor_drawn != 0)
						sprite_shape_to_1(tracksmenushapes3[multi_tile],
							cursor_x, cursor_y);
					else
						sprite_shape_to_1(
							cursor_sprites[multi_tile]->sprite_bitmapptr,
							cursor_x, cursor_y);
				} else {
					sub_3702E(cursor_x, cursor_y,
						LEGACY_S16_WRAP_ADD(cursor_x, cursor_width),
						LEGACY_S16_WRAP_SUB(
							LEGACY_S16_WRAP_ADD(cursor_y, cursor_height), 1),
						word_407F2);
				}
				mouse_draw_transparent_check();
				cursor_drawn ^= 1U;
				blink_timer = 0;
			}

			delta = (legacy_u16)timer_get_delta_alt();
			blink_timer = LEGACY_U16_WRAP_ADD(blink_timer, delta);
			key = (legacy_u16)input_checking(LEGACY_S16_FROM_BITS(delta));
			hit = (legacy_u8)mouse_multi_hittest(5,
				button_x1, button_x2, button_y1, button_y2);
			if (hit != 0xFFU) {
				if (hit == 0U && (mouse_butstate & 3) != 0) {
					focus = 0;
					clicked_column = (legacy_u8)mouse_track_op(1,
						9, 0xC0, 0xB5, 5, map_column_offset, 12, 30);
					selection_column[0] = (legacy_u8)(selection_column[0] +
						clicked_column - map_column_offset);
					map_column_offset = clicked_column;
					key = 1;
				} else if (hit == 1U && (mouse_butstate & 3) != 0) {
					focus = 0;
					clicked_row = (legacy_u8)mouse_track_op(1,
						0xCA, 5, 4, 0xB0, map_row_offset, 11, 30);
					selection_row[0] = (legacy_u8)(selection_row[0] +
						clicked_row - map_row_offset);
					map_row_offset = clicked_row;
					key = 1;
				} else if (hit == 2U) {
					if ((mouse_butstate & 3) != 0) {
						focus = 1;
						page = (legacy_u8)mouse_track_op(1,
							0xDD, 0x5F, 0x85, 5, page - 1U, 1, 10) + 1U;
						key = 1;
					}
				} else if (hit == 3U) {
					clicked_column = (legacy_u8)((mouse_xpos - 8) / 16);
					clicked_row = (legacy_u8)((mouse_ypos - 4) / 16);
					if (page != 0) {
						if (clicked_row == 10U &&
							((legacy_u8)trkObjectList[selected_tile].ss_multiTileFlag & 1U) != 0)
							clicked_row--;
						if (clicked_column == 11U &&
							((legacy_u8)trkObjectList[selected_tile].ss_multiTileFlag & 2U) != 0)
							clicked_column--;
					}
					clicked_column = (legacy_u8)(clicked_column + map_column_offset);
					clicked_row = (legacy_u8)(clicked_row + map_row_offset);
					if (focus != 0 ||
						selection_column[0] != clicked_column ||
						selection_row[0] != clicked_row) {
						focus = 0;
						selection_column[0] = clicked_column;
						selection_row[0] = clicked_row;
						key = 1;
					}
					if (key == 0x20U)
						key = 0x0DU;
				} else if (hit == 4U) {
					clicked_column = (legacy_u8)((mouse_xpos - 0xDC) / 16);
					clicked_row = (legacy_u8)((mouse_ypos - 0x24) / 16);
					if (clicked_row < 6U) {
						if (track_editor_palette_tile(page, clicked_row,
							clicked_column) == 0xFEU)
							clicked_row--;
						if (track_editor_palette_tile(page, clicked_row,
							clicked_column) == 0xFFU)
							clicked_column--;
					} else {
						clicked_row = (legacy_u8)((mouse_ypos - 0x1C) / 16);
						if (clicked_row == 7U)
							clicked_column = 0;
						else if (clicked_column >= 3U)
							clicked_column = 3;
						else
							clicked_column = 0;
					}
					if (focus == 0 ||
						selection_column[1] != clicked_column ||
						selection_row[1] != clicked_row) {
						selection_column[1] = clicked_column;
						selection_row[1] = clicked_row;
						focus = 1;
						key = 1;
					}
					if (key == 0x20U)
						key = 0x0DU;
				}
			}
			if (key == 1U)
				last_column = 0xFFU;
			if (key == 0 && path_animation_index != 0)
				key = 1;
		}
		if (path_animation_index != 0)
			(void)timer_get_counter_unk(10UL);

		if (cursor_drawn != 0) {
			mouse_draw_opaque_check();
			if (blink_focus == 0) {
				sprite_shape_to_1(tracksmenushapes3[multi_tile],
					cursor_x, cursor_y);
			} else {
				sub_3702E(cursor_x, cursor_y,
					LEGACY_S16_WRAP_ADD(cursor_x, cursor_width),
					LEGACY_S16_WRAP_SUB(
						LEGACY_S16_WRAP_ADD(cursor_y, cursor_height), 1),
					word_407F2);
			}
			mouse_draw_transparent_check();
		}

		if (path_animation_index != 0) {
			if (key == 1U && focus == 0)
				path_animation_index = (legacy_u16)
					(track_pieces_counter - 1);
			selection_column[0] = (legacy_u8)
				td21_col_from_path[path_animation_index];
			selection_row[0] = (legacy_u8)
				td22_row_from_path[path_animation_index];
			selected_tile = track_editor_map_tile(
				selection_column[0], selection_row[0]);
			map_dirty = 1;
			palette_dirty = 1;
			path_animation_index++;
			if (path_animation_index >= (legacy_u16)track_pieces_counter) {
				selected_tile = animation_saved_tile;
				path_animation_index = 0;
			}
			goto track_editor_next;
		}

		for (key_index = 0; key_index < 10U; key_index++) {
			if (key == page_keys[key_index]) {
				page = (legacy_u8)(key_index + 1U);
				key = 0;
				break;
			}
		}

		if (key == 0x63U || key == 0x43U) {
			result = (legacy_s8)track_setup();
			resource_id = (legacy_s8*)error_resource_ids +
				(legacy_u16)(legacy_u8)result * 3U;
			__fmemcpy(&resID_byte1, resource_id, 3U);
			*(&resID_byte1 + 3) = 0;
			track_editor_show_message(text_resource, &resID_byte1);
			if (result > 1) {
				focus = 0;
				if (track_pieces_counter == 0) {
					selection_column[0] = byte_45D90;
					selection_row[0] = byte_45E16;
				} else {
					animation_saved_tile = selected_tile;
					selection_column[0] = (legacy_u8)td21_col_from_path[0];
					selection_row[0] = (legacy_u8)td22_row_from_path[0];
					selected_tile = track_editor_map_tile(
						selection_column[0], selection_row[0]);
					path_animation_index = 1;
					palette_dirty = 1;
				}
			}
			check_input();
		} else if (key == 0x0DU) {
			if (focus != 0) {
				if (selection_row[1] < 6U) {
					selected_tile = track_editor_palette_tile(page,
						selection_row[1], selection_column[1]);
					if (page != 0) {
						multi_tile = (legacy_u8)
							trkObjectList[selected_tile].ss_multiTileFlag;
						if ((multi_tile & 1U) != 0 &&
							selection_row[0] - map_row_offset == 10U)
							selection_row[0]--;
						if ((multi_tile & 2U) != 0 &&
							selection_column[0] - map_column_offset == 11U)
							selection_column[0]--;
					}
					palette_dirty = 1;
					focus = 0;
				} else if (selection_row[1] == 6U) {
					page++;
					if (page > 10U)
						page = 1;
				} else if (selection_row[1] == 7U) {
					dialog_result = LEGACY_S8_FROM_BITS(show_dialog(2, 1,
						locate_text_res(text_resource, "mss"),
						0xFFFFU, 0xFFFFU, dialogarg2, 0,
						td14_elem_map_main[0x384]));
					if (dialog_result != 0xFFU && dialog_result != 5U) {
						td14_elem_map_main[0x384] = dialog_result;
						map_dirty = 1;
						track_changed = 1;
					}
				} else if (selection_row[1] == 8U &&
					selection_column[1] != 0) {
					dialog_result = LEGACY_S8_FROM_BITS(show_dialog(2, 1,
						locate_text_res(text_resource, "men"),
						0xFFFFU, 0xFFFFU, dialogarg2, 0, 0));
					if (dialog_result != 0xFFU && dialog_result != 5U) {
						for (index = 0; index < 900U; index++)
							td14_elem_map_main[index] = 0;
						terrain_id[0] = 't';
						terrain_id[1] = 'e';
						terrain_id[2] = 'r';
						terrain_id[3] = (legacy_s8)('0' + dialog_result);
						terrain_id[4] = 0;
						terrain_shape = (struct SHAPE2D far*)
							locate_shape_alt(text_resource, terrain_id);
						__fmemcpy(td15_terr_map_main, terrain_shape, 901U);
						gameconfig.game_trackname[0] = 0;
						map_dirty = 1;
						track_changed = 1;
					}
				} else if (selection_row[1] == 8U) {
					result = 1;
					if (track_changed != 0) {
						result = (legacy_s16)show_dialog(2, 1,
							locate_text_res(text_resource, "chl"),
							0xFFFFU, 0xFFFFU, performGraphColor, 0, 0);
					}
					if (result == 0)
						goto track_editor_save;
					g_is_busy = 1;
					map_dirty = 1;
					text = locate_text_res((legacy_s8 far*)mainresptr, "trk");
					result = do_fileselect_dialog(byte_3B80C,
						gameconfig.game_trackname, ".trk", text);
					file_build_path(byte_3B80C, gameconfig.game_trackname,
						".trk", g_path_buf);
					if (result > 0) {
						file_read_fatal(g_path_buf, td14_elem_map_main);
						track_setup();
						focus = 0;
						selection_column[0] = byte_45D90;
						selection_row[0] = byte_45E16;
						track_changed = 0;
						map_dirty = 1;
					}
					g_is_busy = 0;
				} else if (selection_column[1] == 0) {
track_editor_save:
					save_status = 0;
					g_is_busy = 1;
					while (save_status == 0) {
						sprite_copy_2_to_1_2();
						map_dirty = 1;
						text = locate_text_res((legacy_s8 far*)mainresptr, "trk");
						if (do_savefile_dialog(byte_3B80C,
							gameconfig.game_trackname, text) == 0) {
							save_status = 0xFFU;
							break;
						}
						file_build_path(byte_3B80C,
							gameconfig.game_trackname, ".trk", g_path_buf);
						save_status = 1;
						if (file_find(g_path_buf) != 0) {
							result = LEGACY_S16_FROM_BITS(show_dialog(2, 1,
								locate_text_res((legacy_s8 far*)mainresptr, "fex"),
								0xFFFFU, 0xFFFFU, performGraphColor, 0, 0));
							if (result == -1) {
								save_status = 0xFFU;
								break;
							}
							if (result == 0) {
								save_status = 0;
								continue;
							}
						}
						write_result = file_write_fatal(g_path_buf,
							td14_elem_map_main, 0x70AUL);
						if (write_result == 0)
							highscore_write_a(1);
						if (write_result != 0) {
							track_editor_show_message((legacy_s8 far*)mainresptr, "ser");
							save_status = 0;
						} else {
							track_changed = 0;
						}
					}
					g_is_busy = 0;
				} else {
					result = 1;
					if (track_changed != 0) {
						result = (legacy_s16)show_dialog(2, 1,
							locate_text_res(text_resource, "chx"),
							0xFFFFU, 0xFFFFU, performGraphColor, 0, 0);
					}
					if (result == 0)
						goto track_editor_save;
					menu_active = 0;
				}
			} else if (page == 0) {
				if (selection_column[0] == last_column &&
					selection_row[0] == last_row) {
					value = selected_tile;
					selected_tile = saved_tile;
					saved_tile = value;
					palette_dirty = 1;
				} else {
					source_index = LEGACY_U16_WRAP_ADD(
						(legacy_u16)terrainrows[selection_row[0]],
						selection_column[0]);
					saved_tile = td15_terr_map_main[source_index];
					last_column = selection_column[0];
					last_row = selection_row[0];
				}
				source_index = LEGACY_U16_WRAP_ADD(
					(legacy_u16)terrainrows[last_row], last_column);
				td15_terr_map_main[source_index] = selected_tile;
				track_changed = 1;
				validate_track = 1;
				map_dirty = 1;
			} else {
				multi_tile = (legacy_u8)
					trkObjectList[selected_tile].ss_multiTileFlag;
				if (((multi_tile & 1U) != 0 && selection_row[0] > 28U) ||
					((multi_tile & 2U) != 0 && selection_column[0] > 28U))
					goto track_editor_next;
				if (selection_column[0] == last_column &&
					selection_row[0] == last_row) {
					value = selected_tile;
					selected_tile = saved_tile;
					saved_tile = value;
					palette_dirty = 1;
				} else {
					source_index = LEGACY_U16_WRAP_ADD(
						(legacy_u16)trackrows[selection_row[0]],
						selection_column[0]);
					saved_tile = td14_elem_map_main[source_index];
					if (saved_tile >= 0xFDU)
						saved_tile = 0;
					last_column = selection_column[0];
					last_row = selection_row[0];
				}
				source_index = LEGACY_U16_WRAP_ADD(
					(legacy_u16)trackrows[last_row], last_column);
				td14_elem_map_main[source_index] = selected_tile;
				track_changed = 1;
				validate_track = 1;
				map_dirty = 1;
				if (multi_tile == 1U) {
					td14_elem_map_main[LEGACY_U16_WRAP_ADD(
						(legacy_u16)trackrows[last_row + 1U],
						last_column)] = 0xFEU;
				} else if (multi_tile == 2U) {
					td14_elem_map_main[LEGACY_U16_WRAP_ADD(
						source_index, 1U)] = 0xFFU;
				} else if (multi_tile == 3U) {
					td14_elem_map_main[LEGACY_U16_WRAP_ADD(
						source_index, 1U)] = 0xFFU;
					source_index = LEGACY_U16_WRAP_ADD(
						(legacy_u16)trackrows[last_row + 1U], last_column);
					td14_elem_map_main[source_index] = 0xFEU;
					td14_elem_map_main[LEGACY_U16_WRAP_ADD(
						source_index, 1U)] = 0xFDU;
				}
			}
		} else if (key == 0x20U || key == 0x5200U) {
			focus ^= 1U;
		} else if (key == (legacy_u16)'+') {
			if (page < 10U)
				page++;
		} else if (key == (legacy_u16)'-') {
			if (page > 1U)
				page--;
		} else if (key == 0x5400U) {
			page = 0;
			selected_tile = 0;
		} else if (key == 0x4700U) {
			if (focus != 0) {
				selection_row[1] = 0;
			} else {
				if (selection_column[0] == map_column_offset &&
					selection_row[0] == map_row_offset) {
					map_column_offset = 0;
					map_row_offset = 0;
				}
				selection_column[0] = map_column_offset;
				selection_row[0] = map_row_offset;
			}
		} else if (key == 0x4800U) {
			if (selection_row[focus] != 0) {
				last_column = 0xFFU;
				selection_row[focus]--;
				if (focus != 0 && selection_row[1] < 6U) {
					while (track_editor_palette_tile(page,
						selection_row[1], selection_column[1]) >= 0xFEU) {
						value = track_editor_palette_tile(page,
							selection_row[1], selection_column[1]);
						if (value == 0xFFU)
							selection_column[1]--;
						else
							selection_row[1]--;
					}
				}
			}
		} else if (key == 0x5000U) {
			if (selection_row[focus] < maximum_rows[focus]) {
				last_column = 0xFFU;
				selection_row[focus]++;
				if (focus != 0 && selection_row[1] < 6U) {
					value = track_editor_palette_tile(page,
						selection_row[1], selection_column[1]);
					if (value == 0xFFU)
						selection_column[1]--;
					else if (value == 0xFEU)
						selection_row[1]++;
				}
			}
		} else if (key == 0x4B00U) {
			if (focus != 0 && selection_row[1] == 6U) {
				if (page > 1U)
					page--;
			} else if (selection_column[focus] != 0) {
				last_column = 0xFFU;
				selection_column[focus]--;
				if (focus != 0) {
					if (selection_row[1] > 5U) {
						selection_column[1] = 0;
					} else {
						while (track_editor_palette_tile(page,
							selection_row[1], selection_column[1]) >= 0xFEU) {
							value = track_editor_palette_tile(page,
								selection_row[1], selection_column[1]);
							if (value == 0xFFU)
								selection_column[1]--;
							else
								selection_row[1]--;
						}
					}
				}
			}
		} else if (key == 0x4D00U) {
			if (focus != 0 && selection_row[1] == 6U) {
				if (page < 10U)
					page++;
			} else {
				step = focus != 0 && selection_row[1] > 5U ? 3U : 1U;
				while ((legacy_u16)selection_column[focus] + step <
					maximum_columns[focus]) {
					if (focus == 0 || selection_row[1] > 5U)
						break;
					value = track_editor_palette_tile(page,
						selection_row[1],
						(legacy_u8)(selection_column[1] + step));
					if (value < 0xFEU)
						break;
					if (value == 0xFFU)
						step++;
					else
						selection_row[1]--;
				}
				if ((legacy_u16)selection_column[focus] + step <
					maximum_columns[focus]) {
					last_column = 0xFFU;
					selection_column[focus] = (legacy_u8)
						(selection_column[focus] + step);
				}
			}
		}

track_editor_next:
		if (menu_active != 0) {
			palette_dirty = 1;
			map_dirty = 1;
		}
	}

	sprite_free_wnd(wndsprite);
	for (index = 4U; index != 0; index--)
		sprite_free_wnd(cursor_sprites[index - 1U]);
	unload_resource(text_resource);
	mmgr_free(shape_resource);
}
legacy_s8 run_menu(void)
{
	static const legacy_u8 previous_selection[5] = { 0, 1, 2, 4, 0 };
	static const legacy_u8 next_selection[5] = { 3, 0, 1, 4, 2 };
	legacy_s8 far* resource;
	struct SHAPE2D far* shape;
	legacy_u8 selected;
	legacy_u8 previous;
	legacy_u8 blit_mode;
	legacy_u16 elapsed;
	legacy_u16 key;
	legacy_s16 hit;

	selected = 0;
	previous = 0xFFU;
	blit_mode = 0xFFU;
	show_waiting();
	waitflag = 0xB4;
	wndsprite = sprite_make_wnd(0x140U, 0xC8U, 0x0FU);
	resource = (legacy_s8 far*)file_load_resource(2, aSdmsel);
	sprite_copy_wnd_to_1();
	shape = (struct SHAPE2D far*)locate_shape_fatal(resource, aScrn);
	sprite_shape_to_1_alt(shape);
	mmgr_free(resource);

	for (;;) {
		if (selected != previous) {
			previous = selected;
			sprite_copy_wnd_to_1();
			sprite_blit_to_video(wndsprite,
				LEGACY_S8_FROM_BITS(blit_mode));
			blit_mode = 0xFEU;
			sprite_copy_2_to_1_2();
			sub_29772();
		}

		elapsed = (legacy_u16)mouse_timer_sprite_unk(selected,
			menu_buttons_x1, menu_buttons_x2,
			menu_buttons_y1, menu_buttons_y2, word_407CE, word_407D0);
		key = (legacy_u16)input_checking(LEGACY_S16_FROM_BITS(elapsed));
		hit = (legacy_s16)mouse_multi_hittest(5, menu_buttons_x1,
			menu_buttons_x2, menu_buttons_y1, menu_buttons_y2);
		if (hit != -1)
			selected = (legacy_u8)hit;

		idle_counter = LEGACY_U16_WRAP_ADD(idle_counter, elapsed);
		if (LEGACY_S16_FROM_BITS((legacy_u16)idle_counter) > 0x1770) {
			idle_counter = 0;
			idle_expired = (legacy_u8)(idle_expired + 1U);
		}
		if (idle_expired != 0) {
			selected = 0;
			key = 0x0DU;
		}

		if (key == 0)
			continue;
		if (key == 0x0DU || key == 0x20U)
			break;
		if (key == 0x1BU) {
			selected = 0xFFU;
			break;
		}
		if (key == 0x4B00U)
			selected = previous_selection[selected];
		else if (key == 0x4D00U)
			selected = next_selection[selected];
	}

	sprite_free_wnd(wndsprite);
	return LEGACY_S8_FROM_BITS(selected);
}

extern legacy_s16 fontdef_unk_0E;
extern struct RECTANGLE word_42248;
extern struct RECTANGLE word_42250;
extern void font_draw_text(const legacy_s8* text, legacy_s16 x, legacy_s16 y);

void font_set_unk(legacy_s16 color, legacy_s16 unknown)
{
	legacy_u8 far* font_definition;

	font_definition = word_405FE;
	font_definition[0] = (legacy_u8)color;
	font_definition[1] = 0;
	font_definition[2] = (legacy_u8)unknown;
	font_definition[3] = 0;
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
	draw_lines_unk(x, y, width, height, top_color, top_color,
		bottom_color);

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
		(legacy_s16)((legacy_s32)remaining / 2L), 1);
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
		horizontal_offset = (legacy_s16)((legacy_s32)remaining / 2L);
		font_draw_text(line,
			LEGACY_S16_WRAP_ADD(x, horizontal_offset),
			LEGACY_S16_WRAP_ADD(
				LEGACY_S16_WRAP_ADD(y, vertical_offset),
				LEGACY_U16_WRAP_MUL(line_index, 8U)));
		line_index++;
		destination_index = 0;
	}
}

struct RECTANGLE* intro_draw_text(legacy_s8* text, legacy_s16 x, legacy_s16 y, legacy_s16 color,
	legacy_s16 shadow_color)
{
	word_42248.left = LEGACY_S16_FROM_BITS((legacy_u16)x);
	word_42248.right = LEGACY_S16_WRAP_ADD(
		LEGACY_S16_WRAP_ADD(x, font_op2(text)), 1);
	word_42248.top = LEGACY_S16_FROM_BITS((legacy_u16)y);
	word_42248.bottom = LEGACY_S16_WRAP_ADD(
		LEGACY_S16_WRAP_ADD(y, fontdef_unk_0E), 1);
	font_set_unk(shadow_color, 0);
	font_draw_text(text, LEGACY_S16_WRAP_ADD(x, 1),
		LEGACY_S16_WRAP_ADD(y, 1));
	font_set_unk(color, 0);
	font_draw_text(text, LEGACY_S16_FROM_BITS((legacy_u16)x),
		LEGACY_S16_FROM_BITS((legacy_u16)y));
	return &word_42248;
}

static void intro_draw_resource_line(
	legacy_s8 far* resource,
	legacy_s8* resource_id,
	legacy_s16 is_text,
	legacy_s16 x,
	legacy_s16 y,
	legacy_s16 color,
	legacy_s16 shadow_color
) {
	legacy_s8 far* text;

	if (is_text != 0)
		text = locate_text_res(resource, resource_id);
	else
		text = locate_shape_alt(resource, resource_id);
	copy_string(&resID_byte1, text);
	intro_draw_text(&resID_byte1, x, y, color, shadow_color);
}

legacy_s8 load_intro_resources(void)
{
	legacy_s8 far* credit_resource;
	struct SHAPE2D far* credit_shapes[11];
	struct SHAPE2D far* arrow_shape;
	legacy_s16 target_x;
	legacy_s16 arrow_x;
	legacy_s16 arrow_y;
	legacy_s16 arrow_width;
	legacy_s16 arrow_height;
	legacy_s16 frame_elapsed;
	legacy_s16 animation_elapsed;
	legacy_s16 animation_target;
	legacy_s16 input;
	legacy_u16 animation_index;

	credit_resource = (legacy_s8 far*)file_load_resfile(aCred);
	locate_many_resources((legacy_s8 far*)tempdataptr,
		aArowarrwarw1ar, (legacy_s8 far**)credit_shapes);
	waitflag = 0x96;
	sprite_copy_wnd_to_1_clear();
	arrow_shape = credit_shapes[1];
	target_x = (legacy_s16)arrow_shape->s2d_pos_x;
	arrow_y = (legacy_s16)arrow_shape->s2d_pos_y;
	arrow_width = LEGACY_S16_WRAP_MUL(
		(legacy_s16)arrow_shape->s2d_width, video_flag1_is1);
	arrow_height = (legacy_s16)arrow_shape->s2d_height;

	intro_draw_resource_line(credit_resource, aCre, 1,
		0x78, 0, word_407D8, word_407DA);
	intro_draw_resource_line(credit_resource, aGds0, 0,
		0x3C, 0x0C, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aGds1, 0,
		0x68, 0x14, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aDes, 1,
		0x14, 0x20, word_407DC, word_407DE);
	intro_draw_resource_line(credit_resource, aGdon, 0,
		0x14, 0x2C, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aGkev, 0,
		0x14, 0x34, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aGbra, 0,
		0x14, 0x3C, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aGrob, 0,
		0x14, 0x44, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aGsta, 0,
		0x14, 0x4C, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aMus, 1,
		0x14, 0x5C, word_407E8, word_407EA);
	intro_draw_resource_line(credit_resource, aGmsy, 0,
		0x14, 0x68, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aGkri, 0,
		0x14, 0x70, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aGbri, 0,
		0x14, 0x78, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aPro, 1,
		0xAC, 0x20, word_407E0, word_407E2);
	intro_draw_resource_line(credit_resource, aGkev_0, 0,
		0xAC, 0x2C, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aOpr, 1,
		0xAC, 0x38, word_407E0, word_407E2);
	intro_draw_resource_line(credit_resource, aGbra_0, 0,
		0xAC, 0x40, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aGric, 0,
		0xAC, 0x48, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aArt, 1,
		0xAC, 0x54, word_407E4, word_407E6);
	intro_draw_resource_line(credit_resource, aGmsm, 0,
		0xAC, 0x60, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aGdav, 0,
		0xAC, 0x68, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aGnic, 0,
		0xAC, 0x70, word_407D4, word_407D6);
	intro_draw_resource_line(credit_resource, aGkev_1, 0,
		0xAC, 0x78, word_407D4, word_407D6);
	unload_resource(credit_resource);

	(void)sprite_blit_to_video(wndsprite, -1);
	sprite_copy_2_to_1_2();
	(void)timer_get_delta_alt();
	arrow_x = 0x14A;
	input = 0;
	for (;;) {
		frame_elapsed = (legacy_s16)timer_get_delta_alt();
		arrow_x = LEGACY_S16_WRAP_SUB(arrow_x,
			LEGACY_S16_WRAP_MUL(frame_elapsed, 2));
		if (target_x > arrow_x)
			break;
		mouse_draw_opaque_check();
		sprite_putimage_and_alt(arrow_shape, arrow_x, arrow_y);
		sprite_1_unk2(LEGACY_S16_WRAP_ADD(arrow_width, arrow_x),
			arrow_y, 0x20, arrow_height, 0);
		mouse_draw_transparent_check();
		input = (legacy_s16)input_do_checking(frame_elapsed);
		if (input != 0)
			break;
	}

	arrow_y = (legacy_s16)credit_shapes[0]->s2d_pos_y;
	animation_target = 0;
	animation_elapsed = 0;
	for (animation_index = 2;
		animation_index < 10U && input == 0;
		animation_index++) {
		sprite_copy_wnd_to_1();
		sprite_set_1_size(0, 0x140, arrow_y, 0xC8);
		sprite_clear_1_color(0);
		sprite_shape_to_1_alt(credit_shapes[animation_index]);
		sprite_copy_2_to_1_2();
		sprite_set_1_size(0, 0x140, arrow_y, 0xC8);
		mouse_draw_opaque_check();
		sprite_putimage(wndsprite->sprite_bitmapptr);
		mouse_draw_transparent_check();
		animation_target = LEGACY_S16_WRAP_ADD(animation_target, 5);
		while (animation_target > animation_elapsed) {
			frame_elapsed = (legacy_s16)timer_get_delta_alt();
			input = (legacy_s16)input_do_checking(frame_elapsed);
			animation_elapsed = LEGACY_S16_WRAP_ADD(
				animation_elapsed, frame_elapsed);
		}
	}

	sprite_set_1_size(0, 0x140, 0, 0xC8);
	mouse_draw_opaque_check();
	sprite_clear_shape(wndsprite->sprite_bitmapptr);
	sprite_copy_wnd_to_1();
	sprite_set_1_size(0, 0x140, arrow_y, 0xC8);
	sprite_clear_1_color(0);
	sprite_shape_to_1_alt(credit_shapes[0]);
	sprite_shape_to_1_alt(credit_shapes[10]);
	if (sprite_blit_to_video(wndsprite, 0) != 0)
		return 1;
	return input_repeat_check(0x1F4) != 0;
}

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
	legacy_u8 record[0x34];
	legacy_u8 far* scores;
	void far* read_result;
	legacy_u16 entry;
	legacy_u16 offset;

	byte_449CE = 0xFFU;
	for (entry = 0; entry < 7U; entry++)
		word_46170[entry] = entry;
	file_build_path(byte_3B80C, gameconfig.game_trackname,
		".hig", g_path_buf);
	if (create_default == 0) {
		g_is_busy = 1;
		read_result = sub_29A86(10, g_path_buf, td11_highscores);
		g_is_busy = 0;
		return read_result == 0 ? 1 : 0;
	}

	for (offset = 0; offset < 40U; offset++)
		record[offset] = '.';
	record[40] = 0;
	record[41] = 0;
	record[42] = '.';
	record[43] = '.';
	record[44] = '/';
	for (offset = 45U; offset < 49U; offset++)
		record[offset] = '.';
	record[49] = 0;
	LEGACY_WRITE_U16_LE(record + 50U, 0xFFFFU);
	scores = (legacy_u8 far*)td11_highscores;
	for (entry = 0; entry < 7U; entry++) {
		for (offset = 0; offset < sizeof(record); offset++)
			scores[entry * sizeof(record) + offset] = record[offset];
	}
	return file_write_fatal(g_path_buf, td11_highscores, 0x16CUL) != 0;
}

void highscore_write_b(void)
{
	legacy_u8 ordered_scores[0x16C];
	legacy_u8 far* scores;
	legacy_u16 entry;
	legacy_u16 offset;
	legacy_u16 source_entry;

	scores = (legacy_u8 far*)td11_highscores;
	for (entry = 0; entry < 7U; entry++) {
		source_entry = (legacy_u16)word_46170[entry];
		for (offset = 0; offset < 0x34U; offset++) {
			ordered_scores[entry * 0x34U + offset] =
				scores[source_entry * 0x34U + offset];
		}
	}
	file_build_path(byte_3B80C, gameconfig.game_trackname,
		".hig", g_path_buf);
	g_is_busy = 1;
	(void)file_write_fatal(g_path_buf, ordered_scores, 0x16CUL);
	g_is_busy = 0;
}

void print_highscore_entry(legacy_s16 entry, legacy_u8* text_offsets)
{
	legacy_u8 record[0x34];
	legacy_u8 far* scores;
	legacy_u16 record_offset;
	legacy_u16 copied;
	legacy_u16 output_offset;
	legacy_s16 saved_frame_rate;
	legacy_s16 frame_count;
	legacy_s8 formatted_time[18];
	legacy_s8* output;

	record_offset = LEGACY_U16_WRAP_MUL(
		(legacy_u16)word_46170[entry], 0x34U);
	scores = (legacy_u8 far*)td11_highscores;
	for (copied = 0; copied < sizeof(record); copied++)
		record[copied] = scores[record_offset + copied];

	text_offsets[0] = 0;
	strcpy(&resID_byte1, (legacy_s8*)record);
	output_offset = (legacy_u16)strlen(&resID_byte1) + 1U;
	text_offsets[1] = (legacy_u8)output_offset;
	strcpy(&resID_byte1 + output_offset, (legacy_s8*)record + 17);
	output_offset = LEGACY_U16_WRAP_ADD(output_offset,
		(legacy_u16)strlen(&resID_byte1 + output_offset) + 1U);
	text_offsets[2] = (legacy_u8)output_offset;

	output = &resID_byte1 + output_offset;
	*output = 0;
	if (record[41] == 1)
		strcat(output, "(");
	strcat(output, (legacy_s8*)record + 42);
	if (record[41] == 1)
		strcat(output, ")");
	output_offset = LEGACY_U16_WRAP_ADD(output_offset,
		(legacy_u16)strlen(output) + 1U);

	saved_frame_rate = framespersec;
	framespersec = 0x14;
	frame_count = LEGACY_S16_FROM_BITS(
		LEGACY_READ_U16_LE(record + 50));
	format_frame_as_string(formatted_time,
		frame_count == -1 ? 0 : frame_count, 1);
	text_offsets[3] = (legacy_u8)output_offset;
	strcpy(&resID_byte1 + output_offset, formatted_time);
	framespersec = saved_frame_rate;
}

extern void font_set_fontdef2(void far* data);

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
		color = entry == (legacy_u8)byte_449CE ? dialogarg2 : 0;
		font_set_unk(color, 0);
		font_draw_text(&resID_byte1 + text_offsets[0], 0x10, row);
		font_draw_text(&resID_byte1 + text_offsets[1], 0x78, row);
		font_draw_text(&resID_byte1 + text_offsets[2], 0xE0, row);
		font_draw_text(&resID_byte1 + text_offsets[3], 0x110, row);
	}
	font_set_fontdef();
}

void run_tracks_menu(legacy_s16 reload_track)
{
	legacy_s8 far* text_resource;
	legacy_s8 far* prompt;
	legacy_u8 far* scores;
	legacy_u8 text_offsets[4];
	legacy_u8 selected;
	legacy_u8 previous;
	legacy_u8 blit_mode;
	legacy_u16 elapsed;
	legacy_u16 key;
	legacy_u16 score_offset;
	legacy_u16 score;
	legacy_s16 hit;
	legacy_s8 chosen;
	legacy_s16 needs_track_setup;

	ensure_file_exists(3);
	needs_track_setup = reload_track != 0;
	for (;;) {
		if (needs_track_setup != 0) {
			check_input();
			show_waiting();
			waitflag = 0x82;
			track_setup();
			load_tracks_menu_shapes();
			needs_track_setup = 0;
		}

		selected = 0;
		previous = 0xFFU;
		blit_mode = 0xFFU;
		show_waiting();
		waitflag = 0x9B;
		wndsprite = sprite_make_wnd(0x140U, 0xC8U, 0x0FU);
		load_skybox((legacy_s8)td14_elem_map_main[0x384]);
		shape3d_load_all();
		set_projection(0x28, 0x28, 0x140, 0xC8);
		init_game_state(-2);
		sprite_copy_wnd_to_1();
		sprite_clear_1_color((legacy_u8)skybox_grd_color);
		sprite_set_1_size(0, 0x140, 0, 0xC8);
		draw_track_preview();
		shape3d_free_all();
		unload_skybox();

		sprite_copy_wnd_to_1();
		strcpy(&resID_byte1, "'");
		strcat(&resID_byte1, gameconfig.game_trackname);
		strcat(&resID_byte1, "'");
		intro_draw_text(&resID_byte1, font_op2_alt(&resID_byte1), 6,
			dialog_fnt_colour, 0);
		if (highscore_write_a(0) == 0) {
			score_offset = LEGACY_U16_WRAP_ADD(
				LEGACY_U16_WRAP_MUL(word_46170[0], 0x34U), 0x32U);
			scores = (legacy_u8 far*)td11_highscores;
			score = (legacy_u16)scores[score_offset] |
				((legacy_u16)scores[
					LEGACY_U16_WRAP_ADD(score_offset, 1U)] << 8);
			if (score != 0xFFFFU) {
				copy_string(&resID_byte1,
					locate_text_res(mainresptr, "hs0"));
				intro_draw_text(&resID_byte1,
					font_op2_alt(&resID_byte1), 0x12,
					dialog_fnt_colour, 0);
				font_set_fontdef2(fontnptr);
				print_highscore_entry(0, text_offsets);
				font_set_unk(0, 0);
				font_draw_text(&resID_byte1 + text_offsets[0],
					0x10, 0x1E);
				font_draw_text(&resID_byte1 + text_offsets[1],
					0x78, 0x1E);
				font_draw_text(&resID_byte1 + text_offsets[2],
					0xE0, 0x1E);
				font_draw_text(&resID_byte1 + text_offsets[3],
					0x110, 0x1E);
				font_set_fontdef();
			}
		}

		text_resource = (legacy_s8 far*)file_load_resfile("tedit");
		draw_button(locate_text_res(text_resource, "bmt"),
			0x11, 0xAC, 0x5E, 0x18, word_407F4, word_407F6,
			word_407F8, 0);
		draw_button(locate_text_res(text_resource, "bet"),
			0x71, 0xAC, 0x5E, 0x18, word_407F4, word_407F6,
			word_407F8, 0);
		draw_button(locate_text_res(text_resource, "bmm"),
			0xD1, 0xAC, 0x5E, 0x18, word_407F4, word_407F6,
			word_407F8, 0);
		unload_resource(text_resource);

		for (;;) {
			if (selected != previous) {
				previous = selected;
				sprite_blit_to_video(wndsprite,
					LEGACY_S8_FROM_BITS(blit_mode));
				blit_mode = 0xFEU;
				sprite_copy_2_to_1_2();
				sub_29772();
			}

			elapsed = (legacy_u16)mouse_timer_sprite_unk(selected,
				trackmenu_buttons_x1, trackmenu_buttons_x2,
				trackmenu_buttons_y1, trackmenu_buttons_y2,
				word_407CE, word_407D0);
			idle_counter = LEGACY_U16_WRAP_ADD(idle_counter, elapsed);
			if (LEGACY_S16_FROM_BITS((legacy_u16)idle_counter) >
				0x1770) {
				idle_counter = 0;
				idle_expired = (legacy_u8)(idle_expired + 1U);
			}
			key = (legacy_u16)input_checking(
				LEGACY_S16_FROM_BITS(elapsed));
			hit = (legacy_s16)mouse_multi_hittest(3,
				trackmenu_buttons_x1, trackmenu_buttons_x2,
				trackmenu_buttons_y1, trackmenu_buttons_y2);
			if (hit != -1)
				selected = (legacy_u8)hit;
			if (idle_expired != 0) {
				selected = 2;
				key = 0x0DU;
			}

			if (key == 0)
				continue;
			if (key == 0x4B00U) {
				selected = selected == 0 ? 2U :
					(legacy_u8)(selected - 1U);
				continue;
			}
			if (key == 0x4D00U) {
				selected = selected >= 2U ? 0U :
					(legacy_u8)(selected + 1U);
				continue;
			}
			if (key == 0x1BU)
				selected = 0xFFU;
			else if (key != 0x0DU && key != 0x20U)
				continue;

			if (selected == 0) {
				prompt = locate_text_res(mainresptr, "trk");
				chosen = do_fileselect_dialog(byte_3B80C,
					gameconfig.game_trackname, ".trk", prompt);
				file_build_path(byte_3B80C,
					gameconfig.game_trackname, ".trk", g_path_buf);
				if (chosen != 0) {
					file_read_fatal(g_path_buf, td14_elem_map_main);
					sprite_free_wnd(wndsprite);
					break;
				}
				previous = 0xFFU;
				continue;
			}

			sprite_free_wnd(wndsprite);
			if (selected == 1)
				needs_track_setup = 1;
			else
				return;
			break;
		}
	}
}

void run_opponent_menu(void)
{
	static legacy_s8* button_resource_ids[5] = {
		aBla, aBnx, aBcl, aBca, aBdo
	};
	legacy_s8 far* opponent_resource;
	legacy_s8 far* description;
	struct SHAPE2D far* shape;
	legacy_u8 selected;
	legacy_u8 previous_selection;
	legacy_u8 displayed_opponent;
	legacy_u8 blit_mode;
	legacy_u8 resource_loaded;
	legacy_u8 character;
	legacy_u16 line_length;
	legacy_s16 line_y;
	legacy_u16 elapsed;
	legacy_u16 key;
	legacy_s16 hit;
	legacy_u16 index;

	ensure_file_exists(4);
	miscptr = file_load_resfile(aMisc);
	opp_res = (legacy_s8 far*)file_load_resource(8, aSdosel);
	locate_many_resources(opp_res, aOpp0opp1opp2op, oppresources);
	selected = 0;
	resource_loaded = 0;
	displayed_opponent = 0xFFU;
	blit_mode = 0xFFU;
	sub_29772();

	for (;;) {
		mouse_draw_transparent_check();

opponent_menu_refresh:
		if (displayed_opponent !=
			(legacy_u8)gameconfig.game_opponenttype) {
			if (displayed_opponent != 0xFFU) {
				sprite_free_wnd(wndsprite);
				if (resource_loaded != 0)
					unload_resource(opponent_resource);
			}

			ensure_file_exists(4);
			if ((legacy_u8)gameconfig.game_opponenttype != 0) {
				aOpp1[3] = (legacy_s8)(
					(legacy_u8)gameconfig.game_opponenttype + '0');
				opponent_resource = (legacy_s8 far*)file_load_resfile(aOpp1);
				resource_loaded = 1;
			} else {
				resource_loaded = 0;
			}

			wndsprite = sprite_make_wnd(0x140U, 0xC8U, 0x0FU);
			displayed_opponent =
				(legacy_u8)gameconfig.game_opponenttype;
			previous_selection = 0xFFU;
			if (video_flag5_is0 == 0)
				sprite_copy_wnd_to_1();
			else
				setup_mcgawnd2();
			sprite_clear_1_color(0);

			shape = (struct SHAPE2D far*)locate_shape_fatal(
				opp_res, aScrn_0);
			sub_34526(shape);
			for (index = 0; index < 5U; index++) {
				draw_button(locate_text_res((legacy_s8 far*)miscptr,
					button_resource_ids[index]),
					LEGACY_S16_WRAP_ADD(0x15,
						LEGACY_U16_WRAP_MUL(index, 0x38U)),
					opponentmenu_buttons_y1[0] + 1,
					0x36, 0x12, word_407F4, word_407F6,
					word_407F8, 0);
			}

			sub_34526((struct SHAPE2D far*)
				oppresources[(legacy_u8)gameconfig.game_opponenttype]);
			shape = (struct SHAPE2D far*)locate_shape_fatal(
				opp_res, aClip);
			sub_34526(shape);
			if (video_flag5_is0 != 0) {
				sprite_clear_shape_alt(
					wndsprite->sprite_bitmapptr, 0, 0);
				sprite_copy_wnd_to_1();
			}

			if ((legacy_u8)gameconfig.game_opponenttype != 0)
				description = locate_text_res(
					opponent_resource, aDes_0);
			else
				description = locate_text_res(
					(legacy_s8 far*)miscptr, aRac);
			font_set_fontdef2(fontnptr);
			font_set_unk(0, dialog_fnt_colour);
			line_length = 0;
			line_y = 0;
			for (;;) {
				character = (legacy_u8)*description++;
				if (character == ']') {
					if (line_length != 0) {
						*(&resID_byte1 + line_length) = 0;
						font_draw_text(&resID_byte1, 0x0C,
							LEGACY_S16_WRAP_ADD(line_y, 0x21));
					}
					line_length = 0;
					line_y = LEGACY_S16_WRAP_ADD(
						line_y, fontdef_unk_0E);
				} else {
					*(&resID_byte1 + line_length++) = (legacy_s8)character;
				}
				if (*description == 0)
					break;
			}
			font_set_fontdef();
		}

		if (selected != previous_selection) {
			previous_selection = selected;
			sprite_blit_to_video(wndsprite,
				LEGACY_S8_FROM_BITS(blit_mode));
			blit_mode = 0xFEU;
			(void)timer_get_delta_alt();
			sub_29772();
		}

		elapsed = (legacy_u16)mouse_timer_sprite_unk(selected,
			opponentmenu_buttons_x1, opponentmenu_buttons_x2,
			opponentmenu_buttons_y1, opponentmenu_buttons_y2,
			word_407CE, word_407D0);
		key = (legacy_u16)input_checking(
			LEGACY_S16_FROM_BITS(elapsed));
		hit = (legacy_s16)mouse_multi_hittest(5,
			opponentmenu_buttons_x1, opponentmenu_buttons_x2,
			opponentmenu_buttons_y1, opponentmenu_buttons_y2);
		if (hit != -1 &&
			!((legacy_u8)gameconfig.game_opponenttype == 0 && hit == 3))
			selected = (legacy_u8)hit;

		if (key == 0)
			goto opponent_menu_refresh;
		if (key == 0x4B00U) {
			selected = selected == 0 ? 4U :
				(legacy_u8)(selected - 1U);
			if ((legacy_u8)gameconfig.game_opponenttype == 0 &&
				selected == 3)
				selected--;
			goto opponent_menu_refresh;
		}
		if (key == 0x4D00U) {
			selected = selected < 4U ?
				(legacy_u8)(selected + 1U) : 0U;
			if ((legacy_u8)gameconfig.game_opponenttype == 0 &&
				selected == 3)
				selected++;
			goto opponent_menu_refresh;
		}
		if (key != 0x0DU && key != 0x1BU && key != 0x20U)
			goto opponent_menu_refresh;

		if (selected == 0) {
			gameconfig.game_opponenttype = (legacy_s8)(
				(legacy_u8)gameconfig.game_opponenttype - 1U);
			if (LEGACY_S8_FROM_BITS(
				(legacy_u8)gameconfig.game_opponenttype) < 1)
				gameconfig.game_opponenttype = 6;
			goto opponent_menu_refresh;
		}
		if (selected == 1) {
			gameconfig.game_opponenttype = (legacy_s8)(
				(legacy_u8)gameconfig.game_opponenttype + 1U);
			if ((legacy_u8)gameconfig.game_opponenttype == 7)
				gameconfig.game_opponenttype = 1;
			goto opponent_menu_refresh;
		}
		if (selected == 2) {
			gameconfig.game_opponenttype = 0;
			goto opponent_menu_refresh;
		}
		if (selected == 3) {
			if ((legacy_u8)gameconfig.game_opponenttype == 0)
				goto opponent_menu_refresh;
			check_input();
			mouse_draw_opaque_check();
			sprite_free_wnd(wndsprite);
			unload_resource(opponent_resource);
			show_waiting();
			run_car_menu(&gameconfig.game_opponentcarid[0],
				&gameconfig.game_opponentmaterial,
				&gameconfig.game_opponenttransmission,
				(legacy_u8)gameconfig.game_opponenttype);
			displayed_opponent = 0xFFU;
			continue;
		}

		if ((legacy_u8)gameconfig.game_opponenttype != 0) {
			if ((legacy_u8)gameconfig.game_opponentcarid[0] == 0xFFU) {
				for (index = 0; index < 4U; index++)
					gameconfig.game_opponentcarid[index] =
						gameconfig.game_playercarid[index];
				gameconfig.game_opponentmaterial = (legacy_s8)(
					(((legacy_u8)gameconfig.game_playermaterial & 1U) ^ 1U));
				gameconfig.game_opponenttransmission = 0;
			}
		} else {
			gameconfig.game_opponentcarid[0] = (legacy_s8)0xFFU;
		}

		sprite_free_wnd(wndsprite);
		if (resource_loaded != 0)
			unload_resource(opponent_resource);
		mmgr_free(opp_res);
		unload_resource(miscptr);
		mouse_draw_opaque_check();
		return;
	}
}

extern legacy_s8 gnam_string[];
extern legacy_s8 gsna_string[];
extern legacy_s8 unk_46464[];
extern legacy_s8 byte_459E0[];

static legacy_u16 read_highscore_u16(legacy_u8 far* address)
{
	return (legacy_u16)((legacy_u16)address[0] |
		((legacy_u16)address[1] << 8));
}

void enter_hiscore(legacy_s16 frame_count, void far* prompt, legacy_u8 car_flag)
{
	legacy_u8 record[0x34];
	legacy_u8 far* scores;
	legacy_u16 entry;
	legacy_u16 copied;
	legacy_u16 rank;
	legacy_u16 time_bits;
	legacy_s16 positions[2];

	time_bits = (legacy_u16)frame_count;
	if (framespersec == 0x0A)
		time_bits = LEGACY_U16_WRAP_MUL(time_bits, 2U);
	scores = (legacy_u8 far*)td11_highscores;
	if (read_highscore_u16(scores + 0x16AU) <= time_bits) {
		highscore_text_unk();
		return;
	}

	entry = 0;
	while (read_highscore_u16(scores + entry * 0x34U + 0x32U) <=
		time_bits) {
		if (entry >= 7U)
			break;
		word_46170[entry] = (legacy_s16)entry;
		entry++;
	}
	rank = entry;
	byte_449CE = (legacy_u8)rank;
	while (entry < 6U) {
		word_46170[entry + 1U] = (legacy_s16)entry;
		entry++;
	}
	word_46170[rank] = 6;

	for (copied = 0; copied < sizeof(record); copied++)
		record[copied] = 0;
	strcpy((legacy_s8*)record + 17, gnam_string);
	record[41] = car_flag;
	if (gameconfig.game_opponenttype != 0) {
		strcpy((legacy_s8*)record + 42, unk_46464);
		record[44] = '/';
		strcpy((legacy_s8*)record + 45, gsna_string);
	} else {
		strcpy((legacy_s8*)record + 42, " ");
	}
	LEGACY_WRITE_U16_LE(record + 50, time_bits);
	for (copied = 0; copied < sizeof(record); copied++)
		scores[0x138U + copied] = record[copied];

	sprite_copy_wnd_to_1();
	highscore_text_unk();
	sprite_blit_to_video(wndsprite, -1);
	show_dialog(3, 0, prompt, 0xFFFFU, 0xFFFFU,
		dialogarg2, positions, 0);
	check_input();
	call_read_line(byte_459E0, 0x10, positions[0], positions[1],
		0x7530UL);
	strcpy((legacy_s8*)record, byte_459E0);
	for (copied = 0; copied < sizeof(record); copied++)
		scores[0x138U + copied] = record[copied];

	sprite_copy_wnd_to_1();
	highscore_text_unk();
	sprite_blit_to_video(wndsprite, -1);
	highscore_write_b();
	highscore_text_unk();
}

void security_check(legacy_s16 question_index)
{
	legacy_s8 question_id[4] = "q00";
	legacy_s8 answer_id[4] = "a00";
	legacy_s8 question_text[1024];
	legacy_s8 answer[22];
	legacy_u8 question_parts[6];
	legacy_s16 positions[8];
	void far* resource;
	legacy_u16 answer_length;
	legacy_u16 attempts;
	legacy_u16 i;

	question_id[2] = byte_3BD34[(legacy_u16)question_index];
	answer_id[2] = question_id[2];
	resource = file_load_resfile("misc");
	copy_string(question_text, locate_text_res(resource, "cop"));
	copy_string(&resID_byte1, locate_text_res(resource, question_id));
	strcat(question_text, unk_463EA);
	for (i = 0; i < 6U; i++)
		question_parts[i] = (legacy_u8)(&resID_byte1)[i];

	show_dialog(3, 1, (void far*)question_text, 0xFFFFU, 0x78U,
		performGraphColor, positions, 0);
	(&resID_byte1)[2] = 0;
	(&resID_byte1)[0] = question_parts[0];
	(&resID_byte1)[1] = question_parts[1];
	font_draw_text(&resID_byte1, positions[0], positions[1]);
	(&resID_byte1)[0] = question_parts[2];
	(&resID_byte1)[1] = question_parts[3];
	font_draw_text(&resID_byte1, positions[2], positions[3]);
	(&resID_byte1)[0] = question_parts[4];
	(&resID_byte1)[1] = question_parts[5];
	font_draw_text(&resID_byte1, positions[4], positions[5]);

	copy_string(&resID_byte1, locate_text_res(resource, answer_id));
	answer_length = (legacy_u16)strlen(&resID_byte1);
	answer[0] = 0;
	attempts = 0;
	for (;;) {
		call_read_line(answer, answer_length, positions[6], positions[7],
			0x7530UL);
		for (i = 0; answer[i] != 0; i++) {
			legacy_u8 character = (legacy_u8)answer[i];

			if ((g_ascii_props[character] & RST_ASC_CHAR_UPPER) != 0)
				answer[i] = (legacy_s8)(character + 0x20U);
		}
		if (strcmp(answer, &resID_byte1) == 0) {
			passed_security = 1;
			break;
		}
		attempts++;
		if (passed_security != 0 || attempts == 3U)
			break;
	}

	sub_275C6();
	mouse_draw_transparent_check();
	unload_resource(resource);
}

void replay_unk(void)
{
	legacy_s16 steering_angle;
	legacy_s16 target_angle;
	legacy_s16 response;
	legacy_s16 adjusted_angle;
	legacy_u16 frame;
	legacy_u16 history_index;
	legacy_u16 speed_index;
	legacy_u8 action;
	legacy_s8* response_table;

	frame = state.game_frame;
	history_index = frame & 0x3FU;
	if (byte_442EA[history_index] == 0)
		return;

	target_angle = LEGACY_S8_FROM_BITS(byte_44292[history_index]);
	steering_angle = state.playerstate.car_steeringAngle;
	speed_index = (state.playerstate.car_speed2 >> 10) & 0xFCU;
	response_table = (legacy_s8*)steerWhlRespTable_ptr;
	response = response_table[speed_index + 1U];
	if ((steering_angle < target_angle && steering_angle < -1) ||
		(steering_angle > target_angle && steering_angle > 1)) {
		response = LEGACY_S8_FROM_BITS(
			(legacy_u8)((legacy_u8)response << 2));
	}

	action = 0;
	if (steering_angle > target_angle) {
		adjusted_angle = LEGACY_S16_WRAP_SUB(steering_angle, response);
		if (adjusted_angle >= target_angle)
			action = 8;
	} else if (steering_angle < target_angle) {
		adjusted_angle = LEGACY_S16_WRAP_ADD(steering_angle, response);
		if (adjusted_angle <= target_angle)
			action = 4;
	}
	if (action != 0)
		td16_rpl_buffer[frame] |= action;
	byte_442EA[history_index] = 0;
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

static legacy_s16 font_measure(const legacy_s8* text, legacy_u16 remaining, legacy_s16 bounded)
{
	legacy_u8 far* font_definition;
	legacy_u16 glyph_offset;
	legacy_u16 glyph_width;
	legacy_u16 total_width;
	legacy_u8 character;
	legacy_u8 has_glyph_widths;

	if (bounded != 0 && remaining == 0)
		return 0;
	font_definition = word_405FE;
	has_glyph_widths = font_definition[0x14U];
	glyph_width = audioresource_get_word(font_definition + 0x10U);
	total_width = 0;
	while ((character = (legacy_u8)*text++) != 0) {
		glyph_offset = audioresource_get_word(font_definition + 0x16U +
			(legacy_u16)character * 2U);
		if (glyph_offset == 0)
			continue;
		if (has_glyph_widths != 0)
			glyph_width = font_definition[glyph_offset];
		total_width = LEGACY_U16_WRAP_ADD(total_width, glyph_width);
		remaining--;
		if (remaining == 0)
			break;
	}
	return LEGACY_S16_FROM_BITS(total_width);
}

legacy_s16 font_op(const legacy_s8* text, legacy_s16 glyph_count)
{
	return font_measure(text, (legacy_u16)glyph_count, 1);
}

legacy_s16 font_op2(const legacy_s8* text)
{
	return font_measure(text, 0, 0);
}

static legacy_u32 secondary_timer_target(void)
{
	return ((legacy_u32)word_3F1C4 << 16) | word_3F1C2;
}

static legacy_s16 secondary_timer_target_reached(
	legacy_u32 current,
	legacy_u32 target
) {
	return (legacy_u16)(current >> 16) >= (legacy_u16)(target >> 16) &&
		(legacy_u16)current >= (legacy_u16)target;
}

legacy_u32 set_add_value(legacy_u32 ticks)
{
	legacy_u32 target;

	target = (legacy_u32)(timer_get_slow_counter() + ticks);
	word_3F1C2 = (legacy_u16)target;
	word_3F1C4 = (legacy_u16)(target >> 16);
	return target;
}

legacy_s16 sub_2EB07(void)
{
	return secondary_timer_target_reached(
		timer_get_slow_counter(), secondary_timer_target());
}

legacy_u32 sub_2EB1E(legacy_u32 ticks)
{
	legacy_u32 current;
	legacy_u32 target;

	target = (legacy_u32)(timer_get_slow_counter() + ticks);
	do {
		current = timer_get_slow_counter();
	} while (!secondary_timer_target_reached(current, target));
	return current;
}

void add_exit_handler(void (far* exit_handler)(void))
{
	legacy_s16 index;

	for (index = 0; index < 10; index++) {
		if (exitlistfuncs[index] == exit_handler)
			return;
		if (exitlistfuncs[index] == 0) {
			exitlistfuncs[index] = exit_handler;
			exitlistfuncs[index + 1] = 0;
			return;
		}
	}
	fatal_error(aExitListOverflow);
}

void call_exitlist(void)
{
	legacy_s16 index;

	for (index = 10; index >= 0; index--)
		if (exitlistfuncs[index] != 0)
			exitlistfuncs[index]();
}

void call_exitlist2(void)
{
	call_exitlist();
	dos_process_exit(0);
}

static void fatal_vprintf(const legacy_s8* format, legacy_u16 argument_offset)
{
	legacy_s16 buffer_state;

	buffer_state = __stbuf(stdout);
	__output(stdout, format, (void*)argument_offset);
	__ftbuf(buffer_state, stdout);
}

void fatal_error(const legacy_s8* format, ...)
{
	va_list arguments;

	sprite_copy_2_to_1();
	va_start(arguments, format);
	fatal_vprintf(format, FP_OFF(arguments));
	va_end(arguments);
	flush_stdin();
	call_exitlist();
	va_start(arguments, format);
	fatal_vprintf(format, FP_OFF(arguments));
	va_end(arguments);
	dos_process_exit(1);
}

extern legacy_s16 read_line(legacy_s16 flags, legacy_s8* text, legacy_s16 initial_key,
	legacy_s16 max_characters, legacy_s16 max_pixels, legacy_s16 x, legacy_s16 y,
	void (far* callback)(void), legacy_u32 timeout);
void read_line_helper(void);
void read_line_helper2(void);

legacy_s16 call_read_line(legacy_s8* text, legacy_s16 max_characters, legacy_s16 x, legacy_s16 y,
	legacy_u32 timeout)
{
	legacy_u16 length;
	legacy_u16 trim_index;
	legacy_u16 max_pixels;
	legacy_s16 result;

	mouse_draw_opaque_check();
	max_pixels = LEGACY_U16_WRAP_ADD(
		LEGACY_U16_WRAP_MUL(max_characters, 9U), 9U);
	result = read_line(2, text, 0, max_characters, max_pixels, x, y,
		&dos_kb_clear_numlock, timeout);
	mouse_draw_transparent_check();

	length = (legacy_u16)strlen(text);
	trim_index = LEGACY_U16_WRAP_SUB(length, 1U);
	while (text[trim_index] == ' ')
		trim_index = LEGACY_U16_WRAP_SUB(trim_index, 1U);
	text[LEGACY_U16_WRAP_ADD(trim_index, 1U)] = 0;
	return result;
}

legacy_s16 sprite_blit_to_video(struct SPRITE far* sprite, legacy_s16 mode)
{
	legacy_s16 result;
	legacy_u16 phase;

	sprite_copy_2_to_1_2();
	mouse_draw_opaque_check();
	if ((legacy_u16)mode == 0xFFFEU) {
		sprite_putimage(sprite->sprite_bitmapptr);
		mouse_draw_transparent_check();
		return 0;
	}

	result = 0;
	for (phase = 0; phase < 4U; ++phase) {
		result = input_do_checking((legacy_s16)timer_get_delta_alt());
		if (result != 0)
			break;
		sprite_1_unk3(sprite->sprite_bitmapptr, phase);
	}
	if (result != 0) {
		sprite_copy_2_to_1_2();
		sprite_putimage(sprite->sprite_bitmapptr);
	}
	mouse_draw_transparent_check();
	return result;
}

legacy_s16 read_line(legacy_s16 flags, legacy_s8* text, legacy_s16 initial_key, legacy_s16 max_characters,
	legacy_s16 max_pixels, legacy_s16 x, legacy_s16 y, void (far* callback)(void),
	legacy_u32 timeout)
{
	legacy_u8 input_flags;
	legacy_u16 key;
	legacy_u16 length;
	legacy_u16 index;
	legacy_u16 old_cursor_state;
	legacy_s16 insert_mode;
	legacy_s16 first_key;

	input_flags = (legacy_u8)flags;
	sprite_copy_2_to_1();
	word_42A18 = (legacy_u16)x;
	word_42A1A = (legacy_u16)y;
	off_42A1E = text;
	word_42A20 = (legacy_u16)max_pixels;
	text[(legacy_u16)max_characters] = 0;
	if ((input_flags & 1U) != 0)
		text[0] = 0;
	if ((input_flags & 2U) != 0)
		word_42A22 = 0;
	else
		word_42A22 = (legacy_u16)strlen(text);

	length = (legacy_u16)strlen(text);
	while (LEGACY_S16_FROM_BITS(length) <
		LEGACY_S16_FROM_BITS(max_characters)) {
		text[length] = ' ';
		length = LEGACY_U16_WRAP_ADD(length, 1U);
	}
	read_line_helper2();
	word_42A16 = 1;
	word_42A1C = 1;
	insert_mode = 0;
	read_line_helper();
	timer_copy_counter(timeout);
	set_add_value(4UL);
	first_key = 1;

	for (;;) {
		if ((legacy_u16)initial_key != 0) {
			key = (legacy_u16)initial_key;
			initial_key = 0;
		} else {
			do {
				callback();
				key = (legacy_u16)kb_call_readchar_callback();
				if (key != 0)
					break;
			} while (sub_2EB07() == 0);
		}

		if (key == 0) {
			set_add_value(4UL);
			old_cursor_state = (legacy_u16)word_42A1C;
			word_42A1C = 1;
			read_line_helper();
			word_42A1C = old_cursor_state != 0 ? 0 : 1;
			if (timeout != 0 && timer_compare_dx()) {
				read_line_helper();
				return 0;
			}
			continue;
		}

		timer_copy_counter(timeout);
		if (key == 0x0DU || key == 0x1BU || key == 0x4800U ||
			(key == 0x5000U && (input_flags & 8U) == 0) ||
			(key == 9U && (input_flags & 0x10U) == 0)) {
			read_line_helper();
			return key;
		}

		if (key == 0x4D00U) {
			read_line_helper();
			if (LEGACY_S16_FROM_BITS(max_characters) >
				LEGACY_S16_FROM_BITS(word_42A22))
				word_42A22 = LEGACY_U16_WRAP_ADD(word_42A22, 1U);
			read_line_helper();
			first_key = 0;
			continue;
		}

		if (key == 0x4B00U) {
			read_line_helper();
			if (word_42A22 != 0)
				word_42A22 = LEGACY_U16_WRAP_SUB(word_42A22, 1U);
			read_line_helper();
			first_key = 0;
			continue;
		}

		if (key == 0x4700U) {
			read_line_helper();
			word_42A22 = 0;
			read_line_helper();
			first_key = 0;
			continue;
		}

		if (key == 0x4F00U) {
			read_line_helper();
			word_42A22 = (legacy_u16)strlen(text);
			read_line_helper();
			first_key = 0;
			continue;
		}

		if (key == 0x5200U) {
			read_line_helper();
			insert_mode = !insert_mode;
			word_42A16 = insert_mode ? 8U : 1U;
			read_line_helper();
			first_key = 0;
			continue;
		}

		if (key == 0x5300U) {
			if (LEGACY_S16_FROM_BITS(max_characters) >
				LEGACY_S16_FROM_BITS(word_42A22) &&
				text[(legacy_u16)word_42A22] != 0) {
				read_line_helper();
				index = (legacy_u16)word_42A22;
				while (LEGACY_S16_FROM_BITS(index) <
					LEGACY_S16_FROM_BITS(max_characters)) {
					text[index] = text[LEGACY_U16_WRAP_ADD(index, 1U)];
					index = LEGACY_U16_WRAP_ADD(index, 1U);
				}
				text[LEGACY_U16_WRAP_SUB(max_characters, 1U)] = ' ';
				read_line_helper2();
				read_line_helper();
			}
			first_key = 0;
			continue;
		}

		if (key == 8U) {
			if (word_42A22 != 0) {
				read_line_helper();
				word_42A22 = LEGACY_U16_WRAP_SUB(word_42A22, 1U);
				index = (legacy_u16)word_42A22;
				while (LEGACY_S16_FROM_BITS(index) <
					LEGACY_S16_FROM_BITS(max_characters)) {
					text[index] = text[LEGACY_U16_WRAP_ADD(index, 1U)];
					index = LEGACY_U16_WRAP_ADD(index, 1U);
				}
				text[LEGACY_U16_WRAP_SUB(max_characters, 1U)] = ' ';
				read_line_helper2();
				read_line_helper();
			}
			first_key = 0;
			continue;
		}

		if (LEGACY_S16_FROM_BITS(key) >= 0x20 &&
			LEGACY_S16_FROM_BITS(key) <= 0x7A &&
			LEGACY_S16_FROM_BITS(max_characters) >
				LEGACY_S16_FROM_BITS(word_42A22)) {
			read_line_helper();
			if (first_key && (input_flags & 4U) == 0) {
				word_42A22 = 0;
				for (index = 0;
					LEGACY_S16_FROM_BITS(index) <
						LEGACY_S16_FROM_BITS(max_characters);
					index = LEGACY_U16_WRAP_ADD(index, 1U))
					text[index] = ' ';
			}

			index = (legacy_u16)word_42A22;
			if (text[index] == 0)
				text[LEGACY_U16_WRAP_ADD(index, 1U)] = 0;
			if (insert_mode) {
				legacy_u16 move_index;
				move_index = LEGACY_U16_WRAP_SUB(max_characters, 2U);
				while (LEGACY_S16_FROM_BITS(move_index) >=
					LEGACY_S16_FROM_BITS(word_42A22)) {
					text[LEGACY_U16_WRAP_ADD(move_index, 1U)] =
						text[move_index];
					move_index = LEGACY_U16_WRAP_SUB(move_index, 1U);
				}
			}
			text[index] = (legacy_s8)(legacy_u8)key;
			if (LEGACY_S16_FROM_BITS(max_characters) >
				LEGACY_S16_FROM_BITS(word_42A22))
				word_42A22 = LEGACY_U16_WRAP_ADD(word_42A22, 1U);
			read_line_helper2();
			read_line_helper();
		}
		first_key = 0;
	}
}

void read_line_helper(void)
{
	static const legacy_s8 space[] = " ";
	legacy_u8 far* font_definition;
	legacy_u16 length;
	legacy_u16 cursor;
	legacy_u16 cursor_width;
	legacy_u16 x;
	legacy_u16 y;
	legacy_u16 color;

	if (word_42A1C == 0)
		return;
	length = legacy_near_string_length(off_42A1E);
	cursor = (legacy_u16)word_42A22;
	if (LEGACY_S16_FROM_BITS(length) < LEGACY_S16_FROM_BITS(cursor)) {
		cursor = length;
		word_42A22 = cursor;
	}
	cursor_width = (legacy_u16)font_op(off_42A1E + cursor, 1);
	if (cursor_width == 0)
		cursor_width = (legacy_u16)font_op2(space);
	x = LEGACY_U16_WRAP_ADD(font_op(off_42A1E, cursor), word_42A18);
	font_definition = word_405FE;
	y = LEGACY_U16_WRAP_ADD(
		audioresource_get_word(font_definition + 0x12U), word_42A1A);
	y = LEGACY_U16_WRAP_SUB(y, word_42A16);
	color = audioresource_get_word(font_definition);
	sub_35B76(LEGACY_S16_FROM_BITS(x), LEGACY_S16_FROM_BITS(y),
		LEGACY_S16_FROM_BITS(cursor_width),
		LEGACY_S16_FROM_BITS(word_42A16),
		LEGACY_S16_FROM_BITS(color));
}

void read_line_helper2(void)
{
	legacy_u8 far* font_definition;
	legacy_u16 length;
	legacy_u16 text_width;
	legacy_u16 remaining_width;

	if (word_42A20 != 0) {
		while (LEGACY_S16_FROM_BITS(font_op2(off_42A1E)) >
			LEGACY_S16_FROM_BITS(word_42A20)) {
			length = legacy_near_string_length(off_42A1E);
			if (length == 0)
				break;
			off_42A1E[length - 1U] = 0;
		}
	}
	length = legacy_near_string_length(off_42A1E);
	if (LEGACY_S16_FROM_BITS(length) <
		LEGACY_S16_FROM_BITS(word_42A22))
		word_42A22 = length;
	sub_345BC(off_42A1E, LEGACY_S16_FROM_BITS(word_42A18),
		LEGACY_S16_FROM_BITS(word_42A1A));
	if (word_42A20 == 0)
		return;

	text_width = (legacy_u16)font_op2(off_42A1E);
	remaining_width = LEGACY_U16_WRAP_SUB(word_42A20, text_width);
	if (LEGACY_S16_FROM_BITS(remaining_width) <= 0)
		return;
	font_definition = word_405FE;
	sprite_1_unk2(LEGACY_S16_FROM_BITS(
			LEGACY_U16_WRAP_ADD(text_width, word_42A18)),
		LEGACY_S16_FROM_BITS(word_42A1A),
		LEGACY_S16_FROM_BITS(remaining_width),
		LEGACY_S16_FROM_BITS(
			audioresource_get_word(font_definition + 0x12U)),
		LEGACY_S16_FROM_BITS(
			audioresource_get_word(font_definition + 2U)));
}

legacy_s16 audioresource_get_chunk_index(legacy_s16 extra_name_stride, legacy_s16 chunk_count,
	const legacy_s8* requested_name, const legacy_u8 far* chunk_names)
{
	const legacy_s8 far* requested_name_far;
	const legacy_u8 far* candidate;
	legacy_u16 names_offset;
	legacy_u16 names_segment;
	legacy_s16 count;
	legacy_s16 index;

	count = LEGACY_S16_FROM_BITS(chunk_count);
	if (count <= 0)
		return -1;
	requested_name_far = (const legacy_s8 far*)MK_FP(
		FP_SEG(requested_name), FP_OFF(requested_name));
	names_offset = (legacy_u16)FP_OFF(chunk_names);
	names_segment = (legacy_u16)FP_SEG(chunk_names);
	for (index = 0; index < count;
		index = LEGACY_S16_WRAP_ADD(index, 1)) {
		candidate = (const legacy_u8 far*)MK_FP(
			names_segment, names_offset);
		if (audioresource_compare_chunknames(0,
			(const legacy_s8 far*)candidate, requested_name_far, 4))
			return index;
		names_offset = LEGACY_U16_WRAP_ADD(names_offset,
			LEGACY_U16_WRAP_ADD(4U, extra_name_stride));
	}
	return -1;
}

static void far* audio_far_pointer_add_normalized(void far* pointer,
	legacy_u16 increment)
{
	legacy_u16 old_offset;
	legacy_u16 new_offset;
	legacy_u16 segment;

	old_offset = (legacy_u16)FP_OFF(pointer);
	segment = (legacy_u16)FP_SEG(pointer);
	new_offset = LEGACY_U16_WRAP_ADD(old_offset, increment);
	if (new_offset < old_offset)
		segment = LEGACY_U16_WRAP_ADD(segment, 0x1000U);
	return MK_FP(segment, new_offset);
}

void far* audioresource_find(void far* resource, const legacy_s8* chunk_name)
{
	legacy_u8 far* bytes;
	legacy_u8 far* offset_entry;
	legacy_u16 resource_offset;
	legacy_u16 resource_segment;
	legacy_u16 chunk_count;
	legacy_u16 table_offset;
	legacy_u16 relative_offset;
	legacy_u16 result_offset;
	legacy_s16 chunk_index;

	bytes = (legacy_u8 far*)resource;
	resource_offset = (legacy_u16)FP_OFF(resource);
	resource_segment = (legacy_u16)FP_SEG(resource);
	chunk_count = audioresource_get_word(
		(const legacy_u8 far*)audio_far_pointer_add_normalized(bytes, 4U));
	chunk_index = audioresource_get_chunk_index(0, chunk_count, chunk_name,
		(const legacy_u8 far*)audio_far_pointer_add_normalized(bytes, 6U));
	if (chunk_index < 0)
		return 0;

	table_offset = LEGACY_U16_WRAP_ADD(resource_offset,
		LEGACY_U16_WRAP_MUL(chunk_count, 4U));
	table_offset = LEGACY_U16_WRAP_ADD(table_offset,
		LEGACY_U16_WRAP_MUL(chunk_index, 4U));
	table_offset = LEGACY_U16_WRAP_ADD(table_offset, 6U);
	offset_entry = (legacy_u8 far*)MK_FP(resource_segment, table_offset);
	relative_offset = (legacy_u16)audioresource_get_dword(offset_entry);
	result_offset = LEGACY_U16_WRAP_ADD(resource_offset,
		LEGACY_U16_WRAP_MUL(chunk_count, 8U));
	result_offset = LEGACY_U16_WRAP_ADD(result_offset, relative_offset);
	result_offset = LEGACY_U16_WRAP_ADD(result_offset, 6U);
	return MK_FP(resource_segment, result_offset);
}

void audio_map_song_instruments(void far* song, void far* instruments)
{
	legacy_u8 far* header;
	void far* instrument;
	legacy_s8 name[4];
	legacy_u16 pointer_offset;
	legacy_u16 pointer_segment;
	legacy_u16 count;
	legacy_u16 index;
	legacy_u16 name_offset;

	header = (legacy_u8 far*)audioresource_find(song, "hdr1");
	if (header == 0)
		return;

	count = header[6];
	for (index = 0; index < count; ++index) {
		name_offset = 7U + index * 4U;
		name[0] = header[name_offset];
		name[1] = header[name_offset + 1U];
		name[2] = header[name_offset + 2U];
		name[3] = header[name_offset + 3U];
		instrument = audioresource_find(instruments, name);
		pointer_offset = (legacy_u16)FP_OFF(instrument);
		pointer_segment = (legacy_u16)FP_SEG(instrument);
		header[name_offset] = (legacy_u8)pointer_offset;
		header[name_offset + 1U] = (legacy_u8)(pointer_offset >> 8);
		header[name_offset + 2U] = (legacy_u8)pointer_segment;
		header[name_offset + 3U] = (legacy_u8)(pointer_segment >> 8);
	}

	basdres = audioresource_find(instruments, "BASD");
	snarres = audioresource_find(instruments, "SNAR");
	tommres = audioresource_find(instruments, "TOMM");
	rideres = audioresource_find(instruments, "RIDE");
	crshres = audioresource_find(instruments, "CRSH");
	chhtres = audioresource_find(instruments, "CHHT");
	ohhtres = audioresource_find(instruments, "OHHT");
}

static void audio_write_far_pointer_to_resource(legacy_u8 far* destination,
	legacy_u16 offset, legacy_u16 segment)
{
	destination[0] = (legacy_u8)offset;
	destination[1] = (legacy_u8)(offset >> 8);
	destination[2] = (legacy_u8)segment;
	destination[3] = (legacy_u8)(segment >> 8);
}

static void audio_patch_song_reference(legacy_u8 far* destination,
	legacy_u16 name_table_offset, legacy_u16 offset_table_offset,
	legacy_u16 first_data_offset, legacy_u16 resource_segment,
	legacy_u16 chunk_count)
{
	const legacy_u8 far* names;
	const legacy_u8 far* offset_entry;
	legacy_s8 name[4];
	legacy_u16 relative_offset;
	legacy_s16 chunk_index;

	name[0] = destination[0];
	name[1] = destination[1];
	name[2] = destination[2];
	name[3] = destination[3];
	names = (const legacy_u8 far*)MK_FP(resource_segment,
		name_table_offset);
	chunk_index = audioresource_get_chunk_index(0, chunk_count, name,
		names);
	if (chunk_index < 0)
		return;

	offset_entry = (const legacy_u8 far*)MK_FP(resource_segment,
		LEGACY_U16_WRAP_ADD(offset_table_offset,
			LEGACY_U16_WRAP_MUL((legacy_u16)chunk_index, 4U)));
	relative_offset = (legacy_u16)audioresource_get_dword(offset_entry);
	audio_write_far_pointer_to_resource(destination,
		LEGACY_U16_WRAP_ADD(first_data_offset, relative_offset),
		resource_segment);
}

void audio_map_song_tracks(void far* song)
{
	legacy_u8 far* bytes;
	legacy_u8 far* cursor;
	legacy_u16 resource_offset;
	legacy_u16 resource_segment;
	legacy_u16 chunk_count;
	legacy_u16 name_table_offset;
	legacy_u16 offset_table_offset;
	legacy_u16 first_data_offset;
	legacy_u16 chunk_offset;
	legacy_u16 chunk_end_offset;
	legacy_u16 cursor_offset;
	legacy_u16 relative_offset;
	legacy_u16 header_index;
	legacy_u16 index;
	legacy_u16 reference_count;
	legacy_u16 event;

	bytes = (legacy_u8 far*)song;
	resource_offset = (legacy_u16)FP_OFF(song);
	resource_segment = (legacy_u16)FP_SEG(song);
	chunk_count = audioresource_get_word(
		(const legacy_u8 far*)MK_FP(resource_segment,
			LEGACY_U16_WRAP_ADD(resource_offset, 4U)));
	name_table_offset = LEGACY_U16_WRAP_ADD(resource_offset, 6U);
	offset_table_offset = LEGACY_U16_WRAP_ADD(name_table_offset,
		LEGACY_U16_WRAP_MUL(chunk_count, 4U));
	first_data_offset = LEGACY_U16_WRAP_ADD(resource_offset,
		LEGACY_U16_WRAP_ADD(6U,
			LEGACY_U16_WRAP_MUL(chunk_count, 8U)));
	header_index = (legacy_u16)audioresource_get_chunk_index(0,
		chunk_count, "hdr1", (const legacy_u8 far*)MK_FP(
			resource_segment, name_table_offset));

	for (index = 0; index < chunk_count; ++index) {
		relative_offset = (legacy_u16)audioresource_get_dword(
			(const legacy_u8 far*)MK_FP(resource_segment,
				LEGACY_U16_WRAP_ADD(offset_table_offset,
					LEGACY_U16_WRAP_MUL(index, 4U))));
		chunk_offset = LEGACY_U16_WRAP_ADD(first_data_offset,
			relative_offset);
		bytes = (legacy_u8 far*)MK_FP(resource_segment, chunk_offset);
		chunk_end_offset = LEGACY_U16_WRAP_ADD(chunk_offset,
			(legacy_u16)audioresource_get_dword(bytes));
		cursor_offset = LEGACY_U16_WRAP_ADD(chunk_offset, 4U);

		if (index == header_index) {
			cursor_offset = LEGACY_U16_WRAP_ADD(cursor_offset, 2U);
			cursor = (legacy_u8 far*)MK_FP(resource_segment,
				cursor_offset);
			cursor_offset = LEGACY_U16_WRAP_ADD(cursor_offset,
				LEGACY_U16_WRAP_ADD(
					LEGACY_U16_WRAP_MUL(cursor[0], 4U), 1U));
			cursor = (legacy_u8 far*)MK_FP(resource_segment,
				cursor_offset);
			reference_count = cursor[0];
			cursor_offset = LEGACY_U16_WRAP_ADD(cursor_offset, 1U);
			while (reference_count != 0) {
				cursor = (legacy_u8 far*)MK_FP(resource_segment,
					cursor_offset);
				audio_patch_song_reference(cursor, name_table_offset,
					offset_table_offset, first_data_offset,
					resource_segment, chunk_count);
				cursor_offset = LEGACY_U16_WRAP_ADD(cursor_offset, 5U);
				reference_count--;
			}
			continue;
		}

		while (cursor_offset < chunk_end_offset) {
			cursor = (legacy_u8 far*)MK_FP(resource_segment,
				cursor_offset);
			while ((cursor[0] & 0x80U) != 0) {
				cursor_offset = LEGACY_U16_WRAP_ADD(cursor_offset, 1U);
				cursor = (legacy_u8 far*)MK_FP(resource_segment,
					cursor_offset);
			}
			cursor_offset = LEGACY_U16_WRAP_ADD(cursor_offset, 1U);
			cursor = (legacy_u8 far*)MK_FP(resource_segment,
				cursor_offset);
			event = cursor[0];

			if (event < 0xD9U || event > 0xEAU) {
				if (event >= 0x80U)
					cursor_offset = LEGACY_U16_WRAP_ADD(
						cursor_offset, 1U);
				cursor_offset = LEGACY_U16_WRAP_ADD(cursor_offset, 1U);
				cursor = (legacy_u8 far*)MK_FP(resource_segment,
					cursor_offset);
				while ((cursor[0] & 0x80U) != 0) {
					cursor_offset = LEGACY_U16_WRAP_ADD(
						cursor_offset, 1U);
					cursor = (legacy_u8 far*)MK_FP(resource_segment,
						cursor_offset);
				}
				cursor_offset = LEGACY_U16_WRAP_ADD(cursor_offset, 1U);
				continue;
			}

			switch (event - 0xD9U) {
			case 0:
			case 1:
			case 2:
			case 10:
				cursor_offset = LEGACY_U16_WRAP_ADD(cursor_offset, 1U);
				break;

			case 3:
			case 4:
			case 5:
			case 7:
			case 8:
			case 9:
			case 11:
			case 16:
			case 17:
				cursor_offset = LEGACY_U16_WRAP_ADD(cursor_offset, 2U);
				break;

			case 6:
			case 12:
				cursor_offset = LEGACY_U16_WRAP_ADD(cursor_offset, 3U);
				break;

			case 13:
				cursor_offset = LEGACY_U16_WRAP_ADD(cursor_offset, 2U);
				cursor = (legacy_u8 far*)MK_FP(resource_segment,
					cursor_offset);
				audio_patch_song_reference(cursor, name_table_offset,
					offset_table_offset, first_data_offset,
					resource_segment, chunk_count);
				cursor_offset = LEGACY_U16_WRAP_ADD(cursor_offset, 4U);
				break;

			case 14:
			case 15:
				cursor_offset = LEGACY_U16_WRAP_ADD(cursor_offset, 1U);
				cursor = (legacy_u8 far*)MK_FP(resource_segment,
					cursor_offset);
				cursor_offset = LEGACY_U16_WRAP_ADD(cursor_offset,
					LEGACY_U16_WRAP_ADD(cursor[0], 1U));
				break;
			}
		}
	}
}

void far* init_audio_resources(void far* song, void far* instruments,
	const legacy_s8* name)
{
	legacy_u8 far* song_chunk;
	legacy_u8 far* header;
	legacy_u16 data_offset;
	void far* data;

	song_chunk = (legacy_u8 far*)audioresource_find(song, name);
	if (song_chunk == 0)
		return 0;
	header = (legacy_u8 far*)audioresource_find(song_chunk, "hdr1");
	if (header == 0)
		return 0;

	if (header[5] != 1) {
		audio_map_song_instruments(song_chunk, instruments);
		audio_map_song_tracks(song_chunk);
		header[5] = 1;
		data_offset = LEGACY_U16_WRAP_ADD(
			(legacy_u16)FP_OFF(song_chunk),
			(legacy_u16)((legacy_u16)song_chunk[4] << 3));
		data_offset = LEGACY_U16_WRAP_ADD(data_offset, 1U);
		data = MK_FP(FP_SEG(song_chunk), data_offset);
		audio_write_far_pointer(header, data);
	}

	return header;
}

void load_audio_finalize(void far* audio_resource)
{
	legacy_u8 far* resource;
	legacy_u16 data_offset;

	audio_update_lock = 1;
	sub_3736A();
	resource = (legacy_u8 far*)audio_resource;
	if (resource == 0 || resource[4] != 0 || resource[5] != 1)
		return;

	dos_audio_driver_reset();
	word_44D48 = 0;
	word_454BA = 0x80U;
	data_offset = LEGACY_U16_WRAP_ADD(
		(legacy_u16)((legacy_u16)resource[6] << 2), 7U);
	byte_44290 = resource[data_offset++];
	audio_init_chunk(0,
		LEGACY_S16_FROM_BITS((legacy_u16)(byte_44290 - 1U)),
		resource, data_offset, byte_45950, 0x20U);
	byte_40632 = 1;
	audio_update_lock = 0;
}

void audioresource_copy_n_bytes(const legacy_u8 far* source,
	legacy_u8 far* destination, legacy_s16 size)
{
	legacy_u16 source_offset;
	legacy_u16 source_segment;
	legacy_u16 destination_offset;
	legacy_u16 destination_segment;
	legacy_s16 remaining;

	remaining = LEGACY_S16_FROM_BITS(size);
	if (remaining <= 0)
		return;
	source_offset = (legacy_u16)FP_OFF(source);
	source_segment = (legacy_u16)FP_SEG(source);
	destination_offset = (legacy_u16)FP_OFF(destination);
	destination_segment = (legacy_u16)FP_SEG(destination);
	do {
		*(legacy_u8 far*)MK_FP(destination_segment,
			destination_offset) = *(const legacy_u8 far*)MK_FP(
			source_segment, source_offset);
		source_offset = LEGACY_U16_WRAP_ADD(source_offset, 1U);
		destination_offset = LEGACY_U16_WRAP_ADD(
			destination_offset, 1U);
		remaining = LEGACY_S16_WRAP_SUB(remaining, 1);
	} while (remaining != 0);
}

void audio_op_unk3(legacy_s16 index)
{
	audio_start_indexed_event(index, 0x44U, 0x40U);
}

void audio_op_unk4(legacy_s16 index)
{
	audio_start_indexed_event(index, 0x48U, 0x40U);
}

void audio_function2_wrap(legacy_s16 index)
{
	audio_start_indexed_event(index, 0x38U, 0x64U);
	audio_function2(index);
}

void nopsub_2726C(legacy_s16 index)
{
	audio_start_indexed_event(index, 0x30U, 0x40U);
	audio_function2(index);
}

void nopsub_272B0(legacy_s16 index)
{
	audio_start_indexed_event(index, 0x34U, 0x40U);
	audio_function2(index);
}

static void audio_start_secondary_event(legacy_s16 index,
	legacy_u16 resource_field)
{
	legacy_u16 offset;
	legacy_u16 rate;
	legacy_s16 channel;
	void far* resource;

	offset = LEGACY_U16_WRAP_MUL(index, 0x4CU);
	channel = LEGACY_S16_FROM_BITS(
		LEGACY_READ_U16_LE(audio_timers + offset + 0x16U));
	if (channel != -1)
		audio_init_chunk2(channel);

	rate = LEGACY_READ_U16_LE(audio_timers + offset + 4U) >> 4;
	resource = audio_read_far_pointer(
		audio_timers + offset + resource_field);
	channel = audio_check_flag(resource, -1, 0x40U, rate);
	LEGACY_WRITE_U16_LE(audio_timers + offset + 0x16U, channel);
	audio_timers[offset + 0x1AU] = 1;
}

void audio_op_unk5(legacy_s16 index)
{
	audio_start_secondary_event(index, 0x3CU);
}

void audio_op_unk6(legacy_s16 index)
{
	audio_start_secondary_event(index, 0x40U);
}

static legacy_s16 audio_carstate_position(legacy_s32 position)
{
	legacy_u32 bits;

	bits = (legacy_u32)position;
	bits = (bits >> 6) |
		((bits & 0x80000000UL) != 0 ? 0xFC000000UL : 0);
	return LEGACY_S16_FROM_BITS((legacy_u16)bits);
}

static void audio_carstate_write(legacy_u8* record, legacy_u16 offset,
	legacy_s16 value)
{
	LEGACY_WRITE_U16_LE(record + offset, (legacy_u16)value);
}

static legacy_u8 audio_carstate_update_flags(struct CARSTATE* carstate,
	legacy_s16 channel, legacy_u8 flags)
{
	legacy_u8 desired;

	desired = (legacy_u8)carstate->field_CF;
	if ((desired & 1U) != 0) {
		if ((flags & 1U) == 0) {
			flags = (legacy_u8)(flags | 1U);
			audio_op_unk(channel);
		}
	} else if ((flags & 1U) != 0) {
		flags = (legacy_u8)(flags - 1U);
		audio_function2(channel);
	}

	if ((desired & 6U) != 0) {
		if ((flags & 6U) == (desired & 6U))
			return flags;
		if ((flags & 6U) == 0) {
			if ((desired & 2U) != 0) {
				audio_op_unk5(channel);
				return (legacy_u8)(flags + 2U);
			}
			audio_op_unk6(channel);
			return (legacy_u8)(flags + 4U);
		}
	} else if ((flags & 6U) == 0) {
		return flags;
	}

	if ((flags & 2U) != 0)
		flags = (legacy_u8)(flags - 2U);
	if ((flags & 4U) != 0)
		flags = (legacy_u8)(flags - 4U);
	audio_op_unk7(channel);
	return flags;
}

void audio_carstate(void)
{
	struct VECTOR player_previous;
	struct VECTOR player_current;
	struct VECTOR opponent_previous;
	struct VECTOR opponent_current;
	struct VECTOR camera_previous;
	struct VECTOR camera_current;
	struct CARSTATE* carstate;
	legacy_u8* record;
	legacy_s16 track_index;
	legacy_s16 car_count;
	legacy_s16 car_index;
	legacy_u8 flags;
	legacy_s16 channel;

	if (is_in_replay != 0) {
		if (byte_459D8 != 0) {
			word_44D1E = word_449E4;
			if (((legacy_u8)byte_42D26 & 6U) != 0)
				audio_op_unk7(word_43964);
			if (((legacy_u8)byte_42D26 & 1U) != 0)
				audio_function2(word_43964);
			if (gameconfig.game_opponenttype != 0) {
				if (((legacy_u8)byte_42D2A & 6U) != 0)
					audio_op_unk7(word_4408C);
				if (((legacy_u8)byte_42D2A & 1U) != 0)
					audio_function2(word_4408C);
			}
			byte_459D8 = 0;
			byte_42D26 = 0;
			byte_42D2A = 0;
		}
		if ((legacy_u8)byte_3BE02 != (legacy_u8)is_in_replay)
			audio_reset_channels();
		byte_3BE02 = (legacy_u8)is_in_replay;
		return;
	}

	player_previous.x = audio_carstate_position(
		(legacy_s32)state.playerstate.car_posWorld2.lx);
	player_previous.y = audio_carstate_position(
		(legacy_s32)state.playerstate.car_posWorld2.ly);
	player_previous.z = audio_carstate_position(
		(legacy_s32)state.playerstate.car_posWorld2.lz);
	player_current.x = audio_carstate_position(
		(legacy_s32)state.playerstate.car_posWorld1.lx);
	player_current.y = audio_carstate_position(
		(legacy_s32)state.playerstate.car_posWorld1.ly);
	player_current.z = audio_carstate_position(
		(legacy_s32)state.playerstate.car_posWorld1.lz);

	if (gameconfig.game_opponenttype != 0) {
		opponent_previous.x = audio_carstate_position(
			(legacy_s32)state.opponentstate.car_posWorld2.lx);
		opponent_previous.y = audio_carstate_position(
			(legacy_s32)state.opponentstate.car_posWorld2.ly);
		opponent_previous.z = audio_carstate_position(
			(legacy_s32)state.opponentstate.car_posWorld2.lz);
		opponent_current.x = audio_carstate_position(
			(legacy_s32)state.opponentstate.car_posWorld1.lx);
		opponent_current.y = audio_carstate_position(
			(legacy_s32)state.opponentstate.car_posWorld1.ly);
		opponent_current.z = audio_carstate_position(
			(legacy_s32)state.opponentstate.car_posWorld1.lz);
	}

	if (cameramode == 1) {
		camera_current = state.game_vec1[(legacy_u8)followOpponentFlag];
		camera_previous = followOpponentFlag != 0 ?
			state.game_vec4 : state.game_vec3;
	} else if (cameramode == 3) {
		track_index = LEGACY_S16_FROM_BITS((legacy_u16)(legacy_s8)
			state.field_3F7[(legacy_u8)followOpponentFlag]);
		camera_current.x = trackdata9[track_index * 3];
		camera_current.y = LEGACY_S16_WRAP_ADD(
			LEGACY_S16_WRAP_ADD(trackdata9[track_index * 3 + 1],
				word_44D20), 0x5A);
		camera_current.z = trackdata9[track_index * 3 + 2];
		camera_previous = camera_current;
	} else if (followOpponentFlag != 0) {
		camera_current = opponent_current;
		camera_previous = opponent_previous;
	} else {
		camera_current = player_current;
		camera_previous = player_previous;
	}

	record = unk_44F4C + LEGACY_U16_WRAP_MUL(
		(legacy_u16)word_449E4, 0x22U);
	audio_carstate_write(record, 6U, LEGACY_S16_WRAP_SUB(
		camera_previous.x, player_previous.x));
	audio_carstate_write(record, 8U, LEGACY_S16_WRAP_SUB(
		camera_previous.y, player_previous.y));
	audio_carstate_write(record, 0x0AU, LEGACY_S16_WRAP_SUB(
		camera_previous.z, player_previous.z));
	audio_carstate_write(record, 0x0CU, LEGACY_S16_WRAP_SUB(
		camera_current.x, player_current.x));
	audio_carstate_write(record, 0x0EU, LEGACY_S16_WRAP_SUB(
		camera_current.y, player_current.y));
	audio_carstate_write(record, 0x10U, LEGACY_S16_WRAP_SUB(
		camera_current.z, player_current.z));
	audio_carstate_write(record, 0x1EU,
		state.playerstate.car_currpm);

	car_count = 1;
	if (gameconfig.game_opponenttype != 0) {
		audio_carstate_write(record, 0x12U, LEGACY_S16_WRAP_SUB(
			camera_previous.x, opponent_previous.x));
		audio_carstate_write(record, 0x14U, LEGACY_S16_WRAP_SUB(
			camera_previous.y, opponent_previous.y));
		audio_carstate_write(record, 0x16U, LEGACY_S16_WRAP_SUB(
			camera_previous.z, opponent_previous.z));
		audio_carstate_write(record, 0x18U, LEGACY_S16_WRAP_SUB(
			camera_current.x, opponent_current.x));
		audio_carstate_write(record, 0x1AU, LEGACY_S16_WRAP_SUB(
			camera_current.y, opponent_current.y));
		audio_carstate_write(record, 0x1CU, LEGACY_S16_WRAP_SUB(
			camera_current.z, opponent_current.z));
		audio_carstate_write(record, 0x20U,
			state.opponentstate.car_currpm);
		car_count = 2;
	}

	for (car_index = 0; car_index < car_count; car_index++) {
		if (car_index == 0) {
			carstate = &state.playerstate;
			channel = word_43964;
			flags = (legacy_u8)byte_42D26;
		} else {
			carstate = &state.opponentstate;
			channel = word_4408C;
			flags = (legacy_u8)byte_42D2A;
		}
		flags = audio_carstate_update_flags(carstate, channel, flags);
		if (car_index == 0)
			byte_42D26 = (legacy_s8)flags;
		else
			byte_42D2A = (legacy_s8)flags;
	}

	byte_459D8 = 1;
	word_449E4 = LEGACY_S16_WRAP_ADD(word_449E4, 1);
	if (word_449E4 == 0x28)
		word_449E4 = 0;
	byte_3BE02 = (legacy_u8)is_in_replay;
}

void sub_374DE(legacy_s16 channel)
{
	if (channel > -1) {
		byte_45D9A[channel] = 0;
		audio_init_chunk2(channel);
	}
}

void sub_38156(legacy_s16 index)
{
	legacy_u16 offset;

	offset = LEGACY_U16_WRAP_MUL(index, 0x2EU);
	LEGACY_WRITE_U16_LE(dos_audio_contexts + offset + 0x0CU, 1);
	LEGACY_WRITE_U16_LE(dos_audio_contexts + offset + 0x0EU, 0);
}

legacy_s16 sub_37868(legacy_s16 value)
{
	legacy_s16 channel;

	for (channel = 0; channel < (legacy_u16)byte_44290; channel++)
		dos_audio_set_channel_volume(channel, value);

	return channel;
}

void audio_fade_out(legacy_s16 delay_ticks)
{
	legacy_s16 volume;
	legacy_u32 delay;

	delay = (legacy_u32)(legacy_s32)delay_ticks;
	if (dos_audio_uses_direct_channels != 0) {
		volume = 0x64;
		do {
			audio_update_lock = 1;
			dos_audio_master_volume = (legacy_u8)volume;
			dos_audio_driver_set_master_state(
				4, (void far*)dos_audio_master_state);
			audio_update_lock = 0;
			timer_copy_counter(delay);
			timer_wait_for_dx();
			volume = LEGACY_S16_WRAP_SUB(volume, 2);
		} while (volume > 0);
	} else {
		volume = byte_45950;
		while (volume > 0) {
			audio_update_lock = 1;
			sub_37868(volume);
			audio_update_lock = 0;
			timer_copy_counter(delay);
			timer_wait_for_dx();
			volume = LEGACY_S16_WRAP_SUB(volume, 2);
		}
	}

	sub_3736A();
	if (dos_audio_uses_direct_channels != 0) {
		timer_copy_counter(0x32UL);
		timer_wait_for_dx();
		dos_audio_master_volume = 0x64U;
		dos_audio_driver_set_master_state(4, (void far*)dos_audio_master_state);
	}
}

legacy_s16 nopsub_37898(legacy_s16 value)
{
	byte_45950 = (legacy_u8)value;
	return sub_37868(value);
}

void sub_37C38(legacy_s16 value)
{
	word_4063C = value;
}

legacy_u16 nopsub_378AE(legacy_s16 channel)
{
	return (legacy_u16)byte_44D06[(legacy_u16)channel];
}

legacy_u16 nopsub_378BC(legacy_s16 channel)
{
	return (legacy_u16)byte_44ACA[(legacy_u16)channel];
}

void audio_unk3(legacy_u8 flags, legacy_s16 channel)
{
	if (byte_459D8 == 0)
		return;

	if ((flags & 0x10U) != 0)
		audio_op_unk4(channel);
	if ((flags & 0x20U) != 0)
		audio_op_unk3(channel);
}

void load_sdgame2_shapes(void)
{
	legacy_s16 i;

	sdgame2ptr = file_load_resource(8, "sdgame2");
	locate_many_resources(
		sdgame2ptr,
		"ex01ex02ex03leftrigh",
		sdgame2shapes);
	for (i = 0; i < 3; i++)
		sdgame2_widths[i] =
			((struct SHAPE2D far*)sdgame2shapes[i])->s2d_width;
}

void load_skybox(legacy_s8 skybox_index)
{
	legacy_u16 minimum;
	legacy_u16 maximum;

	if (((legacy_u8)skybox_index & 8U) == 0) {
		if (byte_3B8F6 != 0 &&
			(legacy_u8)skybox_index == (legacy_u8)byte_46167)
			return;

		unload_skybox();
		byte_46167 = skybox_index;
		byte_3B8F6 = 1;
		skybox_res_ofs = file_load_shape2d_fatal(
			skybox_resource_names[(legacy_s8)skybox_index]);
		locate_many_resources(
			skybox_res_ofs,
			"scensce2sce3sce4",
			skyboxes);

		skybox_ptr1 =
			((struct SHAPE2D far*)skyboxes[0])->s2d_height;
		skybox_ptr2 =
			((struct SHAPE2D far*)skyboxes[1])->s2d_height;
		skybox_ptr3 =
			((struct SHAPE2D far*)skyboxes[2])->s2d_height;
		skybox_ptr4 =
			((struct SHAPE2D far*)skyboxes[3])->s2d_height;

		minimum = skybox_ptr1;
		if (minimum > skybox_ptr2)
			minimum = skybox_ptr2;
		if (minimum > skybox_ptr3)
			minimum = skybox_ptr3;
		if (minimum > skybox_ptr4)
			minimum = skybox_ptr4;
		skybox_current = minimum;

		maximum = skybox_ptr1;
		if (maximum < skybox_ptr2)
			maximum = skybox_ptr2;
		if (maximum < skybox_ptr3)
			maximum = skybox_ptr3;
		if (maximum < skybox_ptr4)
			maximum = skybox_ptr4;
		word_454CE = maximum;
	}

	skybox_sky_color = material_clrlist_ptr[17];
	skybox_grd_color = material_clrlist_ptr[16];
	skybox_wat_color = material_clrlist_ptr[100];
	meter_needle_color = dialog_fnt_colour;
}

extern legacy_s8 aCarcoun[];
extern void far* engptr;
extern void far* eng1ptr;
extern void far* fontledresptr;
extern void far* sdgameresptr;
extern void far* wallptr;
extern void far* planptr;
extern legacy_s8 unk_3E7FC[];
extern legacy_s8 unk_3E82C[];
extern legacy_s16 audio_init_engine(legacy_s16, void far*, void far*, void far*);

static legacy_s16 setup_player_cars_impl(legacy_s16 load_dashboard_shapes) {
	void far* carresptr;
	legacy_u32 var_8;

	wndsprite = 0;
	ensure_file_exists(2);
	shape3d_load_car_shapes(gameconfig.game_playercarid, gameconfig.game_opponentcarid);
	aCarcoun[3] = gameconfig.game_playercarid[0];
	aCarcoun[4] = gameconfig.game_playercarid[1];
	aCarcoun[5] = gameconfig.game_playercarid[2];
	aCarcoun[6] = gameconfig.game_playercarid[3];
	carresptr = file_load_resfile(aCarcoun);
	setup_aero_trackdata(carresptr, 0);
	unload_resource(carresptr);

	if (gameconfig.game_opponenttype != 0) {
		aCarcoun[3] = gameconfig.game_opponentcarid[0];
		aCarcoun[4] = gameconfig.game_opponentcarid[1];
		aCarcoun[5] = gameconfig.game_opponentcarid[2];
		aCarcoun[6] = gameconfig.game_opponentcarid[3];
		carresptr = file_load_resfile(aCarcoun);
		setup_aero_trackdata(carresptr, 1);
		unload_resource(carresptr);
		
		ensure_file_exists(4);
		load_opponent_data();
	}

	ensure_file_exists(3);
	eng1ptr = file_load_resource(5, "eng1");//aEng1); // "eng1"
	engptr = file_load_resource(6, "eng");//aEng); // "eng"
	audio_add_driver_timer();
	word_43964 = audio_init_engine(0x21, &unk_3E7FC, eng1ptr, engptr);

	byte_459D8 = 0;
	byte_42D26 = 0;
	byte_42D2A = 0;
	if (gameconfig.game_opponenttype != 0) {
		word_4408C = audio_init_engine(0x20, &unk_3E82C, eng1ptr, engptr);
	}

	word_44D1E = 0;
	word_449E4 = 0;
	word_443F4 = 0;
	fontledresptr = file_load_resource(0, "fontled.fnt");//aFontled_fnt); // "fontled.fnt"
	slow_video_mgmt_copy = slow_video_mgmt;
	init_rect_arrays();
	/* REPLDUMP advances simulation without rendering the dashboard.  Keep the
	 * car 3D container in its original arena position because later legacy
	 * state still observes that memory layout, but avoid the much larger 2D
	 * dashboard allocation that memory-heavy custom cars cannot afford. */
	if (idle_expired == 0 && load_dashboard_shapes) {
		setup_car_shapes(0);
	}

	if (idle_expired == 0) {
		sdgameresptr = file_load_resource(3, "sdgame");//aSdgame); // "sdgame"
		loop_game(0, 0, 0);
	}

	gameresptr = file_load_resfile("game");
	planptr = locate_shape_alt(gameresptr, "plan");//aPlan); // "plan"
	wallptr = locate_shape_alt(gameresptr, "wall");//aWall); // "wall"
	load_sdgame2_shapes();
	load_skybox(td14_elem_map_main[0x384]);
	if (shape3d_load_all() != 0) {
		return 1;
	}

	if (video_flag5_is0 == 0) {
		// The free-arena check only applies when the window has to come from
		// the arena; 0xFA2 paras is the full 320x200 window incl. header.
		if (!highpool_can_fit(0xFA2)) {
			var_8 = 0xFA00 / (video_flag1_is1 * video_flag4_is1);
			if (mmgr_get_res_ofs_diff_scaled() <= var_8) {
				return 1;
			}
		}
		wndsprite = sprite_make_wnd(0x140, 0xC8, 0x0F);
	}

	followOpponentFlag = 0;
	is_in_replay_copy = -1;
	return 0;
}

legacy_s16 setup_player_cars(void) {
	return setup_player_cars_impl(1);
}

legacy_s16 setup_player_cars_repldump(void) {
	return setup_player_cars_impl(0);
}

void free_player_cars(void) {
	if (video_flag5_is0 == 0) {
		if (wndsprite != 0) {
			sprite_free_wnd(wndsprite);
		}
	}
	shape3d_free_all();
	unload_skybox();
	free_sdgame2();
	unload_resource(gameresptr);
	if (idle_expired == 0) {
		mmgr_free(sdgameresptr);
		setup_car_shapes(3);
	}

	mmgr_free(fontledresptr);
	audio_remove_driver_timer();
	mmgr_free(engptr);
	mmgr_free(eng1ptr);
	shape3d_free_car_shapes();
}

static legacy_s16 dashboard_steering_position(legacy_s16 angle)
{
	legacy_s16 magnitude;
	legacy_u16 bits;

	magnitude = angle < 0 ? LEGACY_S16_WRAP_NEGATE(angle) : angle;
	bits = (legacy_u16)magnitude;
	bits = (legacy_u16)((bits >> 3) |
		((bits & 0x8000U) != 0 ? 0xE000U : 0U));
	magnitude = LEGACY_S16_FROM_BITS(bits);
	return angle < 0 ? LEGACY_S16_WRAP_NEGATE(magnitude) : magnitude;
}

static legacy_u8 dashboard_clear_steering_dot(legacy_u16 player_index)
{
	if (word_40DF6[player_index] == 0)
		return 0;
	sprite_putimage_and_alt(
		gnobshapes[4U + (legacy_u8)byte_44346],
		word_40DF2[player_index], word_40DF6[player_index]);
	word_40DF6[player_index] = 0;
	return 1;
}

static void dashboard_set_viewport(void)
{
	sprite_set_1_size(0, 0x140, 0, height_above_replaybar);
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

	if (operation == 0) {
		for (index = 0; index < 4U; index++) {
			aStdaxxxx[index + 4U] = gameconfig.game_playercarid[index];
			aStdbxxxx[index + 4U] = gameconfig.game_playercarid[index];
		}
		stdaresptr = (legacy_s8 far*)file_load_resource(3, aStdaxxxx);
		stdbresptr = (legacy_s8 far*)file_load_resource(2, aStdbxxxx);
		locate_many_resources(stdaresptr, aWhl1whl2whl3ins2gboxins1i,
			(legacy_s8 far**)whlshapes);
		locate_many_resources(stdbresptr, aGnobgnabdotDotadot1dot2,
			(legacy_s8 far**)gnobshapes);
		if (simd_player.spdcenter.py == 0) {
			locate_many_resources(stdbresptr,
				aDig0dig1dig2dig3dig4dig5d, (legacy_s8 far**)digshapes);
		}

		whlsprite1 = sprite_make_wnd(
			LEGACY_U16_WRAP_MUL(whlshapes[3]->s2d_width,
				(legacy_u16)video_flag1_is1),
			whlshapes[3]->s2d_height, 0x0FU);
		whlsprite2 = sprite_make_wnd(
			LEGACY_U16_WRAP_MUL(whlshapes[4]->s2d_width,
				(legacy_u16)video_flag1_is1),
			whlshapes[4]->s2d_height, 0x0FU);
		whlsprite3 = sprite_make_wnd(
			LEGACY_U16_WRAP_MUL(whlshapes[4]->s2d_width,
				(legacy_u16)video_flag1_is1),
			whlshapes[4]->s2d_height, 0x0FU);

		dashboard_shape = (struct SHAPE2D far*)
			locate_shape_fatal(stdaresptr, aDash);
		gearbox_shape = whlshapes[4];
		sprite_set_1_from_argptr(whlsprite3);
		shape2d_op_unk2(dashboard_shape,
			LEGACY_S16_WRAP_SUB(
				(legacy_s16)dashboard_shape->s2d_pos_x,
				(legacy_s16)gearbox_shape->s2d_pos_x),
			LEGACY_S16_WRAP_SUB(
				(legacy_s16)dashboard_shape->s2d_pos_y,
				(legacy_s16)gearbox_shape->s2d_pos_y));
		sprite_copy_2_to_1();
		dashbmp_y = dashboard_shape->s2d_pos_y;

		shape = (struct SHAPE2D far*)locate_shape_nofatal(stdaresptr, aRoof);
		if (shape != 0) {
			shape = (struct SHAPE2D far*)locate_shape_fatal(stdaresptr, aRoof);
			roofbmpheight = shape->s2d_height;
		} else {
			roofbmpheight = 0;
		}

		shape = (struct SHAPE2D far*)locate_shape_nofatal(stdaresptr, aDast);
		if (shape != 0) {
			dastbmp_y = shape->s2d_pos_y;
			dastbmp_y2 = FP_OFF(shape);
			dastseg = FP_SEG(shape);
			dasmshapeptr = locate_shape_fatal(stdaresptr, aDasm);
		} else {
			dastbmp_y = 0;
		}
		return;
	}

	if (operation == 1) {
		mouse_draw_opaque_check();
		shape = (struct SHAPE2D far*)locate_shape_nofatal(stdaresptr, aRoof);
		if (shape != 0)
			shape2d_op_unk((struct SHAPE2D far*)
				locate_shape_fatal(stdaresptr, aRoof));
		shape2d_op_unk3((struct SHAPE2D far*)
			locate_shape_fatal(stdaresptr, aDash));
		shape2d_op_unk3(whlshapes[1]);
		mouse_draw_transparent_check();

		player_index = (legacy_u8)byte_4432A;
		byte_449D8[player_index] = 0;
		byte_40DFA[player_index] = 0;
		word_40DF6[player_index] = 0;
		byte_40DF0[player_index] = 0;
		word_40E00[player_index] = -1;
		word_40D78[player_index] = -1;
		word_40D6C[player_index] = -1;
		return;
	}

	if (operation == 3) {
		sprite_free_wnd(whlsprite3);
		sprite_free_wnd(whlsprite2);
		sprite_free_wnd(whlsprite1);
		mmgr_free(stdbresptr);
		mmgr_free(stdaresptr);
		return;
	}
	if (operation != 2)
		return;

	player_index = (legacy_u8)byte_4432A;
	steering_dot_cleared = 0;
	if (state.playerstate.car_fpsmul2 == 0 &&
		state.playerstate.car_changing_gear == 0 &&
		byte_40DFA[player_index] != 0) {
		if (video_flag5_is0 == 0)
			mouse_draw_opaque_check();
		dashboard_set_viewport();
		sprite_putimage_and_alt(whlsprite3->sprite_bitmapptr,
			whlshapes[4]->s2d_pos_x, whlshapes[4]->s2d_pos_y);
		byte_40DFA[player_index] = 0;
	} else if (byte_40DFA[player_index] !=
			(legacy_u8)state.playerstate.car_changing_gear ||
		word_40D70[player_index] != state.playerstate.car_knob_x ||
		word_40D74[player_index] != state.playerstate.car_knob_y ||
		(state.playerstate.car_fpsmul2 != 0 &&
			byte_40DFA[player_index] == 0)) {
		sprite_set_1_from_argptr(whlsprite2);
		byte_40DFA[player_index] = 1;
		shape2d_op_unk2(whlshapes[4], 0, 0);
		word_40D70[player_index] = state.playerstate.car_knob_x;
		word_40D74[player_index] = state.playerstate.car_knob_y;
		sprite_putimage_and_alt2(gnobshapes[1],
			state.playerstate.car_knob_x, state.playerstate.car_knob_y);
		sprite_putimage_or_alt(gnobshapes[0],
			state.playerstate.car_knob_x, state.playerstate.car_knob_y);
		if (video_flag5_is0 != 0) {
			setup_mcgawnd2();
		} else {
			sprite_copy_2_to_1_2();
			mouse_draw_opaque_check();
		}
		dashboard_set_viewport();
		sprite_putimage_and_alt(whlsprite2->sprite_bitmapptr,
			whlshapes[4]->s2d_pos_x, whlshapes[4]->s2d_pos_y);
	}

	steering_position = dashboard_steering_position(
		state.playerstate.car_steeringAngle);
	wheel_state = 1;
	if (steering_position < -10)
		wheel_state = 0;
	else if (steering_position > 10)
		wheel_state = 2;
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

	if (simd_player.spdcenter.py == -1) {
		speed_index = 0;
		gauge_mode = 2;
	} else if (simd_player.spdcenter.py == 0) {
		speed_index = (legacy_u16)state.playerstate.car_speed >> 8;
		gauge_mode = 1;
	} else {
		speed_index = (legacy_u16)state.playerstate.car_speed / 0x280U;
		if ((legacy_s16)speed_index >= simd_player.spdnumpoints)
			speed_index = (legacy_u16)(simd_player.spdnumpoints - 1);
		gauge_mode = 0;
	}
	rpm_index = (legacy_u16)state.playerstate.car_currpm >> 7;
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
		shape2d_op_unk5(whlshapes[3], 0, 0);
		word_40D78[player_index] = (legacy_s16)speed_index;
		word_40D6C[player_index] = (legacy_s16)rpm_index;

		if (gauge_mode == 1) {
			digit_started = 0;
			digit_group = 0;
			if (speed_index >= 200U) {
				digit_group = 2;
				speed_index -= 200U;
			} else if (speed_index >= 100U) {
				digit_group = 1;
				speed_index -= 100U;
			}
			if (digit_group != 0) {
				sprite_putimage_or(digshapes[digit_group],
					(legacy_u8)simd_player.spdpoints[0],
					(legacy_u8)simd_player.spdpoints[1]);
				digit_started = 1;
			}
			digit = speed_index / 10U;
			if (digit != 0 || digit_started != 0) {
				sprite_putimage_or(digshapes[digit],
					(legacy_u8)simd_player.spdpoints[2],
					(legacy_u8)simd_player.spdpoints[3]);
				speed_index -= digit * 10U;
			}
			sprite_putimage_or(digshapes[speed_index],
				(legacy_u8)simd_player.spdpoints[4],
				(legacy_u8)simd_player.spdpoints[5]);
		} else if (gauge_mode == 0) {
			dot_index = speed_index * 2U;
			preRender_line(simd_player.spdcenter.px,
				simd_player.spdcenter.py,
				(legacy_u8)simd_player.spdpoints[dot_index],
				(legacy_u8)simd_player.spdpoints[dot_index + 1U],
				meter_needle_color);
		}

		dot_index = rpm_index * 2U;
		preRender_line(simd_player.revcenter.px, simd_player.revcenter.py,
			(legacy_u8)simd_player.revpoints[dot_index],
			(legacy_u8)simd_player.revpoints[dot_index + 1U],
			meter_needle_color);
		if (wheel_state == 0) {
			shape2d_render_bmp_as_mask(whlshapes[7]);
			shape2d_op_unk4(FP_OFF(whlshapes[5]), FP_SEG(whlshapes[5]));
		} else if (wheel_state == 2) {
			shape2d_render_bmp_as_mask(whlshapes[8]);
			shape2d_op_unk4(FP_OFF(whlshapes[6]), FP_SEG(whlshapes[6]));
		}
		if (video_flag5_is0 != 0)
			setup_mcgawnd2();
		else
			sprite_copy_2_to_1_2();
		dashboard_set_viewport();
		sprite_putimage_and_alt(whlsprite1->sprite_bitmapptr,
			whlshapes[3]->s2d_pos_x, whlshapes[3]->s2d_pos_y);
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
			steering_position) * 2U;
		dot_x = steering_dots[dot_index];
		dot_y = steering_dots[dot_index + 1U];
		if (steering_position < 0) {
			dot_x = (legacy_u8)(dot_x -
				(legacy_u8)((legacy_u8)(dot_x - steering_dots[0]) << 1));
		}
		word_40DF2[player_index] = LEGACY_S16_FROM_BITS(
			((legacy_u16)((legacy_u8)dot_x - gnobshapes[2]->s2d_unk1)) &
			(legacy_u16)video_flag3_isFFFF);
		word_40DF6[player_index] = LEGACY_S16_FROM_BITS(
			LEGACY_U16_WRAP_SUB((legacy_u8)dot_y,
				gnobshapes[2]->s2d_unk2));
		sprite_clear_shape_alt(
			gnobshapes[4U + (legacy_u8)byte_44346],
			word_40DF2[player_index], word_40DF6[player_index]);
		sprite_putimage_and_alt2(gnobshapes[3], dot_x, dot_y);
		sprite_putimage_or_alt(gnobshapes[2], dot_x, dot_y);
		word_40E00[player_index] = steering_position;
	}
	mouse_draw_transparent_check();
}

extern void update_car_speed(legacy_s8 input, legacy_s16 multiplayer,
	struct CARSTATE* carstate, struct SIMD* simd);

static void car_menu_draw_standard_button(legacy_s8 far* text,
	legacy_u16 button_index)
{
	draw_button(text,
		LEGACY_S16_WRAP_ADD(carmenu_buttons_y1[0], 1),
		LEGACY_S16_WRAP_ADD(carmenu_buttons_x1[button_index], 1),
		0x56, 0x10, word_407F4, word_407F6, word_407F8, 0);
}

void run_car_menu(legacy_s8* car_id, legacy_s8* material, legacy_s8* transmission,
	legacy_u16 opponent_type)
{
	legacy_s8 car_ids[32][5];
	legacy_s8 swap_id[5];
	const legacy_s8* found_path;
	legacy_s8 far* car_resource;
	legacy_s8 far* description;
	legacy_s8 far* transmission_text;
	void far* selector_resource;
	struct SHAPE2D far* opponent_shape;
	struct SHAPE2D far* shape;
	struct SPRITE far* opponent_sprite;
	struct TRANSFORMEDSHAPE3D transformed;
	struct RECTANGLE current_rect;
	struct RECTANGLE previous_rect;
	struct RECTANGLE union_rect;
	legacy_u8 car_count;
	legacy_u8 car_index;
	legacy_u8 previous_car_index;
	legacy_u8 selected;
	legacy_u8 previous_selected;
	legacy_u8 blit_mode;
	legacy_u8 render_phase;
	legacy_u8 car_ready;
	legacy_u8 character;
	legacy_u16 i;
	legacy_u16 j;
	legacy_u16 line_length;
	legacy_u16 old_frame_rate;
	legacy_u16 input;
	legacy_u16 speed;
	legacy_u16 graph_x;
	legacy_u16 graph_y;
	legacy_u16 graph_step;
	legacy_s16 car_position_angle;
	legacy_s16 rotation;
	legacy_s16 rotation_delta;
	legacy_s16 text_y;
	legacy_s16 mouse_hit;

	transformed.pos = carmenu_carpos;
	transformed.shapeptr = &game3dshapes[124];
	transformed.rotvec.x = 0;
	transformed.rotvec.y = 0;
	transformed.unk = 0x7530U;
	slow_video_mgmt_copy = slow_video_mgmt;
	if (slow_video_mgmt_copy != 0) {
		transformed.rectptr = &current_rect;
		transformed.ts_flags = 8;
	} else {
		transformed.rectptr = 0;
		transformed.ts_flags = 0;
	}

	ensure_file_exists(2);
	found_path = file_combine_and_find(0, aCar, a_res_0);
	if (found_path == 0)
		return;
	car_count = 0;
	do {
		for (i = 0; i < 4U; i++)
			car_ids[car_count][i] = found_path[i + 3U];
		car_ids[car_count][4] = 0;
		car_count++;
		if (car_count >= 32U)
			break;
		found_path = file_find_next_alt();
	} while (found_path != 0);

	for (i = 0; i + 1U < car_count; i++) {
		for (j = i + 1U; j < car_count; j++) {
			if (strcmp(car_ids[i], car_ids[j]) > 0) {
				strcpy(swap_id, car_ids[i]);
				strcpy(car_ids[i], car_ids[j]);
				strcpy(car_ids[j], swap_id);
			}
		}
	}

	car_index = 0;
	for (i = 0; i < car_count; i++) {
		for (j = 0; j < 4U; j++) {
			if (car_ids[i][j] != car_id[j])
				break;
		}
		if (j == 4U)
			car_index = (legacy_u8)i;
	}

	waitflag = 0x5AU;
	blit_mode = 0xFFU;
	backlights_paint_override = 0x2D;
	selector_resource = file_load_shape2d_fatal(aSdcsel);
	opponent_sprite = 0;
	if (opponent_type == 0)
		miscptr = file_load_resfile(aMisc_0);

	if (opponent_type != 0) {
		rect_unk16.right = 0xF0;
		if (video_flag5_is0 != 0) {
			opponent_shape = (struct SHAPE2D far*)
				oppresources[(legacy_u16)opponent_type];
			opponent_sprite = sprite_make_wnd(opponent_shape->s2d_width,
				opponent_shape->s2d_height, 0x0FU);
			setup_mcgawnd2();
			sprite_clear_1_color(0);
			sprite_putimage_transparent(opponent_shape, 0, 0);
			sprite_clear_shape_alt(opponent_sprite->sprite_bitmapptr,
				0, 0);
		}
	} else {
		rect_unk16.right = 0x140;
	}

	previous_car_index = 0xFFU;
	rotation = 0;
	selected = 0;
	sub_29772();
	rotation_delta = 0;
	previous_selected = 0xFFU;
	set_projection(0x24, 0x11, 0x140, 0x64);
	(void)timer_get_delta_alt();
	wndsprite = sprite_make_wnd(0x140U, 0xC8U, 0x0FU);

car_menu_top:
	if (previous_car_index != car_index) {
		if (previous_car_index != 0xFFU) {
			unload_resource(car_resource);
			shape3d_free_car_shapes();
		}

		shape3d_load_car_shapes(car_ids[car_index],
			gameconfig.game_opponentcarid);
		for (i = 0; i < 4U; i++)
			aCarcoun[i + 3U] = car_ids[car_index][i];
		car_resource = (legacy_s8 far*)file_load_resfile(aCarcoun);
		setup_aero_trackdata(car_resource, 0);

		sprite_copy_wnd_to_1_clear();
		draw_button(0, 0, 0x67, 0x140, 0x61,
			word_407F4, word_407F6, word_407F8, 0);
		draw_button(0, 5, 0x6D, 0x46, 0x55,
			word_407F4, word_407F6, word_407F8, 0);
		draw_button(0, 0x52, 0x6D, 0x8C, 0x55,
			word_407F4, word_407F6, word_407F8, 0);
		shape = (struct SHAPE2D far*)locate_shape_fatal(
			selector_resource, aGrap);
		sprite_shape_to_1_alt(shape);

		font_set_fontdef2(fontnptr);
		font_set_unk(0, dialog_fnt_colour);
		font_draw_text(a150, 9, 0x73);
		font_draw_text(a100, 9, 0x87);
		font_draw_text(a50, 9, 0x9B);
		font_draw_text(a0, 9, 0xAF);
		font_draw_text(a02040, 0x1A, 0xB9);
		font_set_fontdef();

		car_menu_draw_standard_button(
			locate_text_res(miscptr, aBdo_0), 0);
		car_menu_draw_standard_button(
			locate_text_res(miscptr, aBnx_0), 1);
		car_menu_draw_standard_button(
			locate_text_res(miscptr, aBla_0), 2);
		transmission_text = locate_text_res(miscptr,
			*transmission != 0 ? aBau : aBma);
		car_menu_draw_standard_button(transmission_text, 3);
		car_menu_draw_standard_button(
			locate_text_res(miscptr, aBco), 4);

		old_frame_rate = (legacy_u16)framespersec;
		framespersec = 0x14;
		init_game_state(-2);
		state.playerstate.car_transmission = 1;
		graph_step = 0;
		for (;;) {
			update_car_speed(1, 0, &state.playerstate, &simd_player);
			speed = (legacy_u16)state.playerstate.car_speed >> 8;
			graph_y = LEGACY_U16_WRAP_SUB(0xB5U,
				(legacy_u16)(((legacy_u32)speed << 6) / 0x96UL));
			if (graph_y < 0x75U)
				break;
			graph_x = (legacy_u16)(((legacy_u32)0x26U *
				graph_step) / 0x320UL) + 0x1CU;
			putpixel_single_maybe(graph_x, graph_y,
				performGraphColor);
			graph_step++;
			if (graph_step >= 0x320U)
				break;
		}
		framespersec = (legacy_s16)old_frame_rate;

		font_set_fontdef2(fontnptr);
		description = locate_text_res(car_resource, aDes_1);
		line_length = 0;
		text_y = 0x74;
		do {
			character = (legacy_u8)*description++;
			if (character == ']') {
				if (line_length != 0) {
					(&resID_byte1)[line_length] = 0;
					font_draw_text(&resID_byte1, 0x58, text_y);
				}
				line_length = 0;
				text_y = LEGACY_S16_WRAP_ADD(text_y,
					fontdef_unk_0E);
			} else {
				(&resID_byte1)[line_length++] = (legacy_s8)character;
			}
		} while (*description != 0);
		font_set_fontdef();
		(void)timer_get_delta_alt();
		previous_selected = 0xFFU;
		previous_rect.left = 0;
		previous_rect.right = 0x140;
		previous_rect.top = 0;
		previous_rect.bottom = 0xC8;
		car_ready = 0;
		render_phase = 3;
	}

	rotation = LEGACY_S16_WRAP_ADD(rotation, rotation_delta);
	if (render_phase == 0 || render_phase == 3) {
		car_position_angle = (legacy_s16)polarAngle(
			carmenu_carpos.y, carmenu_carpos.z);
		current_rect = slow_video_mgmt_copy != 0 ?
			cliprect_unk : carmenu_cliprect;
		select_cliprect_rotate(0, car_position_angle, 0,
			&carmenu_cliprect, 0);
		if ((legacy_s8)(legacy_u8)*material >=
			(legacy_s8)(legacy_u8)game3dshapes[124].shape3d_numpaints)
			*material = 0;
		transformed.rotvec.z = rotation;
		transformed.material = (legacy_u8)*material;
		transformed_shape_op(&transformed);
		rect_unk16.bottom = previous_car_index == car_index ?
			0x5F : 0xC8;
		(void)rect_intersect(&current_rect, &rect_unk16);
		rect_union(&current_rect, &previous_rect, &union_rect);
		if (render_phase != 3) {
			render_phase = 1;
			goto car_menu_input;
		}
	}

	if (render_phase == 1 || render_phase == 3) {
		render_phase = 0;
		car_ready = 1;
		sprite_copy_wnd_to_1();
		sprite_set_1_size(union_rect.left, union_rect.right,
			union_rect.top, union_rect.bottom);
		sprite_putimage((struct SHAPE2D far*)locate_shape_fatal(
			selector_resource, aStop_1));
		get_a_poly_info();
		sprite_copy_wnd_to_1();
		sprite_set_1_size(union_rect.left, union_rect.right,
			union_rect.top, union_rect.bottom);
		previous_rect = current_rect;

		if (opponent_type != 0 && previous_car_index != car_index) {
			sprite_copy_wnd_to_1();
			if (video_flag5_is0 == 0) {
				sprite_putimage_transparent(
					(struct SHAPE2D far*)oppresources[
						(legacy_u16)opponent_type], 0xF0, 0);
			} else {
				sprite_putimage_and_alt(
					opponent_sprite->sprite_bitmapptr, 0xF0, 0);
			}
		}

		sprite_copy_2_to_1_2();
		sprite_set_1_size(union_rect.left, union_rect.right,
			union_rect.top, union_rect.bottom);
		mouse_draw_opaque_check();
		if (blit_mode != 0xFEU) {
			(void)sprite_blit_to_video(wndsprite,
				LEGACY_S8_FROM_BITS(blit_mode));
			blit_mode = 0xFEU;
		} else {
			sprite_putimage(wndsprite->sprite_bitmapptr);
		}
		mouse_draw_transparent_check();
		previous_car_index = car_index;
	}

car_menu_input:
	if (previous_selected != selected) {
		if (previous_selected != 0xFFU) {
			sprite_copy_2_to_1_2();
			sprite_set_1_size(carmenu_buttons_y1[0],
				LEGACY_S16_FROM_BITS((legacy_u16)(
					(LEGACY_U16_WRAP_ADD(carmenu_buttons_y2[0],
						video_flag2_is1)) &
					(legacy_u16)video_flag3_isFFFF)),
				carmenu_buttons_x1[0],
				LEGACY_S16_WRAP_ADD(carmenu_buttons_x2[4], 1));
			mouse_draw_opaque_check();
			sprite_putimage(wndsprite->sprite_bitmapptr);
			mouse_draw_transparent_check();
			sprite_copy_2_to_1_2();
		}
		sub_29772();
		previous_selected = selected;
	}

	sprite_copy_2_to_1_2();
	rotation_delta = (legacy_s16)mouse_timer_sprite_unk(selected,
		carmenu_buttons_y1, carmenu_buttons_y2,
		carmenu_buttons_x1, carmenu_buttons_x2,
		word_407CE, word_407D0);
	idle_counter = LEGACY_U16_WRAP_ADD(idle_counter, rotation_delta);
	if (LEGACY_S16_FROM_BITS((legacy_u16)idle_counter) > 0x2EE0) {
		idle_counter = 0;
		idle_expired = (legacy_u8)(idle_expired + 1U);
	}
	input = (legacy_u16)input_checking(rotation_delta);
	mouse_hit = (legacy_s16)mouse_multi_hittest(5,
		carmenu_buttons_y1, carmenu_buttons_y2,
		carmenu_buttons_x1, carmenu_buttons_x2);
	if (mouse_hit != -1)
		selected = (legacy_u8)mouse_hit;
	if (idle_expired != 0) {
		selected = 0;
		input = 0x0DU;
	}

	if (input == 0)
		goto car_menu_top;
	if (input == 0x4800U) {
		selected = selected == 0 ? 4U : (legacy_u8)(selected - 1U);
		goto car_menu_top;
	}
	if (input == 0x5000U) {
		selected = selected >= 4U ? 0U : (legacy_u8)(selected + 1U);
		goto car_menu_top;
	}
	if (input != 0x0DU && input != 0x1BU && input != 0x20U)
		goto car_menu_top;

	if (selected == 0) {
		if (car_ready == 0)
			goto car_menu_top;
	} else if (selected == 1) {
		car_index++;
		if (car_index == car_count)
			car_index = 0;
		goto car_menu_top;
	} else if (selected == 2) {
		car_index = car_index == 0 ?
			(legacy_u8)(car_count - 1U) : (legacy_u8)(car_index - 1U);
		goto car_menu_top;
	} else if (selected == 3) {
		*transmission = (legacy_s8)((legacy_u8)*transmission ^ 1U);
		sprite_copy_wnd_to_1();
		transmission_text = locate_text_res(miscptr,
			*transmission != 0 ? aBau_0 : aBma_0);
		car_menu_draw_standard_button(transmission_text, 3);
		sprite_copy_2_to_1_2();
		mouse_draw_opaque_check();
		car_menu_draw_standard_button(transmission_text, 3);
		mouse_draw_transparent_check();
		goto car_menu_top;
	} else if (selected == 4) {
		*material = (legacy_s8)((legacy_u8)*material + 1U);
		render_phase = 3;
		goto car_menu_top;
	} else {
		goto car_menu_top;
	}

	sprite_free_wnd(wndsprite);
	unload_resource(car_resource);
	shape3d_free_car_shapes();
	if (opponent_type != 0 && video_flag5_is0 != 0)
		sprite_free_wnd(opponent_sprite);
	if (opponent_type == 0)
		unload_resource(miscptr);
	mmgr_free((legacy_s8 far*)selector_resource);
	mouse_draw_opaque_check();
	for (i = 0; i < 4U; i++)
		car_id[i] = car_ids[car_index][i];
	idle_expired = 0;
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
				LEGACY_S16_WRAP_MUL(frame_shape->s2d_width,
					video_flag1_is1)),
			animation_y,
			LEGACY_S16_WRAP_ADD(animation_y,
				frame_shape->s2d_height));
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
	legacy_u8 far* scores;
	struct SPRITE far* animation_sprite;
	struct SHAPE2D far* frame_shape;
	legacy_s16 button_x1[4];
	legacy_s16 button_x2[4];
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

	wndsprite = sprite_make_wnd(0x140U, 0xC8U, 0x0FU);
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
		average_speed = (legacy_u16)(((legacy_u32)gState_travDist /
			(legacy_u32)duration) >> 8);
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
	track_resource = (legacy_u8 far*)file_load_resource(1, g_path_buf);
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
		scores = (legacy_u8 far*)td11_highscores;
		if (((legacy_u8)byte_43966 & 6U) == 0 &&
			read_highscore_u16(scores + 0x16AU) >
				(legacy_u16)finish_time) {
			score_status = 1;
		}
	}

	animation_frame = 0;
	animation_timer = 0x1E;
	evaluation_screen = 1;

end_hiscore_start:
	if (opponent_active != 0 && score_status == 2) {
		score_status = 0;
		sprite_copy_wnd_to_1();
		highscore_text_unk();
		selected = 1;
		evaluation_screen = 1;
		goto end_hiscore_menu_draw;
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
		goto end_hiscore_menu_draw;
	}

	aOp01[3] = '1';
	frame_shape = (struct SHAPE2D far*)locate_shape_fatal(
		animation_resource, aOp01);
	animation_width = LEGACY_S16_WRAP_MUL(frame_shape->s2d_width,
		video_flag1_is1);
	animation_x = LEGACY_S16_WRAP_SUB(0x138, animation_width);
	animation_y = LEGACY_S16_WRAP_SUB(0x63, frame_shape->s2d_height);
	animation_y = LEGACY_S16_FROM_BITS(
		((legacy_u16)animation_y >> 1) |
		((legacy_u16)animation_y & 0x8000U));
	draw_lines_unk(LEGACY_S16_WRAP_SUB(animation_x, 3),
		LEGACY_S16_WRAP_SUB(animation_y, 3),
		LEGACY_S16_WRAP_ADD(animation_width, 5),
		LEGACY_S16_WRAP_ADD(frame_shape->s2d_height, 5),
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
		goto end_hiscore_menu_draw;

	score_status = 0;
	evaluation_screen = 1;
	draw_button(locate_text_res(misc_resource, aBct),
		0x81, 0xAF, 0x46, 0x15,
		word_407F4, word_407F6, word_407F8, 0);
	(void)sprite_blit_to_video(wndsprite,
		LEGACY_S8_FROM_BITS(blit_mode));
	blit_mode = 0xFEU;
	sub_29772();
	check_input();
	sprite_copy_2_to_1_2();
	text_resource_count = outcome == 2 ? 1U : 3U;
	for (;;) {
		delta = (legacy_s16)mouse_timer_sprite_unk(4,
			word_3BCEC, word_3BCF6,
			hiscore_buttons_y1, hiscore_buttons_y2,
			word_407CE, word_407D0);
		animation_timer = LEGACY_S16_WRAP_ADD(animation_timer, delta);
		if (animation_timer >= 0x1E) {
			animation_timer = LEGACY_S16_WRAP_SUB(animation_timer, 0x1E);
			animation_frame++;
			if (animation_sequence[animation_frame] == 0)
				animation_frame = 0;
		}
		if (previous_animation_frame != animation_frame) {
			previous_animation_frame = animation_frame;
			end_hiscore_draw_animation_frame(animation_resource,
				animation_sequence, animation_frame,
				animation_x, animation_y, animation_sprite, 0);
		}
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

end_hiscore_menu_draw:
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
		button_x1[i] = LEGACY_S16_WRAP_ADD(word_3BCEC[i], menu_offset);
		button_x2[i] = LEGACY_S16_WRAP_ADD(word_3BCF6[i], menu_offset);
	}
	check_input();
	(void)sprite_blit_to_video(wndsprite,
		LEGACY_S8_FROM_BITS(blit_mode));
	blit_mode = 0xFEU;
	sprite_copy_2_to_1_2();

end_hiscore_menu_loop:
	if (previous_selection != selected) {
		previous_selection = selected;
		sprite_copy_2_to_1_2();
		sprite_set_1_size(0, 0x140,
			hiscore_buttons_y1[0],
			LEGACY_S16_WRAP_ADD(hiscore_buttons_y2[0], 1));
		mouse_draw_opaque_check();
		sprite_putimage(wndsprite->sprite_bitmapptr);
		mouse_draw_transparent_check();
		(void)timer_get_delta_alt();
		sub_29772();
	}

	delta = (legacy_s16)mouse_timer_sprite_unk(selected,
		button_x1, button_x2, hiscore_buttons_y1, hiscore_buttons_y2,
		word_407CE, word_407D0);
	if (evaluation_screen == 0 && outcome != 2) {
		animation_timer = LEGACY_S16_WRAP_ADD(animation_timer, delta);
		if (animation_timer >= 0x1E) {
			animation_timer = LEGACY_S16_WRAP_SUB(animation_timer, 0x1E);
			animation_frame++;
			if (animation_sequence[animation_frame] == 0)
				animation_frame = 0;
		}
		if (previous_animation_frame != animation_frame) {
			previous_animation_frame = animation_frame;
			end_hiscore_draw_animation_frame(animation_resource,
				animation_sequence, animation_frame,
				animation_x, animation_y, animation_sprite, 1);
		}
	}

	if (opponent_active == 0 || score_status == -1) {
		hit = (legacy_s16)mouse_multi_hittest(3,
			&button_x1[1], &button_x2[1],
			&hiscore_buttons_y1[1], &hiscore_buttons_y2[1]);
		if (hit != -1)
			selected = (legacy_u8)(hit + 1);
	} else {
		hit = (legacy_s16)mouse_multi_hittest(4,
			button_x1, button_x2,
			hiscore_buttons_y1, hiscore_buttons_y2);
		if (hit != -1)
			selected = (legacy_u8)hit;
	}

	input = (legacy_u16)input_checking(delta);
	if (input == 0)
		goto end_hiscore_menu_loop;
	if (input == 0x4B00U) {
		if (opponent_active == 0 || score_status == -1) {
			selected = selected <= 1 ? 3U :
				(legacy_u8)(selected - 1U);
		} else {
			selected = selected == 0 ? 3U :
				(legacy_u8)(selected - 1U);
		}
		goto end_hiscore_menu_loop;
	}
	if (input == 0x4D00U) {
		if (selected < 3U)
			selected++;
		else
			selected = (opponent_active == 0 ||
				score_status == -1) ? 1U : 0U;
		goto end_hiscore_menu_loop;
	}
	if (input != 0x0DU && input != 0x20U)
		goto end_hiscore_menu_loop;

	if (selected == 0) {
		sprite_copy_wnd_to_1();
		draw_button(0, 0, 0, 0x140, 0x64,
			word_407F4, word_407F6, word_407F8, 0);
		score_status = evaluation_screen != 0 ? 0 : 2;
		goto end_hiscore_start;
	}

	audio_unload();
	if (opponent_active != 0)
		mmgr_release(animation_resource);
	if (video_flag5_is0 != 0)
		sprite_free_wnd(animation_sprite);
	sprite_free_wnd(wndsprite);
	if (gameconfig.game_opponenttype != 0)
		unload_resource(opponent_resource);
	unload_resource(misc_resource);
	return (legacy_u16)(selected - 1U);
}

extern legacy_u8 byte_3E9DB;
extern legacy_u8 byte_3E9DC[10];
extern legacy_u8 byte_3E9E6[10];
extern legacy_u8 byte_3E9F0[10];
extern legacy_u8 byte_3E9FA[10];
extern legacy_u8 game_camera_buttons_count[4];
extern legacy_s16 game_camera_buttons_x1[9];
extern legacy_s16 game_camera_buttons_x2[9];
extern legacy_s16 game_camera_buttons_y1[9];
extern legacy_s16 game_camera_buttons_y2[9];
extern legacy_s16 word_3EA18;
extern legacy_s16 word_3EA2A;
extern legacy_s16 word_3EA3A;
extern legacy_s16 word_3EA3C;
extern legacy_s16 word_3EA4C;
extern legacy_s16 word_3EA4E;
extern legacy_s16 gameunk_button_x1;
extern legacy_s16 gameunk_button_x2;
extern legacy_s16 gameunk_button_y1;
extern legacy_s16 gameunk_button_y2;
extern legacy_s16 custom_camera_distance;
extern legacy_s16 custom_camera_azimuth_angle;
extern legacy_s16 custom_camera_elevation_angle;
extern legacy_s16 word_40E04[2];
extern legacy_u8 byte_40E08[2];
extern legacy_s16 word_40E0A[2];
extern struct SHAPE2D far* rplyshapes[23];
extern legacy_u8 byte_40E6A[9];
extern legacy_u8 byte_40E6C;
extern legacy_u8 byte_40E6D;
extern legacy_u8 byte_40E74[2];
extern legacy_s16 word_40E76[2];
extern legacy_u8 byte_40E7A[18];
extern struct RECTANGLE* rectptr_unk2;
extern legacy_s16 word_407FC;
extern legacy_s16 word_407FE;
extern legacy_s8 aRplyrpicrpacrpmcrptcbof6bof5b[];
extern legacy_s8 aMen_0[];
extern legacy_s8 aCon_0[];
extern legacy_s8 aRep_1[];
extern legacy_s8 a_rpl_2[];
extern legacy_s8 aFex_0[];
extern legacy_s8 aSer_0[];
extern legacy_s8 aMdo[];

static void replay_controls_select(legacy_u8 selection)
{
	legacy_u16 index;

	for (index = 0; index < 9U; index++)
		byte_40E6A[index] = 0;
	byte_40E6A[selection] = 1;
}

static void replay_controls_draw(legacy_s16 recorded_frame, legacy_s16 current_frame)
{
	legacy_u16 player_index;
	legacy_u16 index;
	legacy_s16 recorded_position;
	legacy_s16 current_position;
	legacy_u16 displayed_time;
	legacy_u8 previous_selection;
	legacy_u8 state_changed;

	player_index = (legacy_u8)byte_4432A;
	if (byte_449D8[player_index] == 0) {
		byte_449D8[player_index] = 1;
		byte_40E74[player_index] = 0xFFU;
		byte_40E08[player_index] = 0xFFU;
		for (index = 0; index < 9U; index++)
			byte_40E7A[player_index + index * 2U] = 0;
		mouse_draw_opaque_check();
		shape2d_op_unk(rplyshapes[0]);
		word_40E0A[player_index] = -1;
		word_40E76[player_index] = -1;
		format_frame_as_string(&resID_byte1,
			(legacy_u16)(gameconfig.game_recordedframes + elapsed_time1),
			1);
		font_set_unk(dialog_fnt_colour, 0);
		font_set_fontdef2(fontledresptr);
		sub_345BC(&resID_byte1, 0xD8, 0xBB);
		font_set_fontdef();
	}

	displayed_time = (legacy_u16)(current_frame + elapsed_time1);
	if ((legacy_u16)word_40E0A[player_index] != displayed_time) {
		word_40E0A[player_index] = (legacy_s16)displayed_time;
		format_frame_as_string(&resID_byte1, displayed_time, 1);
		font_set_unk(dialog_fnt_colour, 0);
		mouse_draw_opaque_check();
		font_set_fontdef2(fontledresptr);
		sub_345BC(&resID_byte1, 0x98, 0xBB);
		font_set_fontdef();
	}

	if (byte_40E74[player_index] != (legacy_u8)cameramode) {
		byte_40E74[player_index] = (legacy_u8)cameramode;
		word_40E76[player_index] = -1;
		mouse_draw_opaque_check();
		shape2d_op_unk(rplyshapes[1U + (legacy_u8)cameramode]);
		if (LEGACY_S8_FROM_BITS(byte_3E9DB) > LEGACY_S8_FROM_BITS(
			game_camera_buttons_count[(legacy_u8)cameramode]))
			byte_3E9DB = game_camera_buttons_count[(legacy_u8)cameramode];
		if (byte_40E08[player_index] > 6U)
			byte_40E08[player_index] = 0xFFU;
	}

	if (gameconfig.game_recordedframes == 0) {
		recorded_position = 0;
		current_position = 0;
	} else {
		recorded_position = (legacy_s16)(((legacy_s32)(legacy_s16)
			recorded_frame * 110L) /
			(legacy_s32)(legacy_s16)gameconfig.game_recordedframes);
		current_position = (legacy_s16)(((legacy_s32)(legacy_s16)
			current_frame * 110L) /
			(legacy_s32)(legacy_s16)gameconfig.game_recordedframes);
	}
	if (word_40E76[player_index] != recorded_position ||
		word_40E04[player_index] != current_position) {
		mouse_draw_opaque_check();
		word_40E76[player_index] = recorded_position;
		word_40E04[player_index] = current_position;
		sprite_1_unk(0x9A, 0xB1, 0x74, 6, word_407FC);
		sprite_1_unk(LEGACY_S16_WRAP_ADD(0x9A, recorded_position),
			0xB1, 6, 6, dialog_fnt_colour);
		sprite_1_unk4(LEGACY_S16_WRAP_ADD(0x9A, current_position),
			0xB1, LEGACY_S16_WRAP_ADD(0x9F, current_position),
			0xB6, word_407FE);
	}

	state_changed = byte_40E08[player_index] != byte_3E9DB;
	if (state_changed == 0) {
		for (index = 0; index < 7U; index++) {
			if (byte_40E7A[player_index + index * 2U] !=
				byte_40E6A[index]) {
				state_changed = 1;
				break;
			}
		}
	}
	if (state_changed == 0) {
		mouse_draw_transparent_check();
		return;
	}

	mouse_draw_opaque_check();
	previous_selection = byte_40E08[player_index];
	if (previous_selection != 0xFFU) {
		if (byte_40E7A[player_index + previous_selection * 2U] != 0)
			shape2d_op_unk(rplyshapes[14U + previous_selection]);
		else
			shape2d_op_unk(rplyshapes[5U + previous_selection]);
		byte_40E08[player_index] = 0xFFU;
	}
	for (index = 0; index < 7U; index++) {
		if (byte_40E6A[index] == 0 &&
			byte_40E7A[player_index + index * 2U] != 0) {
			shape2d_op_unk(rplyshapes[5U + index]);
			byte_40E7A[player_index + index * 2U] = 0;
		}
	}
	for (index = 0; index < 7U; index++) {
		if (byte_40E6A[index] != 0) {
			byte_40E7A[player_index + index * 2U] = 1;
			shape2d_op_unk(rplyshapes[14U + index]);
			byte_40E7A[player_index + index * 2U] = 1;
		}
	}
	byte_40E08[player_index] = byte_3E9DB;
	if (byte_3E9DB != 0xFFU) {
		sprite_1_unk4(game_camera_buttons_x1[byte_3E9DB],
			game_camera_buttons_y1[byte_3E9DB],
			game_camera_buttons_x2[byte_3E9DB],
			game_camera_buttons_y2[byte_3E9DB], word_407FE);
	}
	mouse_draw_transparent_check();
}

static void replay_draw_waiting(void)
{
	struct RECTANGLE* text_rectangle;

	copy_string(&resID_byte1, locate_text_res(gameresptr, "wai"));
	text_rectangle = intro_draw_text(&resID_byte1,
		font_op2_alt(&resID_byte1), 0x64, dialog_fnt_colour, 0);
	if (slow_video_mgmt_copy != 0)
		rect_union(rectptr_unk2, text_rectangle, rectptr_unk2);
}

static void replay_pause_menu(void)
{
	struct GAMEINFO saved_config;
	legacy_s16 options[8];
	legacy_s16 mode_options[5];
	legacy_s16 dialog_result;
	legacy_s8 menu_result;
	legacy_s8 save_status;
	legacy_u16 index;
	legacy_u8 saved_track;
	legacy_s16 resources_changed;
	legacy_s16 opponent_changed;

	is_in_replay = 1;
	audio_carstate();
	replay_controls_select(4);
	replay_controls_draw(state.game_frame, state.game_frame);
	for (index = 0; index < 8U; index++)
		options[index] = 0;
	if (state.playerstate.car_crashBmpFlag != 0)
		options[3] = 1;
	if (gameconfig.game_recordedframes == 0 || elapsed_time1 != 0)
		options[5] = 1;
	if (passed_security == 0) {
		options[2] = 1;
		options[3] = 1;
	}
	if (((legacy_u8)byte_43966 & 4U) == 0)
		options[1] = 1;
	byte_454A4 = (legacy_u8)video_flag6_is1;
	menu_result = LEGACY_S8_FROM_BITS(show_dialog(2, 0,
		locate_text_res(gameresptr, aMen_0), 0xFFFFU, 0xFFFFU,
		dialogarg2, options, 0));

	switch (menu_result) {
	case 1:
		update_crash_state(4, 0);
		byte_449DA = 2;
		break;

	case 2:
		check_input();
		framespersec = framespersec2;
		*(legacy_u8*)&gameconfig.game_framespersec =
			(legacy_u8)framespersec2;
		init_game_state(-1);
		elapsed_time2 = 0;
		gameconfig.game_recordedframes = 0;
		*(legacy_u8*)&word_45D3E = 0;
		byte_43966 = 1;
		goto replay_pause_resume_driving;

	case 3:
		if (((legacy_u8)byte_43966 & 2U) != 0) {
			byte_43966 = 3;
		} else if (gameconfig.game_recordedframes != elapsed_time2) {
			dialog_result = LEGACY_S16_FROM_BITS(show_dialog(2, 0,
				locate_text_res(gameresptr, aCon_0), 0xFFFFU,
				0xFFFFU, performGraphColor, 0, 0));
			if (dialog_result < 1)
				break;
			byte_43966 = 3;
		} else {
			byte_43966 = 1;
		}
		elapsed_time2 = (legacy_u16)state.game_frame;
		gameconfig.game_recordedframes = (legacy_u16)state.game_frame;

replay_pause_resume_driving:
		dashb_toggle = 1;
		show_penalty_counter = 0;
		followOpponentFlag = 0;
		game_replay_mode = 0;
		cameramode = 0;
		state.game_3F6autoLoadEvalFlag = 0;
		state.game_frame_in_sec = 0;
		byte_449E6 = 0;
		replay_controls_select(3);
		is_in_replay = 0;
		mouse_minmax_position(LEGACY_S8_FROM_BITS(byte_3B8F2));
		check_input();
		kbormouse = 0;
		break;

	case 4:
		byte_43966 = 0;
		audio_carstate();
		if (do_fileselect_dialog(byte_3B85E, aDefault_1, ".rpl",
			locate_text_res(mainresptr, "rep")) == 0)
			break;
		waitflag = 0x96;
		show_waiting();
		saved_config = gameconfig;
		saved_track = td14_elem_map_main[0x384];
		if ((legacy_u8)file_load_replay(byte_3B85E, aDefault_1) != 0)
			gameconfig.game_recordedframes = 0;
		dashb_toggle = 0;
		track_setup();
		resources_changed = td14_elem_map_main[0x384] != saved_track;
		for (index = 0; index < 4U; index++) {
			if (saved_config.game_playercarid[index] !=
				gameconfig.game_playercarid[index])
				resources_changed = 1;
		}
		if (saved_config.game_opponenttype !=
			gameconfig.game_opponenttype) {
			resources_changed = 1;
		} else if (gameconfig.game_opponenttype != 0) {
			opponent_changed = 0;
			for (index = 0; index < 4U; index++) {
				if (saved_config.game_opponentcarid[index] !=
					gameconfig.game_opponentcarid[index]) {
					resources_changed = 1;
					opponent_changed = 1;
				}
			}
			if (opponent_changed == 0) {
				ensure_file_exists(2);
				load_opponent_data();
			}
		}
		if (resources_changed != 0) {
			free_player_cars();
			setup_player_cars();
		}
		framespersec = (legacy_s16)LEGACY_S8_FROM_BITS(
			*(legacy_u8*)&gameconfig.game_framespersec);
		init_game_state(-1);
		break;

	case 5:
		audio_carstate();
		for (;;) {
			save_status = 0;
			if (do_savefile_dialog(byte_3B85E, aDefault_1,
				locate_text_res(mainresptr, aRep_1)) == 0) {
				save_status = -1;
			} else {
				file_build_path(byte_3B85E, aDefault_1, a_rpl_2,
					g_path_buf);
				save_status = 1;
				g_is_busy = 1;
				if (file_find(g_path_buf) != 0) {
					dialog_result = LEGACY_S16_FROM_BITS(show_dialog(
						2, 0, locate_text_res(mainresptr, aFex_0),
						0xFFFFU, 0xFFFFU, performGraphColor, 0, 0));
					if (dialog_result == -1)
						save_status = -1;
					else if (dialog_result == 0)
						save_status = 0;
				}
				g_is_busy = 0;
			}
			if (save_status != 1)
				break;
			if ((legacy_u8)file_write_replay(g_path_buf) == 0)
				break;
			show_dialog(1, 0, locate_text_res(mainresptr, aSer_0),
				0xFFFFU, 0xFFFFU, performGraphColor, 0, 0);
		}
		break;

	case 6:
		for (index = 0; index < 5U; index++)
			mode_options[index] = 0;
		if (gameconfig.game_opponenttype == 0)
			mode_options[4] = 1;
		menu_result = LEGACY_S8_FROM_BITS(show_dialog(2, 0,
			locate_text_res(gameresptr, aMdo), 0xFFFFU, 0xFFFFU,
			dialogarg2, mode_options, 0));
		switch (menu_result) {
		case 0:
			dashb_toggle ^= 1;
			break;
		case 1:
			replaybar_toggle ^= 1;
			break;
		case 2:
			cameramode = (legacy_s8)(((legacy_u8)cameramode + 1U) & 3U);
			break;
		case 3:
			show_graphic_levels_menu();
			break;
		case 4:
			followOpponentFlag ^= 1;
			break;
		}
		break;

	case 7:
		update_crash_state(4, 0);
		byte_43966 = 0;
		byte_449DA = 2;
		break;
	}
	check_input();
}

static legacy_s32 replay_scrub_accumulate(legacy_s32 accumulated,
	legacy_s16 speed, legacy_u16 delta)
{
	legacy_s16 increment;

	increment = LEGACY_S16_WRAP_MUL(LEGACY_S16_FROM_BITS(delta), speed);
	return LEGACY_S32_WRAP_ADD_S16(accumulated, increment);
}

static void replay_fast_forward(void)
{
	legacy_s32 accumulated;
	legacy_s16 speed;
	legacy_u16 delta;
	legacy_u16 remaining;
	legacy_u16 amount;
	legacy_u16 target;

	is_in_replay = 1;
	audio_carstate();
	replay_controls_select(0);
	(void)timer_get_delta_alt();
	accumulated = 20L;
	while (((legacy_u8)input_combined_flags & 0x30U) != 0) {
		speed = (legacy_s16)(accumulated / 50L + 3L);
		if (speed > 100)
			speed = 100;
		delta = (legacy_u16)timer_get_delta_alt();
		accumulated = replay_scrub_accumulate(accumulated, speed, delta);
		remaining = LEGACY_U16_WRAP_SUB(
			gameconfig.game_recordedframes, elapsed_time2);
		amount = (legacy_u16)(accumulated / 20L);
		if (amount > remaining)
			accumulated = (legacy_s32)remaining * 20L;
		amount = (legacy_u16)(accumulated / 20L);
		replay_controls_draw(state.game_frame,
			LEGACY_U16_WRAP_ADD(elapsed_time2, amount));
		input_do_checking(LEGACY_S16_FROM_BITS(delta));
	}

	remaining = LEGACY_U16_WRAP_SUB(
		gameconfig.game_recordedframes, elapsed_time2);
	amount = (legacy_u16)(accumulated / 20L);
	if (amount > remaining) {
		accumulated = (legacy_s32)remaining * 20L;
		amount = remaining;
	}
	target = LEGACY_U16_WRAP_ADD(elapsed_time2, amount);
	if (LEGACY_S16_FROM_BITS(target) >
		LEGACY_S16_FROM_BITS(gameconfig.game_recordedframes))
		target = gameconfig.game_recordedframes;
	restore_gamestate(target);
	elapsed_time2 = target;
	replay_controls_select(4);
	replay_draw_waiting();
	while ((legacy_u16)state.game_frame != elapsed_time2) {
		update_gamestate();
		replay_controls_draw(state.game_frame, elapsed_time2);
	}
	input_do_checking(1000);
}

static void replay_rewind(void)
{
	legacy_s32 accumulated;
	legacy_s32 interpolation;
	legacy_s16 speed;
	legacy_s16 frames_to_catch_up;
	legacy_s16 frames_remaining;
	legacy_u16 delta;
	legacy_u16 amount;
	legacy_u16 target;
	legacy_u16 displayed_frame;

	is_in_replay = 1;
	audio_carstate();
	replay_controls_select(1);
	(void)timer_get_delta_alt();
	accumulated = 20L;
	while (((legacy_u8)input_combined_flags & 0x30U) != 0) {
		speed = (legacy_s16)(accumulated / 50L + 3L);
		if (speed > 100)
			speed = 100;
		delta = (legacy_u16)timer_get_delta_alt();
		accumulated = replay_scrub_accumulate(accumulated, speed, delta);
		amount = (legacy_u16)(accumulated / 20L);
		if (amount > elapsed_time2)
			accumulated = (legacy_s32)elapsed_time2 * 20L;
		amount = (legacy_u16)(accumulated / 20L);
		replay_controls_draw(state.game_frame,
			LEGACY_U16_WRAP_SUB(elapsed_time2, amount));
		input_do_checking(LEGACY_S16_FROM_BITS(delta));
	}

	amount = (legacy_u16)(accumulated / 20L);
	if (amount > elapsed_time2)
		amount = elapsed_time2;
	replay_controls_select(4);
	if (amount != 0) {
		replay_draw_waiting();
		target = LEGACY_U16_WRAP_SUB(elapsed_time2, amount);
		restore_gamestate(target);
		elapsed_time2 = target;
		frames_to_catch_up = LEGACY_S16_WRAP_SUB(
			LEGACY_S16_FROM_BITS(target), state.game_frame);
		frames_remaining = frames_to_catch_up;
		while ((legacy_u16)state.game_frame != elapsed_time2) {
			update_gamestate();
			frames_remaining = LEGACY_S16_WRAP_SUB(frames_remaining, 1);
			interpolation = ((legacy_s32)frames_to_catch_up *
				(legacy_s32)frames_remaining) / (legacy_s32)amount;
			displayed_frame = LEGACY_U16_WRAP_ADD(elapsed_time2,
				(legacy_u16)interpolation);
			replay_controls_draw(displayed_frame, elapsed_time2);
			input_do_checking(1);
		}
	}
	replay_controls_draw(state.game_frame, state.game_frame);
	input_do_checking(1000);
}

void loop_game(legacy_s16 operation, legacy_s16 recorded_frame, legacy_s16 current_frame)
{
	legacy_u16 input;
	legacy_s16 delta;
	legacy_s16 midpoint;
	legacy_s16 x_delta;
	legacy_s16 y_delta;
	legacy_u16 angle;
	legacy_u8 hit;
	legacy_u8 next_selection;
	legacy_u8 custom_camera;

	if (operation == 0) {
		locate_many_resources((legacy_s8 far*)sdgameresptr,
			aRplyrpicrpacrpmcrptcbof6bof5b,
			(legacy_s8 far**)rplyshapes);
		replay_controls_select(4);
		return;
	}
	if (operation == 1) {
		replay_controls_draw(recorded_frame, current_frame);
		return;
	}
	if (operation == 2) {
		replay_controls_select((legacy_u8)recorded_frame);
		return;
	}
	if (operation != 3)
		return;

	if (LEGACY_S8_FROM_BITS(byte_3E9DB) > LEGACY_S8_FROM_BITS(
		game_camera_buttons_count[(legacy_u8)cameramode]) &&
		cameramode != 2)
		byte_3E9DB = game_camera_buttons_count[(legacy_u8)cameramode];
	sprite_copy_2_to_1();
	if (video_flag5_is0 != 0)
		byte_4432A = byte_44346 ^ 1;

replay_input_loop:
	delta = LEGACY_S16_FROM_BITS((legacy_u16)timer_get_delta_alt());
	input = (legacy_u16)input_checking(delta);
	hit = (legacy_u8)mouse_multi_hittest(
		(legacy_u8)(game_camera_buttons_count[(legacy_u8)cameramode] + 1U),
		game_camera_buttons_x1, game_camera_buttons_x2,
		game_camera_buttons_y1, game_camera_buttons_y2);
	if (hit != 0xFFU) {
		if (hit != byte_3E9DB && input == 0)
			input = 1;
		byte_3E9DB = hit;
		if ((input == 0x0DU || input == 0x20U) && byte_3E9DB >= 7U) {
			if (byte_3E9DB == 7U) {
				midpoint = (legacy_s16)((word_3EA3A + word_3EA4C) >> 1);
				input = midpoint < mouse_ypos ? 0x5000U : 0x4800U;
			} else {
				y_delta = LEGACY_S16_WRAP_SUB(
					(legacy_s16)((word_3EA3C + word_3EA4E) >> 1),
					(legacy_s16)mouse_ypos);
				x_delta = LEGACY_S16_WRAP_SUB((legacy_s16)mouse_xpos,
					(legacy_s16)((word_3EA18 + word_3EA2A) >> 1));
				angle = (legacy_u16)polarAngle(x_delta, y_delta);
				switch (((angle + 0x80U) >> 8) & 3U) {
				case 0:
					input = 0x4800U;
					break;
				case 1:
					input = 0x4D00U;
					break;
				case 2:
					input = 0x5000U;
					break;
				default:
					input = 0x4B00U;
					break;
				}
			}
		}
	} else {
		hit = (legacy_u8)mouse_multi_hittest(1,
			&gameunk_button_x1, &gameunk_button_x2,
			&gameunk_button_y1, &gameunk_button_y2);
		if (hit == 0 && (input == 0x0DU || input == 0x20U))
			input = 'c';
	}

	if (input != 0 && input != 0x1BU &&
		(legacy_u8)handle_ingame_kb_shortcuts(input) != 0)
		return;
	if (is_in_replay == 0 && input == 0) {
		if (replaybar_enabled != 0)
			replay_controls_draw(state.game_frame, state.game_frame);
		return;
	}
	if (replaybar_enabled == 0) {
		is_in_replay_copy = (legacy_s8)0xFF;
		word_449EA = -1;
	}
	if (is_in_replay != 0 && (byte_40E6D != 0 || byte_40E6C != 0))
		replay_controls_select(4);
	replay_controls_draw(state.game_frame, state.game_frame);

	custom_camera = 0;
	if (kb_get_key_state(0x1D) != 0 ||
		(byte_3E9DB == 8U && ((legacy_u8)input_combined_flags & 0x30U) != 0))
		custom_camera = 1;
	if (custom_camera != 0) {
		switch (input) {
		case 0x4D00U:
			custom_camera_azimuth_angle = LEGACY_S16_WRAP_ADD(
				custom_camera_azimuth_angle, 0x10);
			return;
		case 0x4B00U:
			custom_camera_azimuth_angle = LEGACY_S16_WRAP_SUB(
				custom_camera_azimuth_angle, 0x10);
			return;
		case 0x4800U:
			if (LEGACY_S16_WRAP_ADD(custom_camera_elevation_angle,
				0x10) < 0x100) {
				custom_camera_elevation_angle = LEGACY_S16_WRAP_ADD(
					custom_camera_elevation_angle, 0x10);
				return;
			}
			input = 0;
			break;
		case 0x5000U:
			if (LEGACY_S16_WRAP_SUB(custom_camera_elevation_angle,
				0x10) > -0x100) {
				custom_camera_elevation_angle = LEGACY_S16_WRAP_SUB(
					custom_camera_elevation_angle, 0x10);
				return;
			}
			input = 0;
			break;
		case '+':
		case '-':
			break;
		default:
			input = 0;
			break;
		}
	}

	if (input == '-' || input == '+') {
replay_zoom:
		if (input == '-') {
			if (cameramode == 3) {
				if (word_44D20 <= 0)
					goto replay_redraw;
				word_44D20 = LEGACY_S16_WRAP_SUB(word_44D20, 0x1E);
			} else {
				if (custom_camera_distance >= 0x5DC)
					goto replay_redraw;
				custom_camera_distance = LEGACY_S16_WRAP_ADD(
					custom_camera_distance, 0x1E);
			}
		} else {
			if (cameramode == 3) {
				if (word_44D20 >= 0x384)
					goto replay_redraw;
				word_44D20 = LEGACY_S16_WRAP_ADD(word_44D20, 0x1E);
			} else {
				if (custom_camera_distance <= 0x78)
					goto replay_redraw;
				custom_camera_distance = LEGACY_S16_WRAP_SUB(
					custom_camera_distance, 0x1E);
			}
		}
		return;
	}

	switch (input) {
	case 0x0DU:
	case 0x20U:
		if (byte_3E9DB > 6U)
			goto replay_redraw;
		switch (byte_3E9DB) {
		case 0:
			replay_fast_forward();
			return;
		case 1:
			replay_rewind();
			return;
		case 2:
			replay_controls_select(2);
			byte_449E6 = 3;
			is_in_replay = 0;
			goto replay_redraw;
		case 3:
			byte_449E6 = 0;
			replay_controls_select(3);
			is_in_replay = 0;
			goto replay_redraw;
		case 4:
			is_in_replay = 1;
			audio_carstate();
			replay_controls_select(4);
			replay_controls_draw(state.game_frame, state.game_frame);
			goto replay_redraw;
		case 5:
			is_in_replay = 1;
			audio_carstate();
			replay_controls_select(5);
			replay_controls_draw(state.game_frame, state.game_frame);
			restore_gamestate(0);
			(void)timer_get_counter_unk(50UL);
			replay_controls_select(4);
			replay_controls_draw(state.game_frame, state.game_frame);
			return;
		case 6:
			replay_pause_menu();
			return;
		}
		break;

	case 0x1BU:
		replay_pause_menu();
		return;

	case 0x4B00U:
		next_selection = byte_3E9DC[byte_3E9DB];
		if (next_selection <=
			game_camera_buttons_count[(legacy_u8)cameramode])
			byte_3E9DB = next_selection;
		goto replay_redraw;

	case 0x4D00U:
		byte_3E9DB = byte_3E9E6[byte_3E9DB];
		goto replay_redraw;

	case 0x4800U:
		if (byte_3E9DB == 7U) {
			input = '+';
			goto replay_zoom;
		}
		byte_3E9DB = byte_3E9F0[byte_3E9DB];
		goto replay_redraw;

	case 0x5000U:
		if (byte_3E9DB == 7U) {
			input = '-';
			goto replay_zoom;
		}
		byte_3E9DB = byte_3E9FA[byte_3E9DB];
		goto replay_redraw;
	}

replay_redraw:
	replay_controls_draw(state.game_frame, state.game_frame);
	goto replay_input_loop;
}

void run_game(void) {
	legacy_s16 var_16[2];
	legacy_s16 var_12, var_E, var_C;
	struct RECTANGLE var_rect;
	legacy_s16 var_2;
	legacy_s16 regsi;

	var_C = -1;
	rect_windshield.left = 0;
	rect_windshield.right = 320;
	var_2 = -1;
	word_449EA = -1;
	run_game_random = get_kevinrandom() << 3;
	replaybar_toggle = 1;
	is_in_replay = 0;
	if (idle_expired == 0) {
		if (gameconfig.game_recordedframes != 0) {
			cameramode = 0;
			game_replay_mode = 2;
			is_in_replay = 1;
		} else {
			cameramode = 0;
			game_replay_mode = 1;
		}
	} else {
		cameramode++;
		if (cameramode == 4) {
			cameramode = 0;
		}

		game_replay_mode = 2;		
		if (file_load_replay(0, "default") != 0) {
			return ;
		}
		track_setup();
	}

	if (setup_player_cars() != 0) {
		free_player_cars();
		do_mer_restext();
	} else {

		kbormouse = 0;
		byte_449E6 = 0;
		byte_449DA = 1;
		set_frame_callback();
		game_replay_mode_copy = -1;
		byte_44346 = 0;
		byte_4432A = 0;
		byte_46467 = 0;
		dashb_toggle = 0;

		if (idle_expired != 0) {
			framespersec = gameconfig.game_framespersec;

			init_game_state(-1);
		} else {
			if (is_in_replay == 0) {
				cameramode = 0;
				dashb_toggle = 1;
				show_penalty_counter = 0;
				framespersec = framespersec2;
				gameconfig.game_framespersec = framespersec2;
				init_game_state(-1);
				word_45D94 = 0;
				*(legacy_s8*)&word_45D3E = 0; // byte ptr!
				byte_4393C = 1;
				mouse_minmax_position(byte_3B8F2);
				game_replay_mode = 1;
				
				state.playerstate.car_posWorld1.lx += multiply_and_scale(sin_fast(track_angle), -240) << 6;
				state.playerstate.car_posWorld1.lz += multiply_and_scale(cos_fast(track_angle), -240) << 6;
				state.playerstate.car_posWorld1.ly += 0x580;
				byte_43966 = 1;
			} else {
				cameramode = 0;
				game_replay_mode = 2;
				word_44DCA = 0x1F4;
				framespersec = gameconfig.game_framespersec;
				restore_gamestate(0);
				restore_gamestate(gameconfig.game_recordedframes);

				while (gameconfig.game_recordedframes != state.game_frame) {
					if (input_do_checking(1) == 27)
						break;
					update_gamestate();
				}

				elapsed_time2 = gameconfig.game_recordedframes;
			}
		}

		while (1) {

			if (state.game_frame != elapsed_time2) {
				if ((byte_3B8F2 != 0 || dos_joystick_enabled != 0) && game_replay_mode == 0) {
					replay_unk();
				}
				update_gamestate();
				continue;
			}
			
			
			if (game_replay_mode == 0 && byte_449DA == 0 && state.game_inputmode != 0) {
				if (var_C == state.game_frame)
					continue;
				var_C = state.game_frame;
			}

			if (state.game_inputmode == 0 && game_replay_mode == 0) {
				elapsed_time2 = 0;
				gameconfig.game_recordedframes = 0;
				state.game_frame = 0;
			}

			if (slow_video_mgmt_copy != slow_video_mgmt) {
				slow_video_mgmt_copy = slow_video_mgmt;
				init_rect_arrays();
			}

			if (byte_46467 != 0) {
				input_push_status();
				audio_suspend();
				regsi = show_dialog(2, 1, locate_text_res(gameresptr, "rbf"), -1, -1, dialogarg2, 0, 0);
				if (regsi == -1)
					regsi = 0;

				audio_resume();
				dos_timer_callbacks_suspended = 0;
				input_pop_status();
				if (regsi != 0) {
					update_crash_state(4, 0);
					byte_449DA = 1;
				}

				byte_46467 = 0;
			}

			if (video_flag5_is0 != 0) {
				setup_mcgawnd2();
				byte_4432A = byte_44346;
			} else {
				sprite_copy_wnd_to_1();
			}

			if (game_replay_mode != game_replay_mode_copy || dashb_toggle != dashb_toggle_copy || replaybar_toggle != replaybar_toggle_copy || is_in_replay != is_in_replay_copy || followOpponentFlag != followOpponentFlag_copy) {
				game_replay_mode_copy = game_replay_mode;
				dashb_toggle_copy = dashb_toggle;
				replaybar_toggle_copy = replaybar_toggle;
				is_in_replay_copy = is_in_replay;
				followOpponentFlag_copy = followOpponentFlag;
				roofbmpheight_copy = 0;
				byte_449E2 = 0;

				if (game_replay_mode != 2 || idle_expired != 0 || (replaybar_toggle == 0 && is_in_replay == 0)) {
					replaybar_enabled = 0;
				} else {
					replaybar_enabled = 1;
				}

				if (idle_expired != 0) {
					dashbmp_y_copy = 0xC8;
				} else 
				if (dashb_toggle == 0 || followOpponentFlag != 0) {
					if (game_replay_mode == 2) {
						if (replaybar_enabled != 0) {
							dashbmp_y_copy = 0x97;
						} else {
							dashbmp_y_copy = 0xC8;
						}
					} else {
						dashbmp_y_copy = 0xC8;
					}
				} else {
					if (game_replay_mode != 2 || replaybar_enabled == 0) {
						height_above_replaybar = 200;
					} else {
						height_above_replaybar = 151;
					}

					byte_449E2 = 1;
					roofbmpheight_copy = roofbmpheight;
					dashbmp_y_copy = dashbmp_y;
				}

				if (var_2 != roofbmpheight_copy || dashbmp_y_copy != word_449EA || var_E != height_above_replaybar) {
					byte_454A4 = video_flag6_is1;
					set_projection(0x23, dashbmp_y_copy / 6, 0x140, dashbmp_y_copy);
					rect_windshield.top = roofbmpheight_copy;
					rect_windshield.bottom = dashbmp_y_copy;
					var_2 = roofbmpheight_copy;
					word_449EA = dashbmp_y_copy;
					var_E = height_above_replaybar;
				}
			}

			if (byte_454A4 != 0) {
				byte_449D8[byte_4432A] = 0;
				if (byte_449E2 != 0) {
					sprite_set_1_size(0, 0x140, dashbmp_y_copy, height_above_replaybar);
					setup_car_shapes(1);
				}

				if (replaybar_enabled != 0) {
					sprite_set_1_size(0, 0x140, 0, 0xC8);
					loop_game(1, state.game_frame, state.game_frame);
				}
			} else {
				if (replaybar_enabled == 0) {
					byte_449D8[byte_4432A] = 0;
				}
			}

			update_frame(byte_44346, &rect_windshield);
			if (dastbmp_y != 0 && byte_449E2 != 0) {			
				if (slow_video_mgmt_copy != 0) {
					var_rect.left = 0;
					var_rect.right = 0x140;
					var_rect.top = dastbmp_y;
					var_rect.bottom = dashbmp_y_copy;
					if (rectptr_unk != 0) {
						rect_union(rectptr_unk, &var_rect, rectptr_unk);
					}
				}

				shape2d_render_bmp_as_mask(dasmshapeptr);
				shape2d_op_unk4(dastbmp_y2, dastseg);
			}

			sub_19F14(&rect_windshield);
			if (byte_449E2 != 0) {
				sprite_set_1_size(0, 0x140, dashbmp_y_copy, height_above_replaybar);
				setup_car_shapes(2);
				sprite_set_1_size(0, 0x140, 0, 0xC8);
			}

			if (byte_454A4 != 0) {
				byte_454A4--;
			}

			if (video_flag5_is0 != 0) {
				mouse_draw_opaque_check();
				setup_mcgawnd1();
				byte_44346 ^= 1;
				byte_4432A = byte_44346;
				mouse_draw_transparent_check();
			}

			if (game_replay_mode == 1 && byte_4393C == 0) {
				game_replay_mode = 0;
				framespersec = framespersec2;
				gameconfig.game_framespersec = framespersec2;
				init_game_state(-1);
			}

			if (idle_expired == 0) {
				if (byte_449DA != 0) {

					if ((game_replay_mode != 0 || state.game_3F6autoLoadEvalFlag == 4) && byte_449DA != 2) {
						byte_449DA = 0;
						game_replay_mode = 2;
						mouse_minmax_position(0);
						loop_game(0, 0, 0);
						loop_game(2, 4, 0);
						is_in_replay = 1;
						audio_carstate();
					} else {
						break;
					}
				}

				if (game_replay_mode == 2) {
					loop_game(3, 0, 0);
					continue;
				}

				do {
					var_12 = dos_kb_get_char();
					if (var_12 != 0) {
						handle_ingame_kb_shortcuts(var_12);
					}

				} while (var_12 == 0x4800 || var_12 == 0x4B00 || var_12 == 0x4D00 || var_12 == 0x5000);

				if (game_replay_mode == 1) {
					dos_mouse_get_state(&mouse_butstate, &mouse_xpos, &mouse_ypos);
					if (((mouse_butstate & 3) != 0) || ((get_kb_or_joy_flags() & 0x30) != 0)) {
						game_replay_mode = 0;
						byte_4393C = 0;
						framespersec = framespersec2;
						gameconfig.game_framespersec = framespersec2;
						init_game_state(-1);
					}
				}

			} else {
				if (dos_kb_get_char() != 0 || byte_449DA != 0 || get_kb_or_joy_flags() != 0) {
					break;
				}
			}
		}

		if (video_flag5_is0 != 0 && get_0() != 0) {
			mouse_draw_opaque_check();
			setup_mcgawnd2();
			sub_35C4E(0, 0, 0x140, 0xC8, 0);
			setup_mcgawnd1();
			mouse_draw_transparent_check();
		}

		sprite_copy_2_to_1_2();
		is_in_replay = 1;
		audio_carstate();
		audio_remove_driver_timer();
		if (game_replay_mode == 0 && gameconfig.game_opponenttype != 0 && state.opponentstate.car_crashBmpFlag == 0) {
			show_dialog(3, 0, locate_text_res(gameresptr, "cop"), -1, 0x50, performGraphColor, &var_16, 0);
			*(legacy_s8*)&word_45D3E = 1;
			regsi = framespersec;
			regsi--;

			while (1) {
				replay_unk2(1);
				update_gamestate();
				regsi++;
				if (regsi == framespersec) {
					regsi = 0;
					format_frame_as_string(&resID_byte1, state.game_frame + elapsed_time1, 1);
					mouse_draw_opaque_check();
					sub_345BC(&resID_byte1, font_op2_alt(&resID_byte1), var_16[1]);
					mouse_draw_transparent_check();
				}

				if (input_do_checking(1) == 27)
					break;
				if (state.opponentstate.car_crashBmpFlag != 0)
					break;
				if (0x5DC * framespersec == state.game_frame + elapsed_time1)
					break;
			}
		}

		*(legacy_s8*)&word_45D3E = 0; // byte ptr 
		mouse_minmax_position(0);
		remove_frame_callback();
		free_player_cars();
	}

	waitflag = 0x64;
	check_input();
	show_waiting();

	return ;
}
#ifdef RESTUNTS_DOS
extern void _CType _setvect(legacy_s16 interrupt_number,
	void interrupt (far* handler)());
extern void interrupt (far* _CType _getvect(
	legacy_s16 interrupt_number))();
extern void interrupt (far* old_intr0_handler)();
extern legacy_u16 word_3BE30;
extern legacy_u16 word_3BE32;

#pragma argsused
static void interrupt c_intr0_handler(legacy_u16 bp, legacy_u16 di,
	legacy_u16 si, legacy_u16 ds, legacy_u16 es, legacy_u16 dx,
	legacy_u16 cx, legacy_u16 bx, legacy_u16 ax, legacy_u16 ip,
	legacy_u16 cs, legacy_u16 flags)
{
	word_3BE30 = cs;
	word_3BE32 = ip;
	ip = LEGACY_U16_WRAP_ADD(ip, 2U);
	ax = 0;
}
#endif

void init_div0(void)
{
#ifdef RESTUNTS_DOS
	old_intr0_handler = _getvect(0);
	_setvect(0, c_intr0_handler);
#endif
}

void copy_material_list_pointers(void* clrlist, void* clrlist2, void* patlist, void* patlist2, legacy_u16 videoConst)
{
	material_clrlist_ptr_cpy = clrlist;
	material_clrlist2_ptr_cpy = clrlist2;
	material_patlist_ptr_cpy = patlist;
	material_patlist2_ptr_cpy = patlist2;
	someZeroVideoConst = videoConst;
}

void init_main(legacy_s16 argc, legacy_s8* argv[])
{
	legacy_u16 i, j;
	legacy_u8 argmode4, argnosound, argnounknown;
	legacy_u32 timerdelta1, timerdelta2, timerdelta3;
	struct POINT2D tmppoint;
	struct RECTANGLE tmprect;

	// Keyboard
	kb_init_interrupt();
	dos_kb_clear_numlock();
	kb_call_readchar_callback();

	kb_reg_callback(0x0007, &show_graphic_levels_menu);
	kb_reg_callback(0x000A, &do_joy_restext);
	kb_reg_callback(0x000B, &do_key_restext);
	kb_reg_callback(0x3200, &do_mof_restext);
	kb_reg_callback(0x0010, &do_pau_restext);
	kb_reg_callback('p', &do_pau_restext);
	kb_reg_callback(0x0011, &do_dos_restext);
	kb_reg_callback(0x0013, &do_sonsof_restext);
	kb_reg_callback(0x0018, &do_dos_restext);
	
	// Video
	video_flag1_is1 = 1;
	video_flag2_is1 = 1;
	video_flag3_isFFFF = -1;
	video_flag4_is1 = 1;

	mmgr_alloc_a000();
	himem_init();

	video_flag5_is0 = 0;
	video_flag6_is1 = 1;
	
	textresprefix = 'e';
	
	// Parse arguments.
	argmode4 = 0;
	argnosound = 0;
	argnounknown = 0;
	
	for (i = 1; argc > i; ++i) {
		if (argv[i][0] == '/') {
			switch (argv[i][1]) {
				case 'h':
					argmode4 = 4;
					break;

				case 'n':
					if (argv[i][2] == 's') {
						argnosound = 1;
					}
					else if (argv[i][2] == 'd') {
						argnounknown = 1;
					}
					break;

				case 's':
				if (strlen(argv[i]) >= 4) {
					if (
						   (argv[i][2] == 'S' || argv[i][2] == 's')
						&& (argv[i][3] == 'B' || argv[i][3] == 'b'))
					{
						// We do not have Sound Blaster drivers.
						// Replace them with Adlib
						audiodriverstring[0] = 'a';
						audiodriverstring[1] = 'd';
					}
					else {
						audiodriverstring[0] = argv[i][2];
						audiodriverstring[1] = argv[i][3];
					}
					break;
				}
			}
		}
	}
	
	// Unused "/nd" switch. Maybe used when loading other video drivers?
	(void)argnounknown;

	// Video mode.
	dos_video_set_mode_13h();
	if (argmode4) {
		dos_video_set_mode4();
	}

	dos_timer_setup_interrupt();

	sprite_copy_2_to_1_clear();

	dos_mouse_init(0x0140, 0x00C8);

	// Audio driver.
	if (audio_load_dos_driver(audiodriverstring, 0, 0)) {
		dos_timer_shutdown();
		dos_process_exit(1);
	}
	
	if (argnosound) {
		audio_toggle_flag2();
		audio_toggle_flag6();
	}
	
	dos_set_critical_error_handler(&do_dea_textres);
	
	load_palandcursor();
	
	// Timing measures.
	sprite_copy_2_to_1();
	sprite_set_1_size(0, 320, 0, 120);

	timer_get_delta_alt();
	for (i = 0; i < 15; ++i) {
		sprite_clear_1_color(0);
	}
	timerdelta1 = timer_get_delta_alt();
	
	sprite_set_1_size(0, 320, 0, 60);

	for (i = 0; i < 15; ++i) {
		tmprect.left = tmprect.right = tmprect.top = tmprect.bottom = 0;
		
		for (j = 0; j < 400; ++j) {
			tmppoint.px = tmppoint.py = j;
			rect_adjust_from_point(&tmppoint, &tmprect);
		}
		
		sprite_clear_1_color(0);
	}
	
	timerdelta2 = timer_get_delta_alt();

	for (i = 0; i < 146; ++i) {
		for (j = 0; j < 255; ++j) {
			rect_adjust_from_point(&tmppoint, &tmprect);
		}
	}
	
	timerdelta3 = timer_get_delta_alt();
	
	slow_video_mgmt = (timerdelta2 <= timerdelta1);
	framespersec2 = (timerdelta3 >= 75) ? 10 : 20;

	if (timerdelta3 < 35) {
		detail_level = 0;
	}
	else if (timerdelta3 < 55) {
		detail_level = 1;
	}
	else if (timerdelta3 < 75) {
		detail_level = 2;
	}
	else if (timerdelta3 < 100) {
		detail_level = 3;
	}
	else if (slow_video_mgmt) {
		detail_level = 4;
	}
	else {
		detail_level = 3;
	}

	framespersec = framespersec2;
	slow_video_mgmt_copy = slow_video_mgmt;
	
	random_wait();
	
	copy_material_list_pointers(material_clrlist_ptr, material_clrlist2_ptr, material_patlist_ptr, material_patlist2_ptr, 0);
}

legacy_s16 stuntsmain2(legacy_s16 argc, legacy_s8* argv[]) {
	legacy_s16 result;
	legacy_s8 far* textresptr;
	legacy_s16 carposangle;
	struct SPRITE far* var42wnd;
	legacy_s16 counter;
	legacy_s16 inch;
	legacy_s16 shapeindex;

	// initialization
	init_main(argc, argv);
	init_div0();
	init_row_tables();
	
	mainresptr = file_load_resfile("main");
	fontdefptr = file_load_resource(0, "fontdef.fnt");
	fontnptr = file_load_resource(0, "fontn.fnt");
	
	font_set_fontdef();
	init_polyinfo();
	init_trackdata();

	init_unknown();
	
	init_kevinrandom("kevin");

	strcpy(gameconfig.game_trackname, "DEFAULT");
	
	input_do_checking(1);
	input_do_checking(1);
	mouse_draw_opaque_check();

	kbormouse = 0;
	passed_security = 1;  // set to 0 for the original copy protection	
	//set_default_car();
		
	// try do something
	sub_29772();
	set_projection(0x24, 0x11, 0x140, 0x64);	// would at best draw just a pixel without this - camera projection??

	wndsprite = sprite_make_wnd(320, 100, 0x0F);

	//run_intro_looped();
	
	carposangle = polarAngle(carpos.y, carpos.z);

	shape3d_load_all();
	shape3d_load_car_shapes("coun", "coun");
	select_cliprect_rotate(0, carposangle, 0, &cliprect, 0);

	//shaperect = cliprect;
	transshape.material = 0;
	transshape.rotvec.x = 0;
	transshape.rotvec.y = 0;
	transshape.pos = carpos;

	transshape.unk = 0;//0x7530;
	transshape.ts_flags = 0;
	transshape.rectptr = &shaperect;

	counter = 0;
	shapeindex = 24;
	for (; ; counter++) {

		transshape.rotvec.z = 0; //counter + 0x230;
		
		// seg000:1C58                 mov     [bp+var_transshape.ts_shapeptr], (offset game3dshapes.shape3d_numverts+0AA8h)
		// 0xAA8 / sizeof(SHAPE3D) = 0xAA8 / 0x16 = 124, points at where car0 is loaded during shape3d_load_car_shapes();

		transshape.shapeptr = &game3dshapes[shapeindex];

		//transshape.shapeptr = &game3dshapes[124];
		//transshape.shapeptr = &game3dshapes[124];
		
		transformed_shape_op(&transshape);
		
		sprite_copy_wnd_to_1();
		sprite_clear_1_color(3);
		
		//sprite_set_1_size(50, 200, 50, 100);
		get_a_poly_info(); // renders to sprite1
	
		//sprite_copy_2_to_1_2();
		sprite_blit_to_video(wndsprite, 0);
		
		inch = get_kb_or_joy_flags();//kb_get_char();
		if (inch == 4) { // right
			shapeindex++;
			shapeindex = (shapeindex + 0x74) % 0x74;
		} else
		if (inch == 8) { // left
			shapeindex--;
			shapeindex = (shapeindex + 0x74) % 0x74;
		} else
		if (inch != 0) {
			textresptr = locate_text_res(mainresptr, "dos");
			//result = show_dialog(2, 1, textresptr, 0xFFFF, 0xFFFF, dialogarg2, 0, 0); // center
			result = show_dialog(2, 1, textresptr, 0, 170, dialogarg2, 0, 0);
			if (result >= 1)
				break;
		}
	}

	//var42wnd = sprite_make_wnd(320, 200);
	//setup_mcgawnd2();
	//sprite_set_1_size(0, 320, 0, 200);
	//sprite_copy_2_to_1_2();
	//sprite_clear_1_color(2);
		//sprite_copy_wnd_to_1();
		//sprite_copy_2_to_1_2();
	
		//sprite_putimage(wndsprite->sprite_bitmapptr);
		//sprite_putimage(var42wnd->sprite_bitmapptr);
	
	//fatal_error("happy yet?");


	// shutdown
	mouse_draw_opaque_check();
	dos_timer_shutdown();
	dos_audio_shutdown();
	kb_exit_handler();
	dos_kb_set_numlock();
	dos_video_set_mode7();
	
	fatal_error("err %i", inch);

	return 0;
}

legacy_s16 stuntsmainimpl(legacy_s16 argc, legacy_s8* argv[]) {

	legacy_s16 i, result;
	legacy_s16 regax, regsi;
	legacy_s8 var_A;
	legacy_s8 far* trkptr;
	legacy_s8 far* textresptr;
	
	//return ported_stuntsmain_(argc, argv);

	init_main(argc, argv);
	init_div0();
	init_row_tables();
	
	mainresptr = file_load_resfile("main");
	
	fontdefptr = file_load_resource(0, "fontdef.fnt");
	fontnptr = file_load_resource(0, "fontn.fnt");

	font_set_fontdef();
	init_polyinfo();
	
	init_trackdata();

	init_unknown();
	
	init_kevinrandom("kevin");
	
	strcpy(gameconfig.game_trackname, "DEFAULT");
	
	//fatal_error("ai");
	
	input_do_checking(1);
	input_do_checking(1);
	mouse_draw_opaque_check();
	
	kbormouse = 0;
	passed_security = 1;  // set to 0 for the original copy protection
	set_default_car();

	regsi = 1;

	while (1) {

		ensure_file_exists(2);
		
		if (regsi != 0) {
			file_build_path(byte_3B80C, gameconfig.game_trackname, ".trk", g_path_buf);
			file_read_fatal(g_path_buf, td14_elem_map_main);
		}
		
		idle_expired = 0;
		result = run_intro_looped();
		if (result == 27) {
			textresptr = locate_text_res(mainresptr, "dos");
			result = show_dialog(2, 1, textresptr, 0xFFFF, 0xFFFF, dialogarg2, 0, 0);
			if (result >= 1) {
				mouse_draw_opaque_check();
				dos_timer_shutdown();
				dos_audio_shutdown();
				kb_exit_handler();
				dos_kb_set_numlock();
				dos_video_set_mode7();
				return result;
			}
			regsi = 0;
			continue;
		}

		while (1) {
			ensure_file_exists(2);
			if (is_audioloaded == 0) {
				file_load_audiores("skidslct", "skidms", "SLCT");
			}
			result = run_menu();
			if (result == -1)  {
				audio_unload();
				regsi = 0;
				break;
			} else if (result == 0) {
				var_A = 0;
			} else if (result == 1) {
				check_input();
				show_waiting();
				run_car_menu(&gameconfig, &gameconfig.game_playermaterial, &gameconfig.game_playertransmission, 0);
				continue;
			} else if (result == 2) {
				check_input();
				show_waiting();
				run_opponent_menu();
				continue;
			} else if (result == 3) {
				run_tracks_menu(0);
				continue;
			} else if (result == 4) {
				check_input();
				show_waiting();
				result = run_option_menu();
				if (result == 0) {
					continue;
				} else {
					// goto replay-mode if option-menu-result != 0
					var_A = 1;
				}
			} else {
				continue;
			}

			_memcpy(&gameconfigcopy, &gameconfig, sizeof(struct GAMEINFO));
			for (i = 0; i < 0x70A; i++) {
				td20_trk_file_appnd[i] = td14_elem_map_main[i];
			}
			for (i = 0; i < 0x51; i++) {
				td20_trk_file_appnd[i + 0x70A] = byte_3B80C[i];
				td20_trk_file_appnd[i + 0x75B] = byte_3B85E[i];
			}
			
			if (idle_expired == 0) {
				result = track_setup();
				//result = setup_track();
				if (result != 0) {
					run_tracks_menu(1);
					continue;
				}
				random_wait();
				if (passed_security == 0) {
					fatal_error("security check");
					//get_super_random();
					//security_check();
				}
			} else if (file_find("tedit.*") == 0) {
				audio_unload();
				regsi = 0;
				break;
			}

			audio_unload();

			cvxptr = mmgr_alloc_resbytes("cvx", sizeof(struct GAMESTATE) * RST_CVX_NUM);
			init_game_state(-1);
			
			if (var_A != 0) {
				byte_43966 = 0;
 			} else {

				gameconfig.game_recordedframes = 0;
			}

			while (1) {
				show_waiting();
				run_game();
				if (idle_expired == 0 && byte_43966 != 0) {
					result = end_hiscore();
					if (result == 0) {
						// view replay
						byte_43966 = 4;
						continue;
					} else if (result == 1) {
						// drive
						gameconfig.game_recordedframes = 0;
						continue;
					}
				}
				// main menu
				break;
			}

			_memcpy(&gameconfigcopy, &gameconfig, sizeof(struct GAMEINFO));
			for (i = 0; i < 0x70A; i++) {
				td14_elem_map_main[i] = td20_trk_file_appnd[i];
			}
			for (i = 0; i < 0x51; i++) {
				byte_3B80C[i] = td20_trk_file_appnd[i + 0x70A];
				byte_3B85E[i] = td20_trk_file_appnd[i + 0x75B];
			}
			mmgr_release(cvxptr);
			
			if (idle_expired != 0) {
				regsi = 0;
				break;
			}
		}
	
	}
}

