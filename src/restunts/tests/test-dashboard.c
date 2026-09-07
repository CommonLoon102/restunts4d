#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../c/externs.h"
#include "../c/dashboard.h"
#include "../c/shape2d.h"
#include "../c/shape3d.h"
#include "../c/platform.h"
#include "../c/fileio.h"
#include "../c/memmgr.h"
#include "../c/game_input.h"
#undef printf
#undef memcpy
#undef memset

static legacy_u32 trace;
static struct SHAPE2D shapes[40];
static struct SPRITE sprites[3];
static legacy_u16 sprite_count;
static int optional_shapes;
static legacy_s8 resources[2];

static void record(legacy_u16 value)
{
	trace = (trace ^ (value & 255U)) * 16777619UL;
	trace = (trace ^ (value >> 8)) * 16777619UL;
}
static legacy_u16 shape_id(const struct SHAPE2D *shape)
{
	return shape->width;
}
legacy_u16 shape2d_get_width(const struct SHAPE2D *shape)
{
	return shape->width;
}
legacy_u16 shape2d_get_height(const struct SHAPE2D *shape)
{
	return shape->height;
}
legacy_u16 shape2d_get_anchor_x(const struct SHAPE2D *shape)
{
	return shape->centre_x;
}
legacy_u16 shape2d_get_anchor_y(const struct SHAPE2D *shape)
{
	return shape->centre_y;
}
legacy_u16 shape2d_get_pos_x(const struct SHAPE2D *shape)
{
	return shape->position_x;
}
legacy_u16 shape2d_get_pos_y(const struct SHAPE2D *shape)
{
	return shape->position_y;
}
void mouse_draw_opaque_check(void)
{
	record(1);
}
void mouse_draw_transparent_check(void)
{
	record(2);
}
void sprite_select_screen(void)
{
	record(3);
}
void sprite_select_screen_compat(void)
{
	record(4);
}
void sprite_select_mcga_backbuffer(void)
{
	record(5);
}
void shape2d_rle_copy_at_position(struct SHAPE2D *shape)
{
	record(10);
	record(shape_id(shape));
}
void shape2d_rle_copy_position_clipped(struct SHAPE2D *shape)
{
	record(11);
	record(shape_id(shape));
}
void shape2d_render_bmp_as_mask(struct SHAPE2D *shape)
{
	record(12);
	record(shape_id(shape));
}
void shape2d_rle_copy(struct SHAPE2D *shape, legacy_s16 x, legacy_s16 y)
{
	record(20);
	record(shape_id(shape));
	record((legacy_u16)x);
	record((legacy_u16)y);
}
void shape2d_rle_copy_clipped(struct SHAPE2D *shape, legacy_s16 x, legacy_s16 y)
{
	record(21);
	record(shape_id(shape));
	record((legacy_u16)x);
	record((legacy_u16)y);
}
void sprite_copy_image_at(struct SHAPE2D *shape, legacy_s16 x, legacy_s16 y)
{
	record(22);
	record(shape_id(shape));
	record((legacy_u16)x);
	record((legacy_u16)y);
}
void sprite_and_image_at_anchor(struct SHAPE2D *shape, legacy_s16 x, legacy_s16 y)
{
	record(23);
	record(shape_id(shape));
	record((legacy_u16)x);
	record((legacy_u16)y);
}
void sprite_or_image_at_anchor(struct SHAPE2D *shape, legacy_s16 x, legacy_s16 y)
{
	record(24);
	record(shape_id(shape));
	record((legacy_u16)x);
	record((legacy_u16)y);
}
void sprite_clear_shape_alt(struct SHAPE2D *shape, legacy_s16 x, legacy_s16 y)
{
	record(25);
	record(shape_id(shape));
	record((legacy_u16)x);
	record((legacy_u16)y);
}
void sprite_putimage_or(struct SHAPE2D *shape, legacy_u16 x, legacy_u16 y)
{
	record(26);
	record(shape_id(shape));
	record((legacy_u16)x);
	record((legacy_u16)y);
}

