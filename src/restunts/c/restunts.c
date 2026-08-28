#include <dos.h>
#include <stddef.h>
#include "restunts.h"
#include "fileio.h"
#include "keyboard.h"
#include "legacy.h"
#include "math.h"
#include "memmgr.h"
#include "shape2d.h"
#include "shape3d.h"

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
extern unsigned const char* g_ascii_props;
extern legacy_u16 joyflag1;
extern legacy_u16 word_3FB18;
extern legacy_u16 word_3FB1C;
extern legacy_u16 word_3FB26;
extern legacy_u16 word_3FB2A;
extern legacy_u16 word_3FB34;
extern legacy_u8 byte_3FB38[];
extern legacy_u8 byte_449CE;
extern int word_46170[7];
extern legacy_u8 byte_44292[64];
extern legacy_u8 byte_442EA[64];
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

int get_0(void)
{
	return 0;
}

void sub_307B4(void)
{
	byte_3FE00 = 1;
	word_3FB18 = 0x50U;
	word_3FB1C = 0;
	word_3FB26 = 0x50U;
	word_3FB2A = 0;
}

int sub_307D2(int index)
{
	return byte_3FB38[(legacy_u16)index & 0x0FU];
}

int sub_307E3(void)
{
	legacy_u16 difference;
	legacy_u32 scaled;

	if (LEGACY_S16_FROM_BITS(joyflag1) <
		LEGACY_S16_FROM_BITS(word_3FB18))
		difference = 0;
	else
		difference = LEGACY_U16_WRAP_SUB(joyflag1, word_3FB18);
	scaled = (legacy_u32)difference * word_3FB34;
	return (legacy_u16)((legacy_u16)(scaled >> 8) - 0x1FU);
}

unsigned long timer_get_counter(void)
{
	/*
	unsigned long val;

	disable();
	val = timer_callback_counter;
	enable();

	return val;
	*/

	// Compiler produces too sloppy code; stick to asm for now...
	__asm {
		cli
		mov     ax, word ptr timer_callback_counter
		mov     dx, word ptr timer_callback_counter+2
		sti
	}
}

unsigned long timer_get_delta(void)
{
    /*
	unsigned long last, curr;
	
	last = last_timer_callback_counter;
	
	disable();
	curr = timer_get_counter();
	enable();
	
	last_timer_callback_counter = curr;

	return curr - last;
	*/
	
	// Compiler produces too sloppy code; stick to asm for now...
	__asm {
		mov     bx, word ptr last_timer_callback_counter
		mov     cx, word ptr last_timer_callback_counter+2
		cli
		mov     ax, word ptr timer_callback_counter
		mov     dx, word ptr timer_callback_counter+2
		sti
		mov     word ptr last_timer_callback_counter, ax
		mov     word ptr last_timer_callback_counter+2, dx
		sub     ax, bx
		sbb     dx, cx
	}
}

unsigned long timer_get_delta_alt(void)
{
	return timer_get_delta();
}

unsigned long timer_custom_delta(unsigned long ticks)
{
	return timer_get_counter() - ticks;
}

void timer_reset()
{
	timer_callback_counter = 0;
}

unsigned long timer_copy_counter(unsigned long ticks)
{
	timer_copy_unk = timer_get_counter() + ticks;
	return timer_copy_unk;
}

unsigned long timer_wait_for_dx(void)
{
	unsigned long res;
	do {
		res = timer_get_counter();
	} while (res < timer_copy_unk);
	
	return res;
}

int timer_compare_dx(void)
{
	return timer_get_counter() >= timer_copy_unk;
}

unsigned long timer_get_counter_unk(unsigned long ticks)
{
	unsigned long target, res;
	target = timer_get_counter() + ticks;
	
	do {
		res = timer_get_counter();
	} while (res < target);
	
	return res;
}

extern legacy_u16 word_46468;
extern legacy_u8 byte_442E4;
extern legacy_s16 word_44D1E;
extern legacy_s16 word_449E4;
extern legacy_s16 word_443F4;
extern legacy_u8 unk_44F4C[];
extern void far frame_callback(void);
extern void replay_unk2(int mode);
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

#define KEVINRANDOM_SEED_LEN 6
void init_kevinrandom(const char* seed)
{
	int i;

	for (i = 0; i < KEVINRANDOM_SEED_LEN; ++i) {
		g_kevinrandom_seed[i] = seed[i];
	}
}

void get_kevinrandom_seed(char* seed)
{
	int i;

	for (i = 0; i < KEVINRANDOM_SEED_LEN; ++i) {
		seed[i] = g_kevinrandom_seed[i];
	}
}

int get_kevinrandom(void)
{
	g_kevinrandom_seed[4] += g_kevinrandom_seed[5];
	g_kevinrandom_seed[3] += g_kevinrandom_seed[4];
	g_kevinrandom_seed[2] += g_kevinrandom_seed[3];
	g_kevinrandom_seed[1] += g_kevinrandom_seed[2];
	g_kevinrandom_seed[0] += g_kevinrandom_seed[1];
	
	!++g_kevinrandom_seed[5] 
	&& !++g_kevinrandom_seed[4]
	&& !++g_kevinrandom_seed[3]
	&& !++g_kevinrandom_seed[2]
	&& !++g_kevinrandom_seed[1]
	&& ++g_kevinrandom_seed[0];

	return g_kevinrandom_seed[0];
}

int get_super_random(void)
{
	int val = rand() + get_kevinrandom() + timer_get_counter() + gState_frame;
	return val < 0 ? -val : val;
}

int video_get_status(void)
{
	return inport(0x3DA) & 0x8;
}

