#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../c/externs.h"
#include "../c/frame_internal.h"
#include "../c/shape2d.h"
#include "../c/scene_resources.h"
#include "../c/skybox.h"
#include "../c/projection.h"
#include "../c/menu_internal.h"
#include "../c/track_objects.h"
#include "../c/platform.h"
#include "../c/fileio.h"
#include "../c/memmgr.h"
#include "../c/game_input.h"
#include "../c/video_frame.h"
#include "../c/shape3d.h"

#undef strcmp

struct SPRITE *render_window_sprite;

/* Draw-call traces cover the intro scene and full preview traversal. Geometry
 * and rectangle math use production code; rendering, resource and timer endpoints are stubbed.
 * Lifecycle cases also cover timing, camera phases, cancellation and cleanup. */
static legacy_u32 trace_hash;
static unsigned queued_count, flush_count, pixel_count;
static struct SHAPE2D sky_images[4];
static legacy_u8 elements[900], terrain[900];

static void record_word(legacy_u16 value)
{
	trace_hash = (trace_hash ^ value) * 16777619UL;
}

static void record_rect(const struct RECTANGLE *rect)
{
	record_word(rect->left);
	record_word(rect->right);
	record_word(rect->top);
	record_word(rect->bottom);
}

static legacy_u16 shape_id(const struct SHAPE3D *shape)
{
	if (shape == NULL) {
		return 0;
	}
	if (shape == &logoshape) {
		return 131;
	}
	if (shape == &logo2shape) {
		return 132;
	}
	if (shape == &bravshape) {
		return 133;
	}
	for (unsigned index = 0; index < 130; index++) {
		if (shape == &game3dshapes[index]) {
			return index + 1;
		}
	}
	abort();
}

void fatal_error(const legacy_s8 *format, ...)
{
	(void)format;
	abort();
}

void sprite_set_target_clip_bounds(legacy_u16 left, legacy_u16 right, legacy_u16 top,
								   legacy_u16 bottom)
{
	record_word(1);
	record_word(left);
	record_word(right);
	record_word(top);
	record_word(bottom);
}

void sprite_clear_target(legacy_u8 color)
{
	record_word(2);
	record_word(color);
}

void sprite_copy_image_at(struct SHAPE2D far *shape, legacy_s16 x, legacy_s16 y)
{
	record_word(3);
	record_word((legacy_u16)(shape - sky_images));
	record_word(x);
	record_word(y);
}

void sprite_putpixel_clipped(legacy_s16 x, legacy_s16 y, legacy_s16 color)
{
	record_word(4);
	record_word(x);
	record_word(y);
	record_word(color);
	pixel_count++;
}

legacy_u16 select_cliprect_rotate(legacy_s16 z, legacy_s16 x, legacy_s16 y, struct RECTANGLE *clip,
								  legacy_s16 half_scale)
{
	record_word(5);
	record_word(z);
	record_word(x);
	record_word(y);
	record_word(half_scale);
	record_rect(clip);
	mat_temp = *mat_rot_zxy(z, x, y, MATRIX_ROTATION_ORDER_ZXY);
	return 0;
}

legacy_u16 shape3d_transform_and_queue(struct TRANSFORMEDSHAPE3D *shape)
{
	legacy_u16 id = shape_id(shape->shapeptr);

	record_word(6);
	record_word(id);
	record_word(shape->pos.x);
	record_word(shape->pos.y);
	record_word(shape->pos.z);
	record_word(shape->rotvec.x);
	record_word(shape->rotvec.y);
	record_word(shape->rotvec.z);
	record_word(shape->culling_distance);
	record_word(shape->ts_flags);
	record_word(shape->material);
	struct POINT2D corner;
	if ((shape->ts_flags & 8) != 0) {
		assert(shape->rectptr != NULL);
		record_rect(shape->rectptr);
		corner.px = (legacy_s16)(id % 7 * 30 - 20);
		corner.py = (legacy_s16)(id % 5 * 20 - 10);
		rect_adjust_from_point(&corner, shape->rectptr);
		corner.px += 40;
		corner.py += 30;
		rect_adjust_from_point(&corner, shape->rectptr);
	}
	queued_count++;
	return 0;
}

