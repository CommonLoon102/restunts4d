#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "../c/externs.h"
#include "../c/frame_internal.h"
#include "../c/shape2d.h"
#include "../c/shape2d_internal.h"
#include "../c/skybox.h"
#include "../c/projection.h"

legacy_u8 atantable[257];
legacy_u8 vector_saved_z_low;
legacy_u8 vector_saved_z_high;
legacy_s16 video_x_alignment = 1;
legacy_s16 video_x_alignment_mask = -1;

static struct SHAPE2D images[4];
static legacy_u32 call_hash;
static unsigned clear_count, image_count, polygon_count;

static void record_word(legacy_u16 value)
{
	call_hash = (call_hash ^ value) * 16777619UL;
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
	drawing_sprite.sprite_raster_left = left;
	drawing_sprite.sprite_raster_right = right;
	drawing_sprite.sprite_top = top;
	drawing_sprite.sprite_bottom = bottom;
}

void sprite_clear_target(legacy_u8 color)
{
	record_word(2);
	record_word(color);
	clear_count++;
}

void sprite_copy_image_at(struct SHAPE2D far *shape, legacy_s16 x, legacy_s16 y)
{
	record_word(3);
	record_word((legacy_u16)(shape - images));
	record_word((legacy_u16)x);
	record_word((legacy_u16)y);
	image_count++;
}

void skybox_fill_polygon(legacy_u16 color, legacy_u16 count, struct POINT2D *points)
{
	unsigned i;

	record_word(4);
	record_word(color);
	record_word(count);
	for (i = 0; i < count; i++) {
		record_word((legacy_u16)points[i].px);
		record_word((legacy_u16)points[i].py);
	}
	polygon_count++;
}

static void reset_scene(void)
{
	unsigned i;

	call_hash = 2166136261UL;
	clear_count = 0;
	image_count = 0;
	polygon_count = 0;
	projection_focal_length_x = 256;
	projection_focal_length_y = 256;
	projection_center_x = 160;
	projection_center_y = 100;
	skybox.minimum_height = 20;
	skybox.maximum_height = 40;
	skybox.sky_color = 3;
	skybox.ground_color = 6;
	for (i = 0; i < 4; i++) {
		skybox.heights[i] = 20 + i * 5;
		skyboxes[i] = &images[i];
	}
	for (i = 0; i < 15; i++) {
		frame_rects_page0[i].left = i * 20;
		frame_rects_page0[i].right = i * 20 + 20;
		frame_rects_page0[i].top = 30 + i * 3;
		frame_rects_page0[i].bottom = 60 + i * 3;
		frame_layer_rects[i] = frame_rects_page0[i];
		frame_rect_change_flags[i] = i % 4;
	}
	active_frame_rects = frame_rects_page0;
	memset(merged_redraw_rects, 0, 45 * sizeof(merged_redraw_rects[0]));
	redraw_rect_count = 0;
	frame_buffer_camera_headings[0] = 0;
	frame_buffer_camera_headings[1] = 0;
	last_rendered_camera_heading = 0;
	detail_level = 0;
	full_redraw_frames_remaining = 0;
	slow_video_mgmt_copy = 0;
}

static void test_level_horizon(void)
{
	struct RECTANGLE clip = {0, 320, 20, 180};
	struct MATRIX rotation;

	reset_scene();
	rotation = *mat_rot_zxy(0, 0, 0, MATRIX_ROTATION_ORDER_ZXY);
	assert(skybox_render(0, &clip, 1, &rotation, 0, 0, 0) == 0);
	assert(clear_count == 2);
	assert(image_count == 5);
	assert(polygon_count == 0);
	assert(drawing_sprite.sprite_top == 100);
	assert(drawing_sprite.sprite_bottom == 180);

	reset_scene();
	slow_video_mgmt_copy = 1;
	assert(skybox_render(0, &clip, -1, &rotation, 0, 0, 0) == 1);
	assert(clear_count == 1);
	assert(image_count == 0);
	assert(rect_skybox.left == 0);
	assert(rect_skybox.right == 320);
	assert(rect_skybox.top == 20);
	assert(rect_skybox.bottom == 180);
}

