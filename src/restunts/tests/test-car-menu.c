#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../c/fileio.h"
#include "../c/legacy.h"
#include "../c/memmgr.h"
#include "../c/menu_internal.h"
#include "../c/platform.h"
#include "../c/shape2d.h"
#include "../c/shape3d.h"
#include "../c/ui_text.h"
#include "../c/timing.h"
#include "../c/game_input.h"
#include "../c/ui_input.h"
#include "../c/ui_dialog.h"
#include "../c/car_speed.h"
#include "../c/video_frame.h"
#include "../c/car_model.h"
#include "../c/scene_resources.h"
#include "../c/car_resources.h"
#include "../c/menu_common.h"
#include "../c/externs.h"
#include "../c/keyboard.h"

#undef printf

struct SPRITE *render_window_sprite;

static uint64_t trace_hash = UINT64_C(1469598103934665603);
static unsigned int scenario, frame_index, file_index, allocation_index, sprite_index;
static unsigned int acceleration_step;
static legacy_u8 resource_bytes[64][32];
static struct SHAPE2D fixture_shapes[4];
static struct SPRITE fixture_sprites[4];
static const legacy_s8 *fixture_files[] = {(const legacy_s8 *)"CARVETT.RES",
										   (const legacy_s8 *)"CARANSX.RES",
										   (const legacy_s8 *)"CARCOUN.RES"};

static void trace_word(legacy_u16 value)
{
	trace_hash = (trace_hash ^ (value & 255U)) * UINT64_C(1099511628211);
	trace_hash = (trace_hash ^ (value >> 8)) * UINT64_C(1099511628211);
}

static void trace_text(const legacy_s8 *text)
{
	if (text == 0) {
		trace_word(65535U);
		return;
	}
	while (*text != 0) {
		trace_word((legacy_u8)*text++);
	}
	trace_word(0);
}

static void trace_pointer(const void *pointer)
{
	if (pointer == 0) {
		trace_word(0);
		return;
	}
	for (unsigned int i = 0; i < 64U; i++) {
		if (pointer == resource_bytes[i]) {
			trace_word(100U + i);
			return;
		}
	}
	for (unsigned int i = 0; i < 4U; i++) {
		if (pointer == &fixture_sprites[i]) {
			trace_word(200U + i);
			return;
		}
		if (pointer == &fixture_shapes[i]) {
			trace_word(300U + i);
			return;
		}
	}
	trace_word(1);
}

static void trace_rect(const struct RECTANGLE *rect)
{
	trace_word(rect->left);
	trace_word(rect->right);
	trace_word(rect->top);
	trace_word(rect->bottom);
}

legacy_s16 _strcmp(const legacy_s8 *dest, const legacy_s8 *src)
{
	while (*dest != 0 && *dest == *src) {
		dest++;
		src++;
	}
	return (legacy_s16)((legacy_u8)*dest - (legacy_u8)*src);
}

legacy_s8 *_strcpy(legacy_s8 *dest, const legacy_s8 *src)
{
	legacy_s8 *result = dest;
	do {
		*dest++ = *src;
	} while (*src++ != 0);
	return result;
}

void draw_button(legacy_s8 *text, legacy_s16 x, legacy_s16 y, legacy_s16 width, legacy_s16 height,
				 legacy_s16 top_color, legacy_s16 bottom_color, legacy_s16 fill_color,
				 legacy_s16 font_color)
{
	trace_word(1002);
	trace_text(text);
	trace_word((legacy_u16)x);
	trace_word((legacy_u16)y);
	trace_word((legacy_u16)width);
	trace_word((legacy_u16)height);
	trace_word((legacy_u16)top_color);
	trace_word((legacy_u16)bottom_color);
	trace_word((legacy_u16)fill_color);
	trace_word((legacy_u16)font_color);
}

void ensure_file_exists(legacy_s16 unused)
{
	trace_word(1003);
	trace_word((legacy_u16)unused);
}

const legacy_s8 *file_combine_and_find(const legacy_s8 *dir, const legacy_s8 *name,
									   const legacy_s8 *ext)
{
	trace_word(1004);
	trace_text(dir);
	trace_text(name);
	trace_text(ext);
	file_index = 0;
	if (scenario % 17U == 0U) {
		return 0;
	}
	return fixture_files[file_index++];
}