void shape3d_render_queued_primitives(void)
{
	record_word(7);
	flush_count++;
}

struct TRACKOBJECT *frame_track_object_from_legacy_index(legacy_u8 index)
{
	record_word(8);
	record_word(index);
	if (index < 215) {
		return &trkObjectList[index];
	}
	assert(index < 234);
	return &terrain_scene_objects[index - 215];
}

static void reset_projection(void)
{
	video_x_alignment = 1;
	video_x_alignment_mask = -1;
	queued_count = 0;
	flush_count = 0;
	pixel_count = 0;
	projection_focal_length_x = 256;
	projection_focal_length_y = 192;
	projection_center_x = 160;
	projection_center_y = 100;
}

static void intro_case(unsigned scenario)
{
	reset_projection();
	slow_video_mgmt_copy = scenario & 1;
	unsigned draw_car = (scenario >> 1) & 1;
	unsigned logo = (scenario >> 2) & 1;
	unsigned rotation = (scenario >> 3) % 4;
	static const legacy_s16 offsets[] = {0, 200, -200, 32767, -32768};
	legacy_s16 camera_x = offsets[(scenario >> 5) % 5];
	legacy_s16 camera_y = offsets[((scenario >> 5) + 1) % 5];
	legacy_s16 camera_z = offsets[((scenario >> 5) + 2) % 5];
	legacy_s16 old_count = scenario % 3 == 0 ? 0 : (scenario % 3 == 1 ? 3 : 100);
	legacy_s16 count = old_count;
	intro_palette_color_count = scenario % 2 ? 16 : 3;
	intro_colorvalue = intro_palette_color_count - 1;
	intro_redraw_cliprect = intro_cliprect;
	struct RECTANGLE shape_rect = {30, 110, 25, 75};
	struct RECTANGLE previous_rect = {-40, 360, -20, 220};
	if (scenario % 3 == 0) {
		previous_rect.left = shape_rect.left = 400;
		previous_rect.right = shape_rect.right = 420;
	}
	state.opponentstate.car_position.lx = 16385;
	state.opponentstate.car_position.ly = -321;
	state.opponentstate.car_position.lz = -327681;
	state.opponentstate.car_rotate.x = (legacy_s16)(scenario * 97);
	struct VECTOR stars[100];
	struct POINT2D previous_points[100];
	for (unsigned index = 0; index < 100; index++) {
		stars[index].x = LEGACY_S16_WRAP_ADD(camera_x, (legacy_s16)(index * 173 - 8000));
		stars[index].y = LEGACY_S16_WRAP_ADD(camera_y, (legacy_s16)(index * 113 - 5000));
		stars[index].z = LEGACY_S16_WRAP_ADD(camera_z, (legacy_s16)(index * 73 - 2400));
		previous_points[index].px = (legacy_s16)(index * 7 - 30);
		previous_points[index].py = (legacy_s16)(index * 3 - 20);
	}
	stars[0].z = LEGACY_S16_WRAP_ADD(camera_z, 199);
	stars[1].z = LEGACY_S16_WRAP_ADD(camera_z, 200);
	stars[2].z = LEGACY_S16_WRAP_ADD(camera_z, 201);
	struct RECTANGLE combined_rect = {1, 2, 3, 4};
	intro_render_scene(camera_x, camera_y, camera_z, rotation * 256, 0, draw_car, logo, stars,
					   previous_points, &count, previous_rect, &shape_rect, &combined_rect);
	assert(queued_count == 1 + draw_car);
	assert(flush_count == 1);
	assert(count >= 0 && count <= 100);
	if (slow_video_mgmt_copy != 0) {
		assert(pixel_count == (unsigned)(old_count + count));
	} else {
		assert(count == old_count);
	}
	record_word(count);
	record_word(intro_colorvalue);
	record_rect(&shape_rect);
	record_rect(&combined_rect);
	for (unsigned index = 0; index < 100; index++) {
		record_word(previous_points[index].px);
		record_word(previous_points[index].py);
	}
}