static legacy_u16 random_word(legacy_u32 *seed)
{
	*seed = *seed * 1664525UL + 1013904223UL;
	return (legacy_u16)(*seed >> 16);
}

static legacy_u32 skybox_fingerprint(legacy_s16 rolled, legacy_s16 slow_copy)
{
	struct MATRIX rotation;
	struct RECTANGLE clip;
	legacy_s16 angle, camera_y, direction, result;
	legacy_u16 rotate_z, rotate_x, rotate_y;
	legacy_u32 seed = 161803UL;
	legacy_u32 hash = 2166136261UL;
	unsigned iteration, i;

	for (iteration = 0; iteration < 8192; iteration++) {
		reset_scene();
		clip.left = 0;
		clip.right = 320;
		clip.top = random_word(&seed) % 80;
		clip.bottom = 120 + random_word(&seed) % 81;
		angle = random_word(&seed) % 1024;
		camera_y = LEGACY_S16_FROM_BITS(random_word(&seed));
		direction = iteration % 2 == 0 ? 1 : -1;
		rotate_z = random_word(&seed) % 1024;
		rotate_x = random_word(&seed) % 1024;
		rotate_y = random_word(&seed) % 1024;
		rotation = *mat_rot_zxy(rotate_z, rotate_x, rotate_y, MATRIX_ROTATION_ORDER_ZXY);
		if (iteration % 3 == 0) {
			/* Small bank angles exercise the textured horizon-strip path. */
			rotation = *mat_rot_zxy(iteration % 32, 0, 0, MATRIX_ROTATION_ORDER_ZXY);
			camera_y = iteration % 1500;
			direction = 1;
		}
		detail_level = iteration % 5;
		slow_video_mgmt_copy = slow_copy;
		full_redraw_frames_remaining = iteration % 3 == 1;
		last_rendered_camera_heading = angle;
		frame_buffer_camera_headings[iteration % 2] = angle;
		result = skybox_render(iteration % 2, &clip, direction, &rotation, rolled, angle, camera_y);
		record_word((legacy_u16)result);
		record_word((legacy_u16)redraw_rect_count);
		for (i = 0; i < 15; i++) {
			record_word((legacy_u16)frame_rect_change_flags[i]);
			record_word((legacy_u16)frame_layer_rects[i].left);
			record_word((legacy_u16)frame_layer_rects[i].right);
			record_word((legacy_u16)frame_layer_rects[i].top);
			record_word((legacy_u16)frame_layer_rects[i].bottom);
		}
		for (i = 0; i < (unsigned)redraw_rect_count; i++) {
			record_word((legacy_u16)merged_redraw_rects[i].left);
			record_word((legacy_u16)merged_redraw_rects[i].right);
			record_word((legacy_u16)merged_redraw_rects[i].top);
			record_word((legacy_u16)merged_redraw_rects[i].bottom);
		}
		record_word((legacy_u16)frame_buffer_camera_headings[0]);
		record_word((legacy_u16)frame_buffer_camera_headings[1]);
		hash = (hash ^ call_hash) * 16777619UL;
	}
	return hash;
}

int main(void)
{
	/* Geometry and rectangle merging use the real implementations. Raster
	 * callbacks fingerprint their arguments and order without a video device.
	 * Rolled-view baselines include the original long-line slope rounding. */
	static const legacy_u32 expected[] = {1958318220UL, 1443166741UL, 1150680283UL, 2189361964UL};
	unsigned i;

	test_level_horizon();
	for (i = 0; i < 4; i++) {
		assert(skybox_fingerprint(i & 1U, i >> 1U) == expected[i]);
	}
	return 0;
}