const legacy_s8 *file_find_next_alt(void)
{
	trace_word(1005);

	if (file_index < 3U) {
		return fixture_files[file_index++];
	}
	return 0;
}

void *file_load_resfile(const legacy_s8 *filename)
{
	trace_word(1006);
	trace_text(filename);
	assert(allocation_index < 64U);
	return resource_bytes[allocation_index++];
}

void *file_load_shape2d_fatal(const legacy_s8 *shapename)
{
	trace_word(1007);
	trace_text(shapename);
	assert(allocation_index < 64U);
	return resource_bytes[allocation_index++];
}

void font_draw_text(const legacy_s8 *text, legacy_s16 x, legacy_s16 y)
{
	trace_word(1008);
	trace_text(text);
	trace_word((legacy_u16)x);
	trace_word((legacy_u16)y);
}

void font_set_colors(legacy_s16 color, legacy_s16 background_color)
{
	trace_word(1009);
	trace_word((legacy_u16)color);
	trace_word((legacy_u16)background_color);
}

void font_set_fontdef(void)
{
	trace_word(1010);
}

void font_set_fontdef2(void *data)
{
	trace_word(1011);
	trace_pointer(data);
}

void init_game_state(legacy_s16 initialization_mode)
{
	trace_word(1012);
	trace_word((legacy_u16)initialization_mode);
	acceleration_step = 0;
	state.playerstate.car_rev_speed = 0;
}

legacy_s16 input_checking(legacy_s16 frame_delta)
{
	trace_word(1013);
	trace_word((legacy_u16)frame_delta);
	static const legacy_u16 keys[] = {0,		 KEY_DOWN, KEY_ENTER, KEY_ENTER, KEY_DOWN,
									  KEY_ENTER, KEY_DOWN, KEY_ENTER, KEY_DOWN,	 KEY_ENTER,
									  KEY_UP,	 KEY_UP,   KEY_UP,	  KEY_UP,	 KEY_ENTER};
	assert(frame_index < 100U);
	if (frame_index >= 15U) {
		return KEY_ENTER;
	}
	return keys[frame_index];
}

legacy_s8 *locate_shape_fatal(legacy_s8 *data, const legacy_s8 *name)
{
	trace_word(1014);
	trace_pointer(data);
	trace_text(name);
	return (legacy_s8 *)&fixture_shapes[0];
}

legacy_s8 *locate_text_res(legacy_s8 *data, const legacy_s8 *name)
{
	trace_word(1015);
	trace_pointer(data);
	trace_text(name);
	if (_strcmp(name, car_description_id) == 0) {
		return (legacy_s8 *)"First]Second]]Third]";
	}
	return (legacy_s8 *)name;
}

legacy_s16 menu_animate_button_highlight(legacy_s16 item_index, const struct BUTTON_AREA *buttons,
										 legacy_s16 second_color, legacy_s16 first_color)
{
	trace_word(1016);
	trace_word((legacy_u16)item_index);
	trace_pointer(buttons);
	trace_word((legacy_u16)second_color);
	trace_word((legacy_u16)first_color);
	return (legacy_s16)(1U + frame_index % 3U);
}

void menu_reset_animation_timers(void)
{
	trace_word(1017);
}

void menu_update_idle_counter(legacy_u16 elapsed, legacy_s16 limit)
{
	trace_word(1018);
	trace_word((legacy_u16)elapsed);
	trace_word((legacy_u16)limit);
	if (scenario % 5U == 0U && frame_index >= 7U) {
		idle_expired = 1;
	}
}

void *mmgr_free(legacy_s8 *ptr)
{
	trace_word(1019);
	trace_pointer(ptr);
	return 0;
}

void mouse_draw_opaque_check(void)
{
	trace_word(1020);
}

void mouse_draw_transparent_check(void)
{
	trace_word(1021);
}

legacy_s16 mouse_multi_hittest(legacy_s16 count, const struct BUTTON_AREA *buttons)
{
	trace_word(1022);
	trace_word((legacy_u16)count);
	trace_pointer(buttons);
	for (unsigned int i = 0; i < (unsigned int)count; i++) {
		trace_word(buttons[i].x1);
		trace_word(buttons[i].x2);
		trace_word(buttons[i].y1);
		trace_word(buttons[i].y2);
	}
	legacy_s16 result = -1;
	if (frame_index == 10U && scenario % 3U == 0U) {
		result = 4;
	}
	if (frame_index >= 15U) {
		result = 0;
	}
	frame_index++;
	return result;
}