static void preview_map(unsigned variant)
{
	track_element_map = elements;
	track_terrain_map = terrain;
	for (unsigned row = 0; row < 30; row++) {
		trackrows[row] = row * 30;
		terrainrows[row] = (29 - row) * 30;
		track_column_positions[row] = row * 1024;
		track_column_centers[row] = row * 1024 + 512;
		track_row_positions[row] = (30 - row) * 1024;
		track_row_centers[row] = (29 - row) * 1024 + 512;
	}
	for (unsigned row = 0; row < 30; row++) {
		for (unsigned column = 0; column < 30; column++) {
			unsigned tile = (row * 30 + column + variant * 43) % 215;
			if ((column == 29 && (trkObjectList[tile].ss_multiTileFlag & 2) != 0) ||
				(row == 29 && tile >= 105 && tile <= 108)) {
				tile = 0;
			}
			if (column == 28 && row % 3 == 0) {
				tile = 253 + (row / 3) % 3;
			}
			elements[trackrows[row] + column] = tile;
			terrain[terrainrows[row] + column] =
				variant == 0 ? 0 : (variant == 1 ? 6 : (row + column * 3 + variant) % 19);
		}
	}
}

static void preview_case(unsigned scenario)
{
	reset_projection();
	preview_map(scenario % 4);
	track_preview_camera_x = (legacy_s16)(scenario * 1027 - 16000);
	track_preview_camera_y = (legacy_s16)(scenario * 337 - 8000);
	track_preview_camera_z = (legacy_s16)(17000 - scenario * 919);
	track_preview_target_x = 15360;
	track_preview_target_y = 100;
	track_preview_target_z = 15360;
	track_preview_horizon_vector.x = 0;
	track_preview_horizon_vector.y = scenario & 1 ? -400 : 400;
	track_preview_horizon_vector.z = 1000;
	skybox.minimum_height = 20;
	skybox.sky_color = 3;
	skybox.ground_color = 6;
	for (unsigned index = 0; index < 4; index++) {
		skybox.heights[index] = 20 + index * 5;
		skyboxes[index] = &sky_images[index];
	}
	draw_track_preview();
	assert(flush_count == 900);
	assert(queued_count > 0);
	record_word(queued_count);
}

static legacy_s8 title_data[3];
static struct SPRITE intro_sprite;
static unsigned input_polls, cancel_after, copy_backbuffer;
static legacy_u16 random_value;

void *file_load_3dres(const legacy_s8 *name)
{
	assert(strcmp((const char *)name, "title") == 0);
	record_word(20);
	return title_data;
}
void locate_many_resources(legacy_s8 *data, const legacy_s8 *names, legacy_s8 **result)
{
	assert(data == title_data);
	assert(strcmp((const char *)names, "logolog2brav") == 0);
	record_word(21);
	for (unsigned index = 0; index < 3; index++) {
		result[index] = &title_data[index];
	}
}
void shape3d_init_shape(legacy_s8 *source, struct SHAPE3D *destination)
{
	record_word(22);
	record_word(source - title_data);
	record_word(shape_id(destination));
}
struct SPRITE *sprite_make_wnd(legacy_u16 width, legacy_u16 height, legacy_u16 color)
{
	record_word(23);
	record_word(width);
	record_word(height);
	record_word(color);
	intro_sprite.sprite_bitmapptr = &sky_images[0];
	return &intro_sprite;
}
void sprite_free_wnd(struct SPRITE *sprite)
{
	assert(sprite == &intro_sprite);
	record_word(24);
}
legacy_s16 get_kevinrandom(void)
{
	random_value = (legacy_u16)(random_value * 25173U + 13849U);
	record_word(25);
	record_word(random_value & 255U);
	return random_value & 255U;
}
void set_projection(legacy_s16 horizontal, legacy_s16 vertical, legacy_s16 width, legacy_s16 height)
{
	record_word(26);
	record_word(horizontal);
	record_word(vertical);
	record_word(width);
	record_word(height);
}
void *file_load_resfile(const legacy_s8 *name)
{
	assert(strcmp((const char *)name, "carcoun") == 0);
	record_word(27);
	return title_data;
}
void setup_aero_trackdata(void *resource, legacy_s16 opponent)
{
	assert(resource == title_data);
	record_word(28);
	record_word(opponent);
}
void unload_resource(void *resource)
{
	assert(resource == title_data);
	record_word(29);
}
void *mmgr_free(legacy_s8 *resource)
{
	assert(resource == title_data);
	record_word(30);
	return 0;
}
void init_plantrak(void)
{
	record_word(31);
}
legacy_u32 timer_get_delta(void)
{
	record_word(32);
	return 2;
}
void update_opponent(void)
{
	record_word(33);
	state.opponentstate.car_position.lx += 128;
	state.opponentstate.car_position.lz += 64;
	state.opponentstate.car_rotate.x = LEGACY_S16_WRAP_ADD(state.opponentstate.car_rotate.x, 3);
}
legacy_s16 input_do_checking(legacy_s16 delta)
{
	record_word(34);
	record_word(delta);
	input_polls++;
	return cancel_after != 0 && input_polls >= cancel_after;
}
void sprite_select_mcga_backbuffer(void)
{
	record_word(35);
}
void sprite_select_render_window(void)
{
	record_word(36);
}
void sprite_select_screen_compat(void)
{
	record_word(37);
}
void sprite_present_mcga_backbuffer(void)
{
	record_word(38);
}
void mouse_draw_opaque_check(void)
{
	record_word(39);
}
void mouse_draw_transparent_check(void)
{
	record_word(40);
}
void sprite_putimage(struct SHAPE2D *shape)
{
	assert(shape == &sky_images[0]);
	record_word(41);
}
legacy_s16 video_backbuffer_copy_required(void)
{
	record_word(42);
	return copy_backbuffer;
}
void sprite_copy_rect_shifted(legacy_s16 x, legacy_s16 y, legacy_s16 width, legacy_s16 height,
							  legacy_s16 shift)
{
	record_word(43);
	record_word(x);
	record_word(y);
	record_word(width);
	record_word(height);
	record_word(shift);
}
/* Full lifecycle uses full redraw: the original setup routine leaves its first
 * dirty rectangle uninitialized. Standalone scene cases cover dirty rectangles
 * with explicit previous-frame inputs instead. */