void sprite_set_target_clip_bounds(legacy_u16 left, legacy_u16 right, legacy_u16 top,
								   legacy_u16 bottom)
{
	record(30);
	record(left);
	record(right);
	record(top);
	record(bottom);
}
void preRender_line(legacy_u16 x, legacy_u16 y, legacy_u16 x2, legacy_u16 y2, legacy_u16 color)
{
	record(31);
	record(x);
	record(y);
	record(x2);
	record(y2);
	record(color);
}
void shape2d_rle_or_far_pointer(legacy_u16 offset, legacy_u16 segment)
{
	record(32);
	record(offset);
	record(segment);
}
legacy_u16 dos_memory_pointer_offset(const void *pointer)
{
	return shape_id(pointer);
}
legacy_u16 dos_memory_pointer_segment(const void *pointer)
{
	return shape_id(pointer) + 10;
}
struct SPRITE *sprite_make_wnd(legacy_u16 width, legacy_u16 height, legacy_u16 flags)
{
	struct SPRITE *result;
	assert(sprite_count < 3);
	record(33);
	record(width);
	record(height);
	record(flags);
	result = &sprites[sprite_count];
	result->sprite_bitmapptr = &shapes[35 + sprite_count++];
	return result;
}
void sprite_free_wnd(struct SPRITE *sprite)
{
	record(34);
	record(shape_id(sprite->sprite_bitmapptr));
}
void sprite_select_target(struct SPRITE *sprite)
{
	record(35);
	record(shape_id(sprite->sprite_bitmapptr));
}
void *file_load_resource(legacy_s16 type, const legacy_s8 *name)
{
	record(36);
	record(type);
	record(name[4]);
	return &resources[type == FILE_RESOURCE_SHAPE2D ? 1 : 0];
}
void *mmgr_free(legacy_s8 *pointer)
{
	record(37);
	record(pointer == &resources[1]);
	return 0;
}
void locate_many_resources(legacy_s8 *data, const legacy_s8 *names, legacy_s8 **result)
{
	unsigned int count, first, i;
	(void)data;
	first = names == dashboard_wheel_and_instrument_ids ? 0
			: names == dashboard_gear_and_dot_shape_ids ? 10
														: 20;
	count = first == 0 ? 9 : first == 10 ? 6 : 10;
	record(38);
	record(first);
	for (i = 0; i < count; i++) {
		result[i] = (legacy_s8 *)&shapes[first + i];
	}
}
legacy_s8 *locate_shape_nofatal(legacy_s8 *data, const legacy_s8 *name)
{
	unsigned int index;
	(void)data;
	index = name == dashboard_roof_shape_id ? 31 : 32;
	record(39);
	record(index);
	return optional_shapes ? (legacy_s8 *)&shapes[index] : 0;
}
legacy_s8 *locate_shape_fatal(legacy_s8 *data, const legacy_s8 *name)
{
	unsigned int index;
	(void)data;
	index = name == dashboard_background_shape_id ? 30
			: name == dashboard_roof_shape_id	  ? 31
			: name == dashboard_top_shape_id	  ? 32
												  : 33;
	record(40);
	record(index);
	return (legacy_s8 *)&shapes[index];
}
static void capture_cache(unsigned int buffer)
{
	record(dashboard_gear_knob_visible_cache[buffer]);
	record(dashboard_gear_knob_x_cache[buffer]);
	record(dashboard_gear_knob_y_cache[buffer]);
	record(dashboard_wheel_shape_cache[buffer]);
	record(dashboard_steering_position_cache[buffer]);
	record(dashboard_steering_dot_x_cache[buffer]);
	record(dashboard_steering_dot_y_cache[buffer]);
	record(dashboard_speed_index_cache[buffer]);
	record(dashboard_rpm_index_cache[buffer]);
}
/* Full-entry traces cover resource lifetime, both buffers, mouse ordering,
 * cache invalidation, wheel movement, and the 99/100/199/200 digit boundaries. */
static void run_scenario(unsigned int scenario)
{
	static const legacy_u16 speeds[] = {0, 99, 100, 199, 200, 255};
	static const legacy_s16 steering[] = {-88, -80, 0, 80, 88, 0};
	unsigned int i;
	memset(&state, 0, sizeof(state));
	memset(&simd_player, 0, sizeof(simd_player));
	memset(sprites, 0, sizeof(sprites));
	for (i = 0; i < 40; i++) {
		shapes[i].width = i + 1;
		shapes[i].height = i + 2;
		shapes[i].centre_x = 3;
		shapes[i].centre_y = 4;
		shapes[i].position_x = 20 + i;
		shapes[i].position_y = 50 + i;
	}
	for (i = 0; i < sizeof(simd_player.steeringdots); i++) {
		simd_player.steeringdots[i] = 40 + (i % 50);
	}
	for (i = 0; i < sizeof(simd_player.spdpoints); i++) {
		simd_player.spdpoints[i] = 10 + (i % 50);
	}
	for (i = 0; i < sizeof(simd_player.revpoints); i++) {
		simd_player.revpoints[i] = 20 + (i % 50);
	}
	simd_player.spdnumpoints = 20;
	simd_player.revnumpoints = 20;
	simd_player.spdcenter.py = (scenario % 3) - 1;
	simd_player.revcenter.px = 40;
	simd_player.revcenter.py = 80;
	optional_shapes = (scenario / 3) % 2;
	video_uses_page_flipping = (scenario / 6) % 2;
	dashboard_buffer_index = (scenario / 12) % 2;
	frame_buffer_index = (scenario / 24) % 2;
	video_shape_width_scale = 1;
	video_x_alignment_mask = -1;
	height_above_replaybar = 180;
	meter_needle_color = 15;
	memcpy(gameconfig.game_playercarid, "PMIN", 4);
	sprite_count = 0;
	setup_car_shapes(DASHBOARD_OPERATION_LOAD);
	setup_car_shapes(DASHBOARD_OPERATION_REDRAW_STATIC);
	for (i = 0; i < 6; i++) {
		state.playerstate.car_steeringAngle = steering[i];
		state.playerstate.car_rev_speed = speeds[i] << 8;
		state.playerstate.car_currpm = i * 600;
		state.playerstate.car_knob_x = i + 10;
		state.playerstate.car_knob_y = i + 15;
		state.playerstate.car_changing_gear = i % 2;
		state.playerstate.car_gear_change_delay = i % 3;
		full_redraw_frames_remaining = i % 2;
		setup_car_shapes(DASHBOARD_OPERATION_UPDATE);
		capture_cache((legacy_u8)dashboard_buffer_index);
		setup_car_shapes(DASHBOARD_OPERATION_UPDATE);
		capture_cache((legacy_u8)dashboard_buffer_index);
	}
	setup_car_shapes(DASHBOARD_OPERATION_UNLOAD);
	setup_car_shapes(-1);
}
int main(void)
{
	unsigned int scenario;
	trace = 2166136261UL;
	for (scenario = 0; scenario < 48; scenario++) {
		run_scenario(scenario);
	}
	assert(trace == 0x21c8a2f5UL);
	puts("Dashboard snapshots passed (48 scenarios).");
	return 0;
}