legacy_s16 polarAngle(legacy_s16 z, legacy_s16 y)
{
	trace_word(1023);
	trace_word((legacy_u16)z);
	trace_word((legacy_u16)y);
	return (legacy_s16)(z + y);
}

legacy_s16 rect_intersect(struct RECTANGLE *r1, struct RECTANGLE *r2)
{
	trace_word(1024);
	trace_rect(r1);
	trace_rect(r2);
	r1->left = r2->left;
	r1->right = r2->right;
	r1->top = r2->top;
	r1->bottom = r2->bottom;
	return 1;
}

void rect_union(struct RECTANGLE *r1, struct RECTANGLE *r2, struct RECTANGLE *outrc)
{
	trace_word(1025);
	trace_rect(r1);
	trace_rect(r2);

	*outrc = *r1;
	if (r2->bottom > outrc->bottom) {
		outrc->bottom = r2->bottom;
	}
}

legacy_u16 select_cliprect_rotate(legacy_s16 angZ, legacy_s16 angX, legacy_s16 angY,
								  struct RECTANGLE *cliprect, legacy_s16 half_scale)
{
	trace_word(1026);
	trace_word((legacy_u16)angZ);
	trace_word((legacy_u16)angX);
	trace_word((legacy_u16)angY);
	trace_rect(cliprect);
	trace_word((legacy_u16)half_scale);
	return 0;
}

void set_projection(legacy_s16 horizontal_fov_degrees, legacy_s16 vertical_fov_degrees,
					legacy_s16 width, legacy_s16 height)
{
	trace_word(1027);
	trace_word((legacy_u16)horizontal_fov_degrees);
	trace_word((legacy_u16)vertical_fov_degrees);
	trace_word((legacy_u16)width);
	trace_word((legacy_u16)height);
}

void setup_aero_trackdata(void *carresptr, legacy_s16 is_opponent)
{
	trace_word(1028);
	trace_pointer(carresptr);
	trace_word((legacy_u16)is_opponent);
}

legacy_u16 shape2d_get_height(const struct SHAPE2D *shape)
{
	trace_word(1029);
	trace_pointer(shape);
	return 12;
}

legacy_u16 shape2d_get_width(const struct SHAPE2D *shape)
{
	trace_word(1030);
	trace_pointer(shape);
	return 30;
}

void shape3d_free_car_shapes(void)
{
	trace_word(1031);
}

void shape3d_load_car_shapes(legacy_s8 *carid, legacy_s8 *opponent_carid)
{
	trace_word(1032);
	trace_text(carid);
	trace_text(opponent_carid);
}

void shape3d_render_queued_primitives(void)
{
	trace_word(1033);
}

legacy_u16 shape3d_transform_and_queue(struct TRANSFORMEDSHAPE3D *instance)
{
	trace_word(1034);
	trace_pointer(instance);
	trace_word(instance->rotvec.z);
	trace_word(instance->material);
	trace_word(instance->ts_flags);
	trace_word(instance->pos.x);
	trace_word(instance->pos.y);
	trace_word(instance->pos.z);
	if (instance->rectptr != 0) {
		instance->rectptr->left = 3;
		instance->rectptr->right = 128;
		instance->rectptr->top = 7;
		instance->rectptr->bottom = 88;
	}
	return 0;
}

legacy_s16 sprite_blit_to_video(struct SPRITE *sprite, legacy_s16 mode)
{
	trace_word(1035);
	trace_pointer(sprite);
	trace_word((legacy_u16)mode);
	return 0;
}

void sprite_clear_shape_alt(struct SHAPE2D *shape, legacy_s16 x, legacy_s16 y)
{
	trace_word(1036);
	trace_pointer(shape);
	trace_word((legacy_u16)x);
	trace_word((legacy_u16)y);
}

void sprite_clear_target(legacy_u8 color)
{
	trace_word(1037);
	trace_word((legacy_u16)color);
}

void sprite_copy_image_at(struct SHAPE2D *shape, legacy_s16 x, legacy_s16 y)
{
	trace_word(1038);
	trace_pointer(shape);
	trace_word((legacy_u16)x);
	trace_word((legacy_u16)y);
}

void sprite_free_wnd(struct SPRITE *wndsprite)
{
	trace_word(1039);
	trace_pointer(wndsprite);
}