int random_wait(void)
{
	int status1, i;
	
	status1 = video_get_status();
	
	for (i = 0; status1 == video_get_status() && i < 12000; ++i);
	
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

int toupper(int ch)
{
	if (ch >= 'a' && ch <= 'z') {
		ch -= ' ';
	}
	
	return ch;
}

void init_row_tables(void) {
	int i;
	for (i = 0; i < 30; i++) {
		trackrows[i] = 30 * (29 - i);
		terrainrows[i] = 30 * i;
		trackpos[i] = (29 - i) << 10;
		trackpos2[i] = i << 10;
		trackcenterpos[i] = ((29 - i) << 10) + 0x200;
		terrainpos[i] = i << 10;
		terraincenterpos[i] = (i << 10) + 0x200;
		trackcenterpos2[i] = (i << 10) + 0x200;
	}
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

void init_trackdata(void) {
	char far* trkptr;
	trkptr = mmgr_alloc_resbytes("trakdata", 0x6BF3);

	td01_track_file_cpy = trkptr;
	
	trkptr += 0x70a;
	td02_penalty_related = trkptr;
	
	trkptr += 0x70a;
	trackdata3 = trkptr;

	trkptr += 0x70a;
	td04_aerotable_pl = trkptr;

	trkptr += 0x80;
	td05_aerotable_op = trkptr;

	trkptr += 0x80;
	trackdata6 = trkptr;

	trkptr += 0x80;
	trackdata7 = trkptr;

	trkptr += 0x80;
	td08_direction_related = trkptr;

	trkptr += 0x60;
	trackdata9 = trkptr;

	trkptr += 0x180;
	td10_track_check_rel = trkptr;

	trkptr += 0x120;
	td11_highscores = trkptr;

	trkptr += 0x16c;
	trackdata12 = trkptr;

	trkptr += 0x0f0;
	td13_rpl_header = trkptr;

	trkptr += 0x1a;
	td14_elem_map_main = trkptr;

	trkptr += 0x385;
	td15_terr_map_main = trkptr;

	trkptr += 0x385;
	td16_rpl_buffer = trkptr;

	trkptr += 0x2ee0;
	td17_trk_elem_ordered = trkptr;

	trkptr += 0x385;
	trackdata18 = trkptr;

	trkptr += 0x385;
	trackdata19 = trkptr;

	trkptr += 0x385;
	td20_trk_file_appnd = trkptr;

	trkptr += 0x7ac;
	td21_col_from_path = trkptr;

	trkptr += 0x385;
	td22_row_from_path = trkptr;

	trkptr += 0x385;
	trackdata23 = trkptr;

	trkptr += 0x30;
}

extern struct SHAPE3D game3dshapes[];

extern unsigned select_cliprect_rotate(int angX, int angY, int angZ, struct RECTANGLE* cliprect, int unk);
//extern void transformed_shape_op(struct TRANSFORMSHAPE3D* shape);
extern void sub_29772(void);
extern void set_projection(int, int, int, int);

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
extern void video_set_palette(unsigned int start, unsigned int count,
	unsigned char* palette);

extern struct RECTANGLE cliprect_unk;
//cliprect_unk    RECTANGLE <270Fh, 0FFFFh, 270Fh, 0FFFFh>

extern int polyinfonumpolys;
extern unsigned char far* polyinfoptrs[]; // array size = 0x190 
extern unsigned int poly_linked_list_40ED6[]; // array size = 0x190

extern void preRender_default(int color, int vertlinecount, int* vertlines);
extern char byte_3B8F6;
extern char far* skybox_res_ofs;
extern char far* sdgame2ptr;
extern int sdgame2_widths[];
extern char far* sdgame2shapes[];
extern char byte_46167;
extern unsigned int skybox_ptr1;
extern unsigned int skybox_ptr2;
extern unsigned int skybox_ptr3;
extern unsigned int skybox_ptr4;
extern unsigned int skybox_current;
extern unsigned int word_454CE;
extern int skybox_sky_color;
extern int skybox_grd_color;
extern int skybox_wat_color;
extern int dialog_fnt_colour;
extern int meter_needle_color;
extern char far* skyboxes[];
extern int word_45D1C;
extern int word_45D06;
extern int idle_counter;
extern char byte_3B8F7;
extern char mouse_isdirty;
extern legacy_u8 HKeyFlag;

void load_palandcursor(void)
{
	unsigned char palette[0x300];
	char far* resource;
	struct SHAPE2D far* mouse_shape;
	unsigned int mouse_width;
	unsigned int mouse_height;
	unsigned int i;

	resource = (char far*)file_load_shape2d_fatal("sdmain");
	mouse_shape = (struct SHAPE2D far*)locate_shape_fatal(resource, "!pal");
	for (i = 0; i < sizeof(palette); ++i)
		palette[i] = ((unsigned char far*)mouse_shape)[0x10U + i];
	video_set_palette(0, 0x100, palette);

	mouse_shape = (struct SHAPE2D far*)locate_shape_fatal(resource, "smou");
	mouse_width = (legacy_u16)(mouse_shape->s2d_width * video_flag2_is1);
	mouse_height = mouse_shape->s2d_height;
	mmgr_free(resource);

	smouspriteptr = sprite_make_wnd(mouse_width, mouse_height, 0x0F);
	mmouspriteptr = sprite_make_wnd(mouse_width, mouse_height, 0x0F);
	mouseunkspriteptr = sprite_make_wnd(
		mouse_width + video_flag2_is1, mouse_height, 0x0F);

	resource = (char far*)file_load_shape2d_fatal("sdmain");
	sprite_set_1_from_argptr(smouspriteptr);
	mouse_shape = (struct SHAPE2D far*)locate_shape_fatal(resource, "smou");
	sprite_shape_to_1(mouse_shape, 0, 0);

	sprite_set_1_from_argptr(mmouspriteptr);
	mouse_shape = (struct SHAPE2D far*)locate_shape_fatal(resource, "mmou");
	sprite_shape_to_1(mouse_shape, 0, 0);

	mmgr_free(resource);
	sprite_copy_2_to_1_2();
}

static char skybox_resource_names[5][9] = {
	"desert",
	"tropical",
	"alpine",
	"city",
	"country"
};

void init_unknown(void)
{
	byte_44A8A = 1;
	byte_4552F = 2;
	elapsed_time2 = 0;
	byte_449DA = 0;
	byte_4393C = 0;
	word_44DCA = 0;
}

int handle_ingame_kb_shortcuts(int key)
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

void copy_string(char* destination, char far* source)
{
	/* Preserve the original post-copy lookahead, including its empty input bug. */
	do {
		*destination = *source;
		destination++;
		source++;
	} while (*source != 0);
	*destination = 0;
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

int mouse_multi_hittest(int count, int* x1_array, int* x2_array,
	int* y1_array, int* y2_array)
{
	int i;

	if (kbormouse == 0)
		return -1;

	for (i = 0; i < count; i++) {
		if (x1_array[i] <= mouse_xpos && mouse_xpos <= x2_array[i] &&
			y1_array[i] <= mouse_ypos && mouse_ypos <= y2_array[i])
			return (signed char)i;
	}

	return -1;
}

extern int input_framecount;
extern int input_framecount2;
extern int input_framecount3;
extern int input_framecounter;
extern int kbjoyflags;
extern int joyflags;
extern int newjoyflags;
extern int joyinputcode;
extern int mouse_oldx;
extern int mouse_oldy;
extern int mouse_oldbut;
extern int mousebutinputcode;
extern int get_joy_flags(void);
extern void mouse_get_state(int* buttons, int* x, int* y);
extern unsigned char byte_3EBD8;
extern char byte_45D0C[];
extern char byte_45D14[];

int input_checking(int frame_delta)
{
	legacy_u16 current_joy_flags;
	legacy_u16 key;
	int changed_or_repeating;

	input_framecount = LEGACY_U16_WRAP_ADD(input_framecount, frame_delta);
	if (LEGACY_S16_FROM_BITS(input_framecount) > 20000) {
		input_framecount = LEGACY_U16_WRAP_SUB(input_framecount, 10000U);
		input_framecount2 = LEGACY_U16_WRAP_SUB(input_framecount2, 10000U);
		input_framecount3 = LEGACY_U16_WRAP_SUB(input_framecount3, 10000U);
	}

	key = (legacy_u16)kb_get_char();
	if (key != 0)
		kbormouse = 0;
	current_joy_flags = (legacy_u16)get_joy_flags();
	kbjoyflags = get_kb_or_joy_flags();
	changed_or_repeating = 0;
	if ((legacy_u16)joyflags != current_joy_flags) {
		newjoyflags = ((legacy_u16)joyflags ^ current_joy_flags) &
			current_joy_flags;
		joyflags = current_joy_flags;
		changed_or_repeating = 1;
	} else if (current_joy_flags != 0 &&
		LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_ADD(
			input_framecount3, 20U)) <
		LEGACY_S16_FROM_BITS(input_framecount)) {
		changed_or_repeating = 1;
	}

	if (changed_or_repeating) {
		if (((legacy_u16)newjoyflags & 0x20U) != 0)
			joyinputcode = 0x0D;
		else if (((legacy_u16)newjoyflags & 0x10U) != 0)
			joyinputcode = 0x20;
		else if (((legacy_u16)newjoyflags & 1U) != 0)
			joyinputcode = 0x4800;
		else if (((legacy_u16)newjoyflags & 2U) != 0)
			joyinputcode = 0x5000;
		else if (((legacy_u16)newjoyflags & 8U) != 0)
			joyinputcode = 0x4B00;
		else if (((legacy_u16)newjoyflags & 4U) != 0)
			joyinputcode = 0x4D00;

		if (joyinputcode != 0) {
			input_framecount3 = input_framecount;
			kbormouse = 0;
		}
	}

	mouse_get_state(&mouse_butstate, &mouse_xpos, &mouse_ypos);
	if (mouse_oldx != mouse_xpos || mouse_oldy != mouse_ypos ||
		mouse_oldbut != mouse_butstate) {
		mouse_oldx = mouse_xpos;
		mouse_oldy = mouse_ypos;
		kbormouse = 1;
		input_framecounter = 0;
		if (byte_3B8F7 != 0) {
			if (mouse_isdirty != 0)
				mouse_draw_opaque();
			mouse_draw_transparent();
		}
	} else if (kbormouse != 0) {
		input_framecounter = LEGACY_U16_WRAP_ADD(
			input_framecounter, frame_delta);
		if (LEGACY_S16_FROM_BITS(input_framecounter) > 500) {
			input_framecounter = 0;
			kbormouse = 0;
			if (mouse_isdirty != 0)
				mouse_draw_opaque();
		}
	}

	if (kbormouse != 0) {
		changed_or_repeating = 0;
		if (mouse_oldbut != mouse_butstate) {
			mouse_oldbut = mouse_butstate;
			changed_or_repeating = 1;
		} else if (mouse_butstate != 0 &&
			LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_ADD(
				input_framecount2, 20U)) <
			LEGACY_S16_FROM_BITS(input_framecount)) {
			changed_or_repeating = 1;
		}

		if (changed_or_repeating) {
			if (((legacy_u16)mouse_butstate & 1U) != 0)
				mousebutinputcode = 0x20;
			else if (((legacy_u16)mouse_butstate & 2U) != 0)
				mousebutinputcode = 0x0D;
			if (mousebutinputcode != 0)
				input_framecount2 = input_framecount;
			input_framecounter = 0;
		}

		if (mouse_butstate != 0) {
			if (((legacy_u16)mouse_butstate & 1U) != 0)
				kbjoyflags = (legacy_u16)kbjoyflags | 0x20U;
			else if (((legacy_u16)mouse_butstate & 2U) != 0)
				kbjoyflags = (legacy_u16)kbjoyflags | 0x10U;
		}
	}

	if (key != 0)
		return key;
	if (joyinputcode != 0) {
		key = (legacy_u16)joyinputcode;
		joyinputcode = 0;
		return key;
	}
	if (mousebutinputcode != 0) {
		key = (legacy_u16)mousebutinputcode;
		mousebutinputcode = 0;
		return key;
	}
	return 0;
}

int input_do_checking(int frame_delta)
{
	return input_checking(frame_delta);
}

void check_input(void)
{
	int pressed;

	do {
		pressed = (get_kb_or_joy_flags() & 0x30) != 0;
		if (!pressed) {
			pressed = input_checking(
				(int)timer_get_delta_alt()) != 0;
		}
		if (!pressed && kbormouse != 0 && (mouse_butstate & 3) != 0)
			pressed = 1;
	} while (pressed);
}

void nopsub_28F26(void)
{
	do {
		/* Keep advancing input state until an event is reported. */
	} while (input_checking((int)timer_get_delta_alt()) == 0);

	check_input();
}

void input_push_status(void)
{
	int index = (signed char)byte_3EBD8;

	byte_45D0C[index] = byte_3B8F7;
	byte_45D14[index] = kbormouse;
	byte_3EBD8++;
}

void input_pop_status(void)
{
	int index;

	if (byte_3EBD8 == 0)
		return;

	byte_3EBD8--;
	index = (signed char)byte_3EBD8;
	byte_3B8F7 = byte_45D0C[index];
	kbormouse = byte_45D14[index];
	if (kbormouse == 0)
		mouse_draw_opaque_check();
}

extern int font_op2(const char* text);
extern int font_op(const char* text, int count);
extern unsigned int word_42A16;
extern unsigned int word_42A18;
extern unsigned int word_42A1A;
extern unsigned int word_42A1C;
extern char* off_42A1E;
extern unsigned int word_42A20;
extern unsigned int word_42A22;
extern legacy_u8 far* word_405FE;
extern void sub_345BC(const char* text, int x, int y);
extern void sprite_1_unk2(int x, int y, int width, int height, int color);

int font_op2_alt(const char* text)
{
	legacy_s16 centered;

	centered = LEGACY_S16_WRAP_NEGATE(
		LEGACY_S16_WRAP_SUB(font_op2(text), 0x140));
	return (int)((long)centered / 2L);
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

extern void audio_driver_func3F(int command);
extern char audioflag2;
extern char audioflag6;
extern int word_4063A;
extern int word_4063C;
extern unsigned char byte_40635;
extern char audiodriverstring2[];
extern unsigned char byte_44290;
extern unsigned char byte_40630;
extern unsigned char byte_40632;
extern unsigned char byte_45950;
extern char byte_459D8;
extern unsigned char byte_428D6[];
extern unsigned char audiochunks_unk[];
extern unsigned char audiochunks_unk2[];
extern unsigned char byte_45948;
extern unsigned char byte_45D9A[];
extern unsigned char byte_44D06[];
extern unsigned char byte_44ACA[];
extern unsigned char unk_45A26[];
extern unsigned char audiotimers[];
extern void far* basdres;
extern void far* snarres;
extern void far* tommres;
extern void far* rideres;
extern void far* crshres;
extern void far* chhtres;
extern void far* ohhtres;
extern int word_43964;
extern int word_4408C;
extern unsigned int word_42240;
extern unsigned int word_42242;
extern unsigned int word_42244;
extern unsigned char byte_42246;
extern legacy_s16 word_3EB2A;
extern unsigned char byte_40634;
extern char aStartengineNew[];
extern char audio_filetemp[];
int compare_ds_ss(void);
void nopsub_3219D(const char* format, ...);
void audio_driver_timer(void);
extern void sub_38CF8(int index, void far* context);
extern void audio_map_song_instruments(void far* song,
	void far* instruments);
extern void audio_map_song_tracks(void far* song);
extern int sub_39050(unsigned int value, int handle);
extern void sub_39088(int channel, int value);
extern void sub_35B76(int x, int y, int width, int height, int color);
extern void audio_driver_func1E(int channel, int function);
extern void audio_unk2(int channel, int value);
extern void audio_op_unk3(int channel);
extern void audio_op_unk4(int channel);
extern void sub_39700(void);
int sub_37470(int channel, unsigned char priority);
void sub_374DE(int channel);
int sub_3771E(int channel);
void audio_init_chunk2(int channel);
void sub_38156(int index);
extern int audio_check_flag(void far* resource, int channel,
	unsigned char priority, unsigned int rate);
extern void audio_init_chunk(int first_channel, int last_channel,
	void far* resource, unsigned int resource_data_offset,
	unsigned int rate, unsigned char priority);

static void far* audio_read_far_pointer(const unsigned char* source)
{
	return MK_FP(LEGACY_READ_U16_LE(source + 2),
		LEGACY_READ_U16_LE(source));
}

static void audio_write_far_pointer(unsigned char* destination,
	const void far* value)
{
	LEGACY_WRITE_U16_LE(destination, FP_OFF(value));
	LEGACY_WRITE_U16_LE(destination + 2, FP_SEG(value));
}

void audio_add_driver_timer(void)
{
	unsigned int index;

	for (index = 0; index < 25U; index++)
		audiotimers[index * 0x4CU] = 0;
	word_42240 = 0x16U;
	timer_reg_callback(&audio_driver_timer);
}

void audio_remove_driver_timer(void)
{
	unsigned int index;
	unsigned int offset;
	int channel;

	for (index = 0; index < 25U; index++) {
		offset = index * 0x4CU;
		if (audiotimers[offset] == 1) {
			channel = LEGACY_S16_FROM_BITS(
				LEGACY_READ_U16_LE(audiotimers + offset + 2U));
			sub_374DE(channel);
		}
		audiotimers[offset] = 0;
	}
	timer_remove_callback(&audio_driver_timer);
}

char* pad_id(const char far* source)
{
	unsigned char* destination;
	unsigned int index;

	destination = (unsigned char*)&word_42242;
	for (index = 0; index < 4U; index++) {
		destination[index] = (unsigned char)source[index];
		if (destination[index] == 0)
			destination[index] = ' ';
	}
	byte_42246 = 0;
	return (char*)destination;
}

int audio_init_engine(int unused_type, void far* source_pointer,
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
	unsigned int index;
	unsigned int field;
	int channel;

	(void)unused_type;
	for (index = 0; index < 25U; index++) {
		if (audiotimers[index * 0x4CU] == 0)
			break;
	}
	if (index == 25U) {
		fatal_error("InitEngine: All handles used.");
		return -1;
	}

	timer = audiotimers + index * 0x4CU;
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
		resource = locate_shape_fatal((char far*)shape_resources,
			pad_id((const char far*)audio_read_far_pointer(
				context + 8U)));
		audio_write_far_pointer(context + 8U, resource);
		for (field = 0x10U; field <= 0x2CU; field += 4U) {
			resource = init_audio_resources(audio_resources,
				shape_resources,
				pad_id((const char far*)audio_read_far_pointer(
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
	return (int)index;
}

void audio_op_unk(int index)
{
	legacy_u8* timer;
	legacy_u8* context;
	const legacy_u8 far* definition;
	legacy_u16 offset;
	legacy_u16 sample_count;
	legacy_u16 value;
	legacy_u16 divisor;
	int handle;
	int channel;

	offset = LEGACY_U16_WRAP_MUL(index, 0x4CU);
	timer = audiotimers + offset;
	if (timer[0] != 1 || timer[1] != 0)
		return;

	handle = LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(timer + 2U));
	context = timer + 0x1CU;
	sub_38CF8(handle, audio_read_far_pointer(context + 8U));
	sample_count = LEGACY_READ_U16_LE(context);
	definition = (const legacy_u8 far*)audio_read_far_pointer(context + 8U);
	divisor = definition[0x0EU];
	value = (legacy_u16)(sample_count / divisor);
	value = LEGACY_U16_WRAP_ADD(value,
		(legacy_u16)((legacy_u16)definition[0x0FU] << 4));
	LEGACY_WRITE_U16_LE(timer + 0x0CU, value);
	channel = sub_39050(value, handle);
	LEGACY_WRITE_U16_LE(timer + 0x12U, channel);
	timer[1] = 1;
	timer[0x1AU] = 1;
	audio_unk2(handle, 0);
}

void audio_op_unk2(int index, int base_value,
	int first_x, int first_y, int first_z,
	int second_x, int second_y, int second_z,
	int interval)
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
	timer = audiotimers + offset;
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

void sub_18D06(const legacy_u8* sample, int interval)
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
	if (compare_ds_ss() == 0 || byte_442E4 != 0)
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

void audio_driver_timer(void)
{
	legacy_u8* timer;
	legacy_u32 accumulator;
	legacy_u16 volume_accumulator;
	legacy_u16 pitch;
	legacy_u16 index;
	legacy_u8 volume;
	legacy_u8 secondary_volume;
	int channel;

	if (compare_ds_ss() == 0)
		return;

	word_3EB2A = LEGACY_S16_WRAP_ADD(word_3EB2A, 1);
	if (word_3EB2A < 2 && byte_40634 != 0)
		return;

	for (index = 0; index < 25U; index++) {
		timer = audiotimers + index * 0x4CU;
		if (timer[0] == 0 || audioflag6 == 0)
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
			audio_unk2(channel, volume);
			secondary_volume = volume >= 10U ?
				(legacy_u8)(volume - 10U) : 0;
			channel = LEGACY_S16_FROM_BITS(
				LEGACY_READ_U16_LE(timer + 0x14U));
			if (channel != -1)
				audio_unk2(channel, secondary_volume);
			channel = LEGACY_S16_FROM_BITS(
				LEGACY_READ_U16_LE(timer + 0x16U));
			if (channel != -1)
				audio_unk2(channel, secondary_volume);
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
				sub_39088(channel, pitch);
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
	audio_driver_func3F(2);
	mmgr_free(songfileptr);
	mmgr_free(voicefileptr);
	is_audioloaded = 0;
}

void audio_enable_flag2(void)
{
	audioflag2 = 1;
}

void audio_disable_flag2(void)
{
	audioflag2 = 0;
	word_4063A = 1;
	if (byte_44290 != 0)
		audio_driver_func1E(0, (unsigned int)byte_44290 - 1U);
	sub_39700();
	word_4063A = 0;
}

short audio_toggle_flag2(void)
{
	if (audioflag2 == 1) {
		audio_disable_flag2();
		return 0;
	}

	audio_enable_flag2();
	return 1;
}

short nopsub_373FE(void)
{
	unsigned int offset;
	unsigned int channel;

	if (byte_40630 == 1 || audioflag2 == 0)
		return 1;

	for (channel = 0; channel < (unsigned int)byte_44290; channel++) {
		offset = (channel + 0x10U) * 0x4CU;
		if ((LEGACY_READ_U16_LE(audiochunks_unk + offset) |
			LEGACY_READ_U16_LE(audiochunks_unk + offset + 2)) != 0)
			return 0;
	}

	return 1;
}

void sub_3736A(void)
{
	word_4063A = 1;
	byte_40632 = 0;
	audio_driver_func1E(0, 0x0F);
	audio_init_chunk(0, 0x0F, 0, 0, byte_45950, 0);
	byte_44290 = 0;
	sub_39700();
	word_4063A = 0;
}

void audio_enable_flag6(void)
{
	int channel;

	if (audioflag6 == 1)
		return;

	for (channel = 0x10; channel < 0x18; channel++)
		audio_unk2(channel, byte_428D6[channel]);
	audioflag6 = 1;
}

void audio_disable_flag6(void)
{
	int channel;

	if (audioflag6 == 0)
		return;

	for (channel = 0x10; channel < 0x18; channel++) {
		byte_428D6[channel] =
			audiochunks_unk2[(channel - 0x10) * 0x4C + 0x28];
		audio_unk2(channel, 0);
	}
	audioflag6 = 0;
}

short audio_toggle_flag6(void)
{
	if (audioflag6 == 1) {
		audio_disable_flag6();
		return 0;
	}

	audio_enable_flag6();
	return 1;
}

int sub_3771E(int channel)
{
	unsigned int offset;

	if (audioflag6 == 0 || channel < 0x10 || channel > 0x17)
		return 1;

	offset = (unsigned int)channel * 0x4CU;
	return (LEGACY_READ_U16_LE(audiochunks_unk + offset) |
		LEGACY_READ_U16_LE(audiochunks_unk + offset + 2)) == 0;
}

void nopsub_37750(int channel, void far* value)
{
	void far* *field;

	field = (void far* *)(audiochunks_unk +
		(unsigned int)channel * 0x4CU + 0x48U);
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

void audio_init_chunk(int first_channel, int last_channel,
	void far* resource, unsigned int resource_data_offset,
	unsigned int rate, unsigned char priority)
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
		chunk = audiochunks_unk + chunk_offset;
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

int audio_check_flag(void far* resource, int channel,
	unsigned char priority, unsigned int rate)
{
	const legacy_u8 far* bytes;
	legacy_u16 scaled_rate;
	unsigned int offset;
	unsigned int resource_data_offset;
	int candidate;

	bytes = (const legacy_u8 far*)resource;
	if (audioflag6 == 0 || resource == 0 || bytes[5] != 1)
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
			offset = (unsigned int)(candidate - 0x10) * 0x4CU;
			if ((LEGACY_READ_U16_LE(audiochunks_unk2 + offset) |
				LEGACY_READ_U16_LE(audiochunks_unk2 + offset + 2)) == 0 &&
				byte_45D9A[candidate] == 0) {
				channel = candidate;
				break;
			}
		}
	}

	if (channel == -1)
		return -1;

	resource_data_offset = (unsigned int)bytes[6] * 4U + 8U;
	audio_init_chunk(channel, channel, resource, resource_data_offset,
		rate, priority);
	return channel;
}

int audio_check_flag2(void far* resource, int channel,
	unsigned char priority)
{
	return audio_check_flag(resource, channel, priority,
		(unsigned int)byte_45948);
}

int nopsub_37456(void far* resource)
{
	return audio_check_flag2(resource, -1, 0x40U);
}

int sub_37470(int channel, unsigned char priority)
{
	unsigned int offset;
	int candidate;

	if (channel == -1) {
		for (candidate = 0x10; candidate <= 0x17; candidate++) {
			offset = (unsigned int)(candidate - 0x10) * 0x4CU;
			if ((LEGACY_READ_U16_LE(audiochunks_unk2 + offset) |
				LEGACY_READ_U16_LE(audiochunks_unk2 + offset + 2)) == 0 &&
				byte_45D9A[candidate] == 0) {
				channel = candidate;
				break;
			}
		}
	}

	if (channel != -1) {
		byte_45D9A[channel] = 1;
		offset = (unsigned int)channel * 0x4CU;
		audiochunks_unk[offset + 0x24U] = priority;
	}

	return channel;
}

void audio_init_chunk2(int channel)
{
	unsigned int offset;

	if (channel < 0x10 || channel > 0x17)
		return;

	offset = (unsigned int)channel * 0x4CU;
	LEGACY_WRITE_U16_LE(audiochunks_unk + offset, 0);
	LEGACY_WRITE_U16_LE(audiochunks_unk + offset + 2, 0);
	audio_driver_func1E(channel, channel);
	audio_init_chunk(channel, channel, 0, 0, byte_45948, 0);
}

void audio_op_unk7(int index)
{
	unsigned int offset;
	int channel;

	offset = LEGACY_U16_WRAP_MUL(index, 0x4CU);
	channel = LEGACY_S16_FROM_BITS(
		LEGACY_READ_U16_LE(audiotimers + offset + 0x16U));
	audio_init_chunk2(channel);
	LEGACY_WRITE_U16_LE(audiotimers + offset + 0x16U, 0xFFFFU);
}

int nopsub_27489(int index)
{
	unsigned int offset;
	int channel;

	offset = LEGACY_U16_WRAP_MUL(index, 0x4CU);
	channel = LEGACY_S16_FROM_BITS(
		LEGACY_READ_U16_LE(audiotimers + offset + 0x14U));
	if (channel < 0)
		return 1;

	return sub_3771E(channel);
}

void audio_function2(int index)
{
	unsigned int offset;
	int channel;

	offset = LEGACY_U16_WRAP_MUL(index, 0x4CU);
	if (audiotimers[offset] != 1 || audiotimers[offset + 1U] != 1)
		return;

	channel = LEGACY_S16_FROM_BITS(
		LEGACY_READ_U16_LE(audiotimers + offset + 0x12U));
	sub_38156(channel);
	LEGACY_WRITE_U16_LE(audiotimers + offset + 0x12U, 0xFFFFU);
	audiotimers[offset + 1U] = 0;
	audiotimers[offset + 0x1AU] = 1;
}

static int audio_start_indexed_event(int index,
	unsigned int resource_field, unsigned char priority)
{
	unsigned int offset;
	unsigned int rate;
	int channel;
	void far* resource;

	offset = LEGACY_U16_WRAP_MUL(index, 0x4CU);
	rate = LEGACY_READ_U16_LE(audiotimers + offset + 4U) >> 4;
	resource = audio_read_far_pointer(
		audiotimers + offset + resource_field);
	channel = audio_check_flag(resource, -1, priority, rate);
	LEGACY_WRITE_U16_LE(audiotimers + offset + 0x14U, channel);
	audiotimers[offset + 0x1AU] = 1;
	return channel;
}

void nopsub_27220(int index)
{
	unsigned int offset;
	unsigned int rate;
	int channel;
	void far* resource;

	offset = LEGACY_U16_WRAP_MUL(index, 0x4CU);
	rate = LEGACY_READ_U16_LE(audiotimers + offset + 4U) >> 4;
	resource = audio_read_far_pointer(audiotimers + offset + 0x2CU);
	channel = audio_check_flag(resource, -1, 0x40U, rate);
	LEGACY_WRITE_U16_LE(audiotimers + offset + 0x14U, channel);
	nopsub_3219D(aStartengineNew, channel);
	audiotimers[offset + 0x1AU] = 1;
	audiotimers[offset + 0x1BU] = 1;
}

static void audio_append_filename_part(char* destination,
	const char* source)
{
	while (*destination != 0)
		destination++;
	do {
		*destination++ = *source;
	} while (*source++ != 0);
}

static const char* audio_find_last_backslash(const char* text)
{
	const char* match;

	match = 0;
	while (*text != 0) {
		if (*text == '\\')
			match = text;
		text++;
	}
	return match;
}

char* audio_make_filename(const char* filename, const char* extension,
	const char* inserted_path)
{
	const char* basename;
	const char* source;
	char* separator;
	unsigned int length;

	separator = audio_filetemp;
	source = filename;
	do {
		*separator++ = *source;
	} while (*source++ != 0);
	separator = (char*)audio_find_last_backslash(audio_filetemp);
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

void far* load_sfx_ge(const char* filename, const char* extension,
	const char* inserted_path)
{
	char compressed_extension[4];
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

void far* load_sfx_file(const char* filename)
{
	void far* result;

	result = 0;
	if (byte_40635 != 0)
		result = load_sfx_ge(filename, "dsf", audiodriverstring2);
	if (result == 0)
		result = load_sfx_ge(filename, "sfx", audiodriverstring2);
	if (result == 0 && word_4063C != 0)
		fatal_error("cannot load sfx file %s", filename);
	return result;
}

void far* load_song_file(const char* filename)
{
	void far* result;

	result = load_sfx_ge(filename, "kms", audiodriverstring2);
	if (result == 0 && word_4063C != 0)
		fatal_error("cannot load song file %s", filename);
	return result;
}

void far* load_voice_file(const char* filename)
{
	void far* result;

	result = 0;
	if (byte_40635 != 0)
		result = load_sfx_ge(filename, "dvc", audiodriverstring2);
	if (result == 0)
		result = load_sfx_ge(filename, "vce", audiodriverstring2);
	if (result == 0 && word_4063C != 0)
		fatal_error("cannot load voice file %s", filename);
	return result;
}

int audioresource_compare_chunknames(int case_sensitive,
	const char far* first_name, const char far* second_name, int count)
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

void sub_3702E(int left, int top, int right, int bottom, int color)
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

static legacy_u16 legacy_near_string_length(const char* text)
{
	legacy_u16 length;

	length = 0;
	while (*text++ != 0)
		length = LEGACY_U16_WRAP_ADD(length, 1U);
	return length;
}

void print_int_as_string_maybe(char* destination, int value, int zero_pad,
	int width)
{
	char digits[5];
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
		digits[digit_count++] = (char)('0' + magnitude % 10U);
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

static char* legacy_near_string_copy(char* destination, const char* source)
{
	while ((*destination = *source) != 0) {
		destination++;
		source++;
	}
	return destination;
}

void format_frame_as_string(char* destination, int frame_count,
	int include_hundredths)
{
	char number[18];
	char* output;
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

void parse_filepath_separators(char* destination, const char* path)
{
	legacy_u16 path_index;
	legacy_u16 output_index;
	char current;

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

extern char aId1[];
extern char aId2[];
extern char aId3[];
extern char aId4[];
extern char aDos_0[];
extern char aDea[];
extern char aDer[];
extern char aKey[];
extern char aMer[];
extern char aMof[];
extern char aMon[];
extern char aMou[];
extern char aPau[];
extern char aSof[];
extern char aSon[];
extern char aWai[];
extern char* findfilenames[];
extern void audio_unk(void);
extern void call_exitlist2(void);
extern void sub_372F4(void);

void ensure_file_exists(int file_index)
{
	static char* const message_ids[] = { aId1, aId2, aId3, aId4 };
	char* message_id;

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
	word_3F88E = 1;
	audio_unk();
	show_dialog(4, 1, locate_text_res(mainresptr, aKey),
		-1, -1, dialogarg2, 0, 0);
	byte_3FE00 = 0;
	byte_3B8F2 = 0;
	word_3F88E = 0;
	sub_372F4();
	input_pop_status();
}

void do_mou_restext(void)
{
	input_push_status();
	word_3F88E = 1;
	audio_unk();
	byte_3B8F2 = 1;
	show_dialog(4, 1, locate_text_res(mainresptr, aMou),
		-1, -1, dialogarg2, 0, 0);
	word_3F88E = 0;
	sub_372F4();
	input_pop_status();
}

void do_pau_restext(void)
{
	input_push_status();
	word_3F88E = 1;
	audio_unk();
	show_dialog(0, 1, locate_text_res(mainresptr, aPau),
		-1, -1, dialogarg2, 0, 0);
	word_3F88E = 0;
	sub_372F4();
	input_pop_status();
}

void do_mof_restext(void)
{
	char* message_id;

	input_push_status();
	word_3F88E = 1;
	message_id = audio_toggle_flag2() != 0 ? aMon : aMof;
	show_dialog(4, 1, locate_text_res(mainresptr, message_id),
		-1, -1, dialogarg2, 0, 0);
	word_3F88E = 0;
	input_pop_status();
}

void do_sonsof_restext(void)
{
	char* message_id;

	input_push_status();
	word_3F88E = 1;
	message_id = audio_toggle_flag6() != 0 ? aSon : aSof;
	show_dialog(4, 1, locate_text_res(mainresptr, message_id),
		-1, -1, dialogarg2, 0, 0);
	word_3F88E = 0;
	input_pop_status();
}

void do_dos_restext(void)
{
	int result;

	input_push_status();
	word_3F88E = 1;
	audio_unk();
	result = show_dialog(2, 1, locate_text_res(mainresptr, aDos_0),
		-1, -1, dialogarg2, 0, 0);
	if (result == 1)
		call_exitlist2();
	word_3F88E = 0;
	sub_372F4();
	input_pop_status();
}

short do_dea_textres(void)
{
	short result;

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

int input_repeat_check(int duration)
{
	legacy_u16 delta;
	legacy_u16 elapsed;
	int result;

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

int run_intro(void)
{
	struct SHAPE2D far* shape;
	int result;

	mouse_draw_opaque_check();
	sprite_copy_2_to_1_clear();
	mouse_draw_transparent_check();
	sprite_copy_wnd_to_1_clear();

	shape = (struct SHAPE2D far*)locate_shape_fatal(
		(char far*)tempdataptr, "prod");
	waitflag = shape->s2d_pos_y != 0 ? 0xA0 : 0xB4;

	shape = (struct SHAPE2D far*)locate_shape_fatal(
		(char far*)tempdataptr, "prod");
	sprite_shape_to_1_alt(shape);
	result = sprite_blit_to_video(wndsprite, -1);
	if (result == 0)
		result = input_repeat_check(0x190);

	if (result == 0) {
		sprite_copy_wnd_to_1_clear();
		waitflag = 0xB4;
		shape = (struct SHAPE2D far*)locate_shape_fatal(
			(char far*)tempdataptr, "titl");
		sprite_shape_to_1_alt(shape);
		result = sprite_blit_to_video(wndsprite, -1);
		if (result == 0)
			result = input_repeat_check(0x190);
	}

	return result;
}

int run_intro_looped(void)
{
	int result;

	file_load_audiores("skidtitl", "skidms", "TITL");
	tempdataptr = file_load_resource(2, "sdtitl");
	wndsprite = sprite_make_wnd(0x140, 0xC8, 0x0F);
	result = run_intro();
	sprite_free_wnd(wndsprite);
	mmgr_free((char far*)tempdataptr);

	if (result == 0) {
		result = setup_intro();
		if (result == 0) {
			tempdataptr = file_load_resource(2, "sdcred");
			wndsprite = sprite_make_wnd(0x140, 0xC8, 0x0F);
			sprite_copy_wnd_to_1_clear();
			sprite_blit_to_video(wndsprite, 0);
			result = load_intro_resources();
			sprite_free_wnd(wndsprite);
			mmgr_free((char far*)tempdataptr);
		}
	}

	audio_unload();
	return result;
}

extern void sprite_1_unk4(int x, int y, int width, int height, int color);

int mouse_timer_sprite_unk(int item_index, const int* x_values,
	const int* width_values, const int* y_values, const int* height_values,
	int second_state, int first_state)
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

extern int fontdef_unk_0E;
extern struct RECTANGLE word_42248;
extern struct RECTANGLE word_42250;
extern void font_draw_text(const char* text, int x, int y);

void font_set_unk(int color, int unknown)
{
	legacy_u8 far* font_definition;

	font_definition = word_405FE;
	font_definition[0] = (legacy_u8)color;
	font_definition[1] = 0;
	font_definition[2] = (legacy_u8)unknown;
	font_definition[3] = 0;
}

struct RECTANGLE* intro_draw_text(char* text, int x, int y, int color,
	int shadow_color)
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

struct RECTANGLE* hiscore_draw_text(char* text, int x, int y, int color,
	int shadow_color)
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

void far* sub_29A86(int operation, const char* filename,
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

int highscore_write_a(int create_default)
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

void mouse_minmax_position(int inset)
{
	if (inset != 0) {
		mouse_set_minmax(0x0F, 0, 0x131, 0xC8);
		mouse_set_position(0xA0, 0x64);
	} else {
		mouse_set_minmax(0, 0, 0x140, 0xC8);
	}
}

static int font_measure(const char* text, legacy_u16 remaining, int bounded)
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

int font_op(const char* text, int glyph_count)
{
	return font_measure(text, (legacy_u16)glyph_count, 1);
}

int font_op2(const char* text)
{
	return font_measure(text, 0, 0);
}

static legacy_u32 secondary_timer_target(void)
{
	return ((legacy_u32)word_3F1C4 << 16) | word_3F1C2;
}

static int secondary_timer_target_reached(
	legacy_u32 current,
	legacy_u32 target
) {
	return (legacy_u16)(current >> 16) >= (legacy_u16)(target >> 16) &&
		(legacy_u16)current >= (legacy_u16)target;
}

unsigned long set_add_value(unsigned long ticks)
{
	legacy_u32 target;

	target = (legacy_u32)(sub_2EAD4() + ticks);
	word_3F1C2 = (legacy_u16)target;
	word_3F1C4 = (legacy_u16)(target >> 16);
	return target;
}

int sub_2EB07(void)
{
	return secondary_timer_target_reached(
		sub_2EAD4(), secondary_timer_target());
}

unsigned long sub_2EB1E(unsigned long ticks)
{
	legacy_u32 current;
	legacy_u32 target;

	target = (legacy_u32)(sub_2EAD4() + ticks);
	do {
		current = sub_2EAD4();
	} while (!secondary_timer_target_reached(current, target));
	return current;
}

void add_exit_handler(void (far* exit_handler)(void))
{
	int index;

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
	int index;

	for (index = 10; index >= 0; index--)
		if (exitlistfuncs[index] != 0)
			exitlistfuncs[index]();
}

void call_exitlist2(void)
{
	call_exitlist();
	libsub_quit_to_dos_alt(0);
}

extern int read_line(int flags, char* text, int initial_key,
	int max_characters, int max_pixels, int x, int y,
	void (far* callback)(void), unsigned long timeout);
void read_line_helper(void);
void read_line_helper2(void);

int call_read_line(char* text, int max_characters, int x, int y,
	unsigned long timeout)
{
	legacy_u16 length;
	legacy_u16 trim_index;
	legacy_u16 max_pixels;
	int result;

	mouse_draw_opaque_check();
	max_pixels = LEGACY_U16_WRAP_ADD(
		LEGACY_U16_WRAP_MUL(max_characters, 9U), 9U);
	result = read_line(2, text, 0, max_characters, max_pixels, x, y,
		&kb_shift_checking2, timeout);
	mouse_draw_transparent_check();

	length = (legacy_u16)strlen(text);
	trim_index = LEGACY_U16_WRAP_SUB(length, 1U);
	while (text[trim_index] == ' ')
		trim_index = LEGACY_U16_WRAP_SUB(trim_index, 1U);
	text[LEGACY_U16_WRAP_ADD(trim_index, 1U)] = 0;
	return result;
}

int sprite_blit_to_video(struct SPRITE far* sprite, int mode)
{
	int result;
	unsigned int phase;

	sprite_copy_2_to_1_2();
	mouse_draw_opaque_check();
	if ((legacy_u16)mode == 0xFFFEU) {
		sprite_putimage(sprite->sprite_bitmapptr);
		mouse_draw_transparent_check();
		return 0;
	}

	result = 0;
	for (phase = 0; phase < 4U; ++phase) {
		result = input_do_checking((int)timer_get_delta_alt());
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

int read_line(int flags, char* text, int initial_key, int max_characters,
	int max_pixels, int x, int y, void (far* callback)(void),
	unsigned long timeout)
{
	legacy_u8 input_flags;
	legacy_u16 key;
	legacy_u16 length;
	legacy_u16 index;
	legacy_u16 old_cursor_state;
	int insert_mode;
	int first_key;

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
			text[index] = (char)(legacy_u8)key;
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
	static const char space[] = " ";
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

int audioresource_get_chunk_index(int extra_name_stride, int chunk_count,
	const char* requested_name, const legacy_u8 far* chunk_names)
{
	const char far* requested_name_far;
	const legacy_u8 far* candidate;
	legacy_u16 names_offset;
	legacy_u16 names_segment;
	legacy_s16 count;
	legacy_s16 index;

	count = LEGACY_S16_FROM_BITS(chunk_count);
	if (count <= 0)
		return -1;
	requested_name_far = (const char far*)MK_FP(
		FP_SEG(requested_name), FP_OFF(requested_name));
	names_offset = (legacy_u16)FP_OFF(chunk_names);
	names_segment = (legacy_u16)FP_SEG(chunk_names);
	for (index = 0; index < count;
		index = LEGACY_S16_WRAP_ADD(index, 1)) {
		candidate = (const legacy_u8 far*)MK_FP(
			names_segment, names_offset);
		if (audioresource_compare_chunknames(0,
			(const char far*)candidate, requested_name_far, 4))
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

void far* audioresource_find(void far* resource, const char* chunk_name)
{
	legacy_u8 far* bytes;
	legacy_u8 far* offset_entry;
	legacy_u16 resource_offset;
	legacy_u16 resource_segment;
	legacy_u16 chunk_count;
	legacy_u16 table_offset;
	legacy_u16 relative_offset;
	legacy_u16 result_offset;
	int chunk_index;

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
	char name[4];
	legacy_u16 pointer_offset;
	legacy_u16 pointer_segment;
	unsigned int count;
	unsigned int index;
	unsigned int name_offset;

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
	char name[4];
	legacy_u16 relative_offset;
	int chunk_index;

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
	const char* name)
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

void audioresource_copy_n_bytes(const legacy_u8 far* source,
	legacy_u8 far* destination, int size)
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

void audio_op_unk3(int index)
{
	audio_start_indexed_event(index, 0x44U, 0x40U);
}

void audio_op_unk4(int index)
{
	audio_start_indexed_event(index, 0x48U, 0x40U);
}

void audio_function2_wrap(int index)
{
	audio_start_indexed_event(index, 0x38U, 0x64U);
	audio_function2(index);
}

void nopsub_2726C(int index)
{
	audio_start_indexed_event(index, 0x30U, 0x40U);
	audio_function2(index);
}

void nopsub_272B0(int index)
{
	audio_start_indexed_event(index, 0x34U, 0x40U);
	audio_function2(index);
}

static void audio_start_secondary_event(int index,
	unsigned int resource_field)
{
	unsigned int offset;
	unsigned int rate;
	int channel;
	void far* resource;

	offset = LEGACY_U16_WRAP_MUL(index, 0x4CU);
	channel = LEGACY_S16_FROM_BITS(
		LEGACY_READ_U16_LE(audiotimers + offset + 0x16U));
	if (channel != -1)
		audio_init_chunk2(channel);

	rate = LEGACY_READ_U16_LE(audiotimers + offset + 4U) >> 4;
	resource = audio_read_far_pointer(
		audiotimers + offset + resource_field);
	channel = audio_check_flag(resource, -1, 0x40U, rate);
	LEGACY_WRITE_U16_LE(audiotimers + offset + 0x16U, channel);
	audiotimers[offset + 0x1AU] = 1;
}

void audio_op_unk5(int index)
{
	audio_start_secondary_event(index, 0x3CU);
}

void audio_op_unk6(int index)
{
	audio_start_secondary_event(index, 0x40U);
}

void sub_374DE(int channel)
{
	if (channel > -1) {
		byte_45D9A[channel] = 0;
		audio_init_chunk2(channel);
	}
}

void sub_38156(int index)
{
	unsigned int offset;

	offset = LEGACY_U16_WRAP_MUL(index, 0x2EU);
	LEGACY_WRITE_U16_LE(unk_45A26 + offset + 0x0CU, 1);
	LEGACY_WRITE_U16_LE(unk_45A26 + offset + 0x0EU, 0);
}

int sub_37868(int value)
{
	int channel;

	for (channel = 0; channel < (unsigned int)byte_44290; channel++)
		audio_unk2(channel, value);

	return channel;
}

int nopsub_37898(int value)
{
	byte_45950 = (unsigned char)value;
	return sub_37868(value);
}

void sub_37C38(int value)
{
	word_4063C = value;
}

unsigned int nopsub_378AE(int channel)
{
	return (unsigned int)byte_44D06[(unsigned int)channel];
}

unsigned int nopsub_378BC(int channel)
{
	return (unsigned int)byte_44ACA[(unsigned int)channel];
}

void audio_unk3(unsigned char flags, int channel)
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
	int i;

	sdgame2ptr = file_load_resource(8, "sdgame2");
	locate_many_resources(
		sdgame2ptr,
		"ex01ex02ex03leftrigh",
		sdgame2shapes);
	for (i = 0; i < 3; i++)
		sdgame2_widths[i] =
			((struct SHAPE2D far*)sdgame2shapes[i])->s2d_width;
}

void load_skybox(char skybox_index)
{
	unsigned int minimum;
	unsigned int maximum;

	if (((unsigned char)skybox_index & 8U) == 0) {
		if (byte_3B8F6 != 0 &&
			(unsigned char)skybox_index == (unsigned char)byte_46167)
			return;

		unload_skybox();
		byte_46167 = skybox_index;
		byte_3B8F6 = 1;
		skybox_res_ofs = file_load_shape2d_fatal(
			skybox_resource_names[(signed char)skybox_index]);
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

void init_carstate_from_simd(struct CARSTATE* playerstate, struct SIMD* simd, char transmission, long posX, long posY, long posZ, short track_angle)
{
	int i;
	struct VECTOR whlPos;

	playerstate->car_posWorld1.lx = posX;
	playerstate->car_posWorld2.lx = posX;
	playerstate->car_posWorld1.ly = posY + 512;
	playerstate->car_posWorld2.ly = posY;
	playerstate->car_posWorld1.lz = posZ;
	playerstate->car_posWorld2.lz = posZ;
	
	playerstate->car_rotate.x = track_angle;
	playerstate->car_rotate.y = 0;
	playerstate->car_rotate.z = 0;
	playerstate->car_36MwhlAngle = 0;
	playerstate->car_pseudoGravity = 0;
	playerstate->car_steeringAngle = 0;
	playerstate->car_is_braking = 0;
	playerstate->car_is_accelerating = 0;
	playerstate->car_currpm = simd->idle_rpm;
	playerstate->car_lastrpm = playerstate->car_currpm;
	playerstate->car_idlerpm2 = playerstate->car_currpm;
	playerstate->car_current_gear = 1;
	playerstate->car_speeddiff = 0;
	playerstate->car_speed = 0;
	playerstate->car_speed2 = 0;
	playerstate->car_lastspeed = 0;
	playerstate->car_gearratio = simd->gear_ratios[1];
	playerstate->car_gearratioshr8 = playerstate->car_gearratio >> 8;
	playerstate->car_knob_x = simd->knob_points[1].px;
	playerstate->car_knob_x2 = playerstate->car_knob_x;
	playerstate->car_knob_y = simd->knob_points[1].py;
	playerstate->car_knob_y2 = playerstate->car_knob_y;
	playerstate->car_angle_z = 0;
	playerstate->car_40MfrontWhlAngle = 0;
	playerstate->field_42 = 0;
	playerstate->field_48 = 0;
	playerstate->car_trackdata3_index = 0;
	playerstate->car_sumSurfFrontWheels = 2;
	playerstate->car_sumSurfRearWheels = 2;
	playerstate->car_sumSurfAllWheels = 4;
	playerstate->car_demandedGrip = 0;
	playerstate->car_surfacegrip_sum = 1000;

	whlPos.x = posX / 64;
	whlPos.y = posY / 64;
	whlPos.z = posZ / 64;

	for (i = 0; i < 4; ++i) {
		playerstate->car_surfaceWhl[i] = 1;
		playerstate->car_rc1[i] = 0;
		playerstate->car_rc2[i] = 0;
		playerstate->car_rc3[i] = 0;
		playerstate->car_rc4[i] = 0;
		playerstate->car_rc5[i] = 0;

		playerstate->car_whlWorldCrds1[i] = whlPos;
		playerstate->car_whlWorldCrds2[i] = whlPos;
	}

	playerstate->car_engineLimiterTimer = 0;
	playerstate->car_slidingFlag = 0;
	playerstate->field_C8 = 0;
	playerstate->car_crashBmpFlag = 0;
	playerstate->car_changing_gear = 0;
	playerstate->car_fpsmul2 = 0;
	playerstate->car_transmission = transmission;
	playerstate->field_CD = 0;
	playerstate->field_CE = 0;
	playerstate->field_CF = 1;
}

void init_game_state(short arg)
{
	int i, tmpcol, tmprow;

	if (arg == -1) {
		elapsed_time1 = 0;

		for (i = 0; i < RST_CVX_NUM; ++i) {
			((struct GAMESTATE far*)cvxptr)[i].field_3F4 = 0;
		}
	}
	
	if (framespersec == 10) {
		steerWhlRespTable_ptr = &steerWhlRespTable_10fps;
	}
	else {
		steerWhlRespTable_ptr = &steerWhlRespTable_20fps;
	}
	
	word_45A00 = framespersec * 30;
	word_4499C = 100 / framespersec;

	if (arg != -3) {
		init_unknown();

		state.field_3F4 = 1;
		state.game_frames_per_sec = 1;
		state.game_inputmode = 0;
		state.game_3F6autoLoadEvalFlag = 0;
		state.game_frame_in_sec = 0;
		state.field_2F4 = 0;
		state.field_3F7[0] = 0;
		state.field_3F7[1] = 0;

		for (i = 0; i < 48; ++i) {
			state.field_3FA[i] = 0;
		}
				
		for (i = 0; i < 24; ++i) {
			state.field_38E[i] = 0;
		}

		state.game_vec1[0].x =
			  multiply_and_scale(sin_fast(track_angle + 0x300),  512)
			+ multiply_and_scale(sin_fast(track_angle + 0x200), 4096)
			+ ((short)startcol2 << 10);

		state.game_vec1[0].y = hillHeightConsts[hillFlag] + 960;

		state.game_vec1[0].z =
			  multiply_and_scale(cos_fast(track_angle + 0x300),  512)
			+ multiply_and_scale(cos_fast(track_angle + 0x200), 4096)
			+ trackpos[startrow2];

		state.game_vec1[1] = state.game_vec1[0];
		state.game_vec3 = state.game_vec1[0];
		state.game_vec4 = state.game_vec1[0];
		
		state.game_travDist = 0;
		state.game_frame = 0;
		state.game_total_finish = 0;
		state.field_144 = 0;
		state.game_pEndFrame = 0;
		state.game_oEndFrame = 0;
		state.game_penalty = 0;
		state.game_impactSpeed = 0;
		state.game_topSpeed = 0;
		state.game_jumpCount = 0;

		// Init player car.
		tmpcol =
			  multiply_and_scale(sin_fast(track_angle + 0x200), 210)
			+ multiply_and_scale(sin_fast(track_angle + 0x100),  36);
		
		tmprow =
			  multiply_and_scale(cos_fast(track_angle + 0x200), 210)
			+ multiply_and_scale(cos_fast(track_angle + 0x100),  36);

		init_carstate_from_simd(
			&state.playerstate,
			&simd_player,
			gameconfig.game_playertransmission,
			(long)(trackcenterpos2[startcol2] + tmpcol) * 64L,
			(long)hillHeightConsts[hillFlag] * 64L,
			(long)(trackcenterpos[startrow2] + tmprow) * 64L,
			-track_angle);

		state.field_2F2 = 0;
		state.field_45D = 0;
		state.field_45E = 0;
		state.field_45B = 0;
		state.field_45C = 0;
		
		state.game_startcol  = startcol2;
		state.game_startcol2 = startcol2;
		state.game_startrow  = startrow2;
		state.game_startrow2 = startrow2;

		if (arg != -2) {
			sub_18D60(
				state.playerstate.car_trackdata3_index,
				&state.playerstate.car_vec_unk3,
				state.playerstate.field_CE,
				0);
			
			state.playerstate.field_CE++;
		}

		// Init opponent car.
		tmpcol =
			  multiply_and_scale(sin_fast(track_angle + 0x200), 210)
			+ multiply_and_scale(sin_fast(track_angle + 0x300),  36);
		
		tmprow =
			  multiply_and_scale(cos_fast(track_angle + 0x200), 210)
			+ multiply_and_scale(cos_fast(track_angle + 0x300),  36);

		init_carstate_from_simd(
			&state.opponentstate,
			&simd_opponent,
			1,
			(long)(trackcenterpos2[startcol2] + tmpcol) * 64L,
			(long)hillHeightConsts[hillFlag] * 64L,
			(long)(trackcenterpos[startrow2] + tmprow) * 64L,
			-track_angle);

		if (gameconfig.game_opponenttype && arg != -2) {
			sub_18D60(
				trackdata3[state.opponentstate.car_trackdata3_index], // TODO: Verify
				&state.opponentstate.car_vec_unk3,
				state.opponentstate.field_CE,
				&state.field_3F9); // TODO: Verify
		
			state.opponentstate.field_CE++;
		}

		state.field_42A = 0;
	}
}

void restore_gamestate(unsigned short frame)
{
	unsigned short curframe;

	if (frame == 0 && elapsed_time1 == 0) {
		init_game_state(0);
	}
	
	curframe = frame / word_45A00;
	
	if (curframe == RST_CVX_NUM) {
		curframe--;
	}
	
	// Find last gamestate in cvx.
	if (frame >= state.game_frame) {
		while (1) {
			if (curframe * word_45A00 <= state.game_frame) {
				return;
			}
			else if (((struct GAMESTATE far*)cvxptr)[curframe].field_3F4 != 0) {
				break;
			}
			
			curframe--;
		}
	}
	
	// Copy last gamestate from cvx.
	state = ((struct GAMESTATE far*)cvxptr)[curframe];

	init_kevinrandom(state.kevinseed);
	elapsed_time2 = state.game_frame;
}


void update_gamestate() {
	char var_carInputByte;

	var_carInputByte = td16_rpl_buffer[state.game_frame];
	if (var_carInputByte != 0) {
		state.game_inputmode = 1;
	}
	
	if ((state.game_frame % word_45A00) == 0) {
		get_kevinrandom_seed(state.kevinseed);
	
		fmemcpy(&cvxptr[state.game_frame / word_45A00], MK_FP(FP_SEG(&state), FP_OFF(&state)), sizeof(struct GAMESTATE));
	}

	state.game_frame++;
	if (state.game_3F6autoLoadEvalFlag != 0 && state.game_frame_in_sec < state.game_frames_per_sec) {
		state.game_frame_in_sec++;
		if (state.game_frame_in_sec == state.game_frames_per_sec && byte_449DA == 0) {
			if (state.playerstate.car_crashBmpFlag == 1 && state.playerstate.car_speed2 != 0) {
				state.game_frames_per_sec++;
			} else if (game_replay_mode == 0) {
				byte_449DA = 1;
			}
		}
	}

	if (state.game_inputmode != 0) {
		
		player_op(var_carInputByte);
		
		if (gameconfig.game_opponenttype != 0) {
			opponent_op();
		}

		sub_2298C();
		if (state.field_42A != 0) {
			sub_19BA0();
		}

		audio_carstate();

	} else if (game_replay_mode == 1) {
		// if paused
		audio_carstate();
		if (byte_4393C != 0) {
			if (word_44DCA < 0x1C2) {
				word_44DCA += 8;
			}

			if (byte_4393C == 1 && word_44DCA > 0x180) {
				byte_4393C++;
			}

			if (byte_4393C == 2) {
				if (  multiply_and_scale(cos_fast(track_angle), trackcenterpos[startrow2] - (state.playerstate.car_posWorld1.lz >> 6)) 
					+ multiply_and_scale(sin_fast(track_angle), trackcenterpos2[startcol2] - (state.playerstate.car_posWorld1.lx >> 6)) <= 0xE4) {
					if (state.playerstate.car_speed != 0) {
						player_op(2);
					} else {
						byte_4393C = 0;
					}
				} else 
				if (state.playerstate.car_speed < 0x500) {
					player_op(1);
				} else {
					player_op(0);
				}
			}
		}
	}
}

extern char aCarcoun[];
extern void far* engptr;
extern void far* eng1ptr;
extern void far* fontledresptr;
extern void far* sdgameresptr;
extern void far* wallptr;
extern void far* planptr;
extern char unk_3E7FC[];
extern char byte_42D26;
extern char byte_42D2A;
extern char unk_3E82C[];
extern char gnam_string[]; // 40 bytes
extern char gsna_string[]; // 5 bytes

void setup_aero_trackdata(void far* carresptr, int is_opponent) {
	int i;
	if (is_opponent == 0) {
		fmemcpy(MK_FP(FP_SEG(&simd_player), FP_OFF(&simd_player)), locate_shape_alt(carresptr, "simd"), sizeof(struct SIMD));
		simd_player.aerorestable = td04_aerotable_pl;
		// Maximum speed is 40h
		// Division by 2^9.
		// 2^8 shifts one fullbyte, and it is known there is a 1/2 factor in FDrag.
		for (i = 0; i < 0x40; i++) {
			td04_aerotable_pl[i] = ((long)simd_player.aero_resistance * (long)i * (long)i) >> 9;
		}

		copy_string(gnam_string, locate_shape_alt(carresptr, "gnam"));
	} else {
		fmemcpy(MK_FP(FP_SEG(&simd_opponent), FP_OFF(&simd_opponent)), locate_shape_alt(carresptr, "simd"), sizeof(struct SIMD));
		simd_opponent.aerorestable = td05_aerotable_op;

		for (i = 0; i < 0x40; i++) {
			td05_aerotable_op[i] = ((long)simd_opponent.aero_resistance * (long)i * (long)i) >> 9;
		}
		copy_string(gsna_string, locate_shape_alt(carresptr, "gsna"));
	}
}

extern int audio_init_engine(int, void far*, void far*, void far*);

int setup_player_cars(void) {
	void far* carresptr;
	unsigned long var_8;

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
	if (idle_expired == 0) {
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

void shape2d_render_bmp_as_mask(void far* data);
void shape2d_op_unk4(unsigned short offset, unsigned short segment);

void run_game(void) {
	int var_16[2];
	int var_12, var_E, var_C;
	struct RECTANGLE var_rect;
	int var_2;
	int regsi;

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
				*(char*)&word_45D3E = 0; // byte ptr!
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
				if ((byte_3B8F2 != 0 || byte_3FE00 != 0) && game_replay_mode == 0) {
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
				audio_unk();
				regsi = show_dialog(2, 1, locate_text_res(gameresptr, "rbf"), -1, -1, dialogarg2, 0, 0);
				if (regsi == -1)
					regsi = 0;

				sub_372F4();
				word_3F88E = 0;
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
					var_12 = kb_get_char();
					if (var_12 != 0) {
						handle_ingame_kb_shortcuts(var_12);
					}

				} while (var_12 == 0x4800 || var_12 == 0x4B00 || var_12 == 0x4D00 || var_12 == 0x5000);

				if (game_replay_mode == 1) {
					mouse_get_state(&mouse_butstate, &mouse_xpos, &mouse_ypos);
					if (((mouse_butstate & 3) != 0) || ((get_kb_or_joy_flags() & 0x30) != 0)) {
						game_replay_mode = 0;
						byte_4393C = 0;
						framespersec = framespersec2;
						gameconfig.game_framespersec = framespersec2;
						init_game_state(-1);
					}
				}

			} else {
				if (kb_get_char() != 0 || byte_449DA != 0 || get_kb_or_joy_flags() != 0) {
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
			*(char*)&word_45D3E = 1;
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

		*(char*)&word_45D3E = 0; // byte ptr 
		mouse_minmax_position(0);
		remove_frame_callback();
		free_player_cars();
	}

	waitflag = 0x64;
	check_input();
	show_waiting();

	return ;
}

void init_div0(void)
{
	// Use original code until we can link with a libc for intdosx().
	ported_init_div0_();

	/*
	union REGS inregs, outregs;
	struct SREGS segregs;
	
	// Get current division by zero interrupt.
	inregs.h.ah = 0x35;
	inregs.h.al = 0;
	intdosx(&inregs, &outregs, &segregs);
	
	old_intr0_handler = MK_FP(segregs.es, outregs.x.bx);
	
	// Set division by zero interrupt.
	inregs.h.ah = 0x25;
	inregs.h.al = 0;
	segregs.ds  = FP_SEG(intr0_handler);
	inregs.x.dx = FP_OFF(intr0_handler);
	intdosx(&inregs, &outregs, &segregs);
	*/
}

void copy_material_list_pointers(void* clrlist, void* clrlist2, void* patlist, void* patlist2, unsigned short videoConst)
{
	material_clrlist_ptr_cpy = clrlist;
	material_clrlist2_ptr_cpy = clrlist2;
	material_patlist_ptr_cpy = patlist;
	material_patlist2_ptr_cpy = patlist2;
	someZeroVideoConst = videoConst;
}

void init_main(int argc, char* argv[])
{
	unsigned int i, j;
	unsigned char argmode4, argnosound, argnounknown;
	unsigned long timerdelta1, timerdelta2, timerdelta3;
	struct POINT2D tmppoint;
	struct RECTANGLE tmprect;

	// Keyboard
	kb_init_interrupt();
	kb_shift_checking2();
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
	video_set_mode_13h();
	if (argmode4) {
		video_set_mode4();
	}

	timer_setup_interrupt();

	sprite_copy_2_to_1_clear();

	mouse_init(0x0140, 0x00C8);

	// Audio driver.
	if (audio_load_driver(audiodriverstring, 0, 0)) {
		audio_stop_unk();
		libsub_quit_to_dos_alt(1);
	}
	
	if (argnosound) {
		audio_toggle_flag2();
		audio_toggle_flag6();
	}
	
	set_criterr_handler(&do_dea_textres);
	
	load_palandcursor();
	
	// Timing measures.
	sprite_copy_2_to_1();
	sprite_set_1_size(0, 320, 0, 120);

	timer_get_delta_alt();
	for (i = 0; i < 15; ++i) {
		ported_sprite_clear_1_color_(0); // the c impl is too slow/wrong and produces faulty timing values
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

int stuntsmain2(int argc, char* argv[]) {
	int result;
	char far* textresptr;
	int carposangle;
	struct SPRITE far* var42wnd;
	int counter;
	int inch;
	int shapeindex;

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
	audio_stop_unk();
	audiodrv_atexit();
	kb_exit_handler();
	kb_shift_checking1();
	video_set_mode7();
	
	fatal_error("err %i", inch);

	return 0;
}

int stuntsmainimpl(int argc, char* argv[]) {

	int i, result;
	int regax, regsi;
	char var_A;
	char far* trkptr;
	char far* textresptr;
	
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
				audio_stop_unk();
				audiodrv_atexit();
				kb_exit_handler();
				kb_shift_checking1();
				video_set_mode7();
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