static void lifecycle_case(unsigned scenario)
{
	reset_projection();
	memset(&state, 0, sizeof(state));
	framespersec = 20;
	timer_ticks_per_frame = 1;
	intro_elapsed_ticks = 0;
	intro_colorvalue = 1;
	intro_palette_color_count = 16;
	slow_video_mgmt = 0;
	video_uses_page_flipping = scenario & 1;
	copy_backbuffer = (scenario >> 1) & 1;
	static const unsigned cancellations[] = {0, 1, 80, 160};
	cancel_after = cancellations[(scenario >> 2) & 3];
	input_polls = 0;
	random_value = 1;
	state.opponentstate.car_position.lx = 64000;
	state.opponentstate.car_position.ly = 1280;
	state.opponentstate.car_position.lz = 64000;
	legacy_s8 interrupted = setup_intro();
	assert(interrupted == (cancel_after != 0));
	assert(input_polls > 0);
	record_word(interrupted);
	record_word(input_polls);
	record_word(random_value);
	record_word(intro_elapsed_ticks);
	record_word(intro_colorvalue);
}

int main(void)
{
	trace_hash = 2166136261UL;
	for (unsigned scenario = 0; scenario < 480; scenario++) {
		record_word(scenario);
		intro_case(scenario);
	}
	legacy_u32 intro_hash = trace_hash;
	trace_hash = 2166136261UL;
	for (unsigned scenario = 0; scenario < 32; scenario++) {
		record_word(scenario);
		preview_case(scenario);
	}
	legacy_u32 preview_hash = trace_hash;
	trace_hash = 2166136261UL;
	for (unsigned scenario = 0; scenario < 16; scenario++) {
		record_word(scenario);
		lifecycle_case(scenario);
	}
	legacy_u32 lifecycle_hash = trace_hash;
#ifdef INTRO_PREVIEW_RECORD_BASELINE
	fprintf(stdout, "intro=0x%08lx preview=0x%08lx lifecycle=0x%08lx\n", (unsigned long)intro_hash,
			(unsigned long)preview_hash, (unsigned long)lifecycle_hash);
#else
	/* Captured from unmodified production at 43fba20d. */
	assert(intro_hash == 0xcdcdbe61UL);
	assert(preview_hash == 0x8ae71f07UL);
	assert(lifecycle_hash == 0x89b10059UL);
#endif
	return 0;
}