struct SPRITE *sprite_make_wnd(legacy_u16 width, legacy_u16 height, legacy_u16 color)
{
	trace_word(1040);
	trace_word((legacy_u16)width);
	trace_word((legacy_u16)height);
	trace_word((legacy_u16)color);
	assert(sprite_index < 4U);
	fixture_sprites[sprite_index].sprite_bitmapptr = &fixture_shapes[sprite_index];
	return &fixture_sprites[sprite_index++];
}

void sprite_putimage(struct SHAPE2D *shape)
{
	trace_word(1041);
	trace_pointer(shape);
}

void sprite_putimage_transparent(struct SHAPE2D *shape, legacy_s16 x, legacy_s16 y)
{
	trace_word(1042);
	trace_pointer(shape);
	trace_word((legacy_u16)x);
	trace_word((legacy_u16)y);
}

void sprite_putpixel_clipped(legacy_s16 x, legacy_s16 y, legacy_s16 color)
{
	trace_word(1043);
	trace_word((legacy_u16)x);
	trace_word((legacy_u16)y);
	trace_word((legacy_u16)color);
}

void sprite_select_mcga_backbuffer(void)
{
	trace_word(1044);
}

void sprite_select_render_window(void)
{
	trace_word(1045);
}

void sprite_select_render_window_and_clear(void)
{
	trace_word(1046);
}

void sprite_select_screen_compat(void)
{
	trace_word(1047);
}

void sprite_set_target_clip_bounds(legacy_u16 left, legacy_u16 right, legacy_u16 top,
								   legacy_u16 bottom)
{
	trace_word(1048);
	trace_word((legacy_u16)left);
	trace_word((legacy_u16)right);
	trace_word((legacy_u16)top);
	trace_word((legacy_u16)bottom);
}

void sprite_shape_to_1_alt(struct SHAPE2D *shape)
{
	trace_word(1049);
	trace_pointer(shape);
}

legacy_u32 timer_get_delta_alt(void)
{
	trace_word(1050);

	return 3;
}

void unload_resource(void *resptr)
{
	trace_word(1051);
	trace_pointer(resptr);
}

void update_car_speed(legacy_s8 input, legacy_s16 car_index, struct CARSTATE *carstate,
					  struct SIMD *simd)
{
	trace_word(1052);
	trace_word((legacy_u16)input);
	trace_word((legacy_u16)car_index);
	trace_pointer(carstate);
	trace_pointer(simd);
	trace_word(carstate->car_transmission);
	acceleration_step++;
	carstate->car_rev_speed =
		(legacy_s16)((scenario % 3U == 0U ? acceleration_step % 64U : acceleration_step) * 256U);
}

static void run_car_case(unsigned int index)
{
	legacy_s8 material = index % 6U;
	legacy_s8 transmission = index % 2U;
	scenario = index;
	frame_index = 0;
	file_index = 0;
	allocation_index = 0;
	sprite_index = 0;
	idle_expired = 0;
	video_uses_page_flipping = index % 2U;
	slow_video_mgmt = index % 3U == 0U;
	framespersec = 10 + index % 3U;
	miscptr = (legacy_s8 *)resource_bytes[63];
	fontnptr = (legacy_s8 *)resource_bytes[62];
	font_glyph_height = 8;
	game3dshapes[PLAYER_CAR_LOW_SHAPE].shape3d_numpaints = 3;
	for (unsigned int i = 0; i < 7U; i++) {
		oppresources[i] = (legacy_s8 *)&fixture_shapes[1];
	}
	trace_word(index);
	legacy_s8 car_id[5] = "COUN";
	run_car_menu(car_id, &material, &transmission, index % 3U == 0U ? 2U : 0U);
	trace_text(car_id);
	trace_word((legacy_u8)material);
	trace_word((legacy_u8)transmission);
	trace_word(framespersec);
	trace_word(idle_expired);
	trace_word(waitflag);
}

int main(void)
{
	for (unsigned int index = 0; index < 102U; index++) {
		run_car_case(index);
	}
	/* Original implementation trace: car discovery and sorting, car changes,
	 * graph rendering, animation phases, navigation, idle exit and resource cleanup. */
	assert(trace_hash == UINT64_C(0x25191d328ccaebd8));
	puts("Car menu interaction snapshots passed (102 scenarios).");
	return 0;
}
