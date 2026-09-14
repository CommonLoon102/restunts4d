#include <assert.h>
#include <string.h>

#include "../c/externs.h"
#include "../c/shape2d.h"
#include "../c/shape2d_internal.h"
#include "../c/shape3d.h"
#include "../c/shape3d_internal.h"

/* Exercise the stopped-wheel handoff without exporting its private geometry helpers. */
#include "../c/stateply.c"

#undef memcpy

static unsigned solid_calls;
static legacy_s16 colors[4] = {7, 8, 9, 10};
static legacy_s16 patterns[3];
static legacy_s16 secondary_patterns[3];
static legacy_s16 secondary_colors[3] = {19, 20, 21};
static legacy_u8 polygon_record[22] = {0, 0, 0, 3, RENDER_PRIMITIVE_POLYGON, 0};

void preRender_default(legacy_u16 color, legacy_u16 count, const struct POINT2D *points)
{
	assert(color == 7 && count == 3 && points != 0);
	solid_calls++;
}

void preRender_patterned(legacy_u16 pattern, legacy_u16 color, legacy_u16 count,
						 const struct POINT2D *points)
{
	(void)pattern;
	(void)color;
	(void)count;
	(void)points;
}

void preRender_two_color(legacy_u16 pattern, legacy_u16 color, legacy_u16 alternate_color,
						 legacy_u16 count, const struct POINT2D *points)
{
	(void)pattern;
	(void)color;
	(void)alternate_color;
	(void)count;
	(void)points;
}

void preRender_line(legacy_u16 x1, legacy_u16 y1, legacy_u16 x2, legacy_u16 y2, legacy_u16 color)
{
	(void)x1;
	(void)y1;
	(void)x2;
	(void)y2;
	(void)color;
}

void preRender_sphere(legacy_s16 x, legacy_s16 y, legacy_u16 size, legacy_u16 color)
{
	(void)x;
	(void)y;
	(void)size;
	(void)color;
}

void preRender_wheel(const struct POINT2D *points, legacy_u16 scale, legacy_u16 outer_color,
					 legacy_u16 inner_color, legacy_u16 hub_color)
{
	(void)points;
	(void)scale;
	(void)outer_color;
	(void)inner_color;
	(void)hub_color;
}

void sprite_putpixel_clipped(legacy_s16 x, legacy_s16 y, legacy_s16 color)
{
	(void)x;
	(void)y;
	(void)color;
}

static void queue_polygon(legacy_s16 pattern)
{
	material_clrlist_ptr_cpy = colors;
	material_patlist_ptr_cpy = patterns;
	material_patlist2_ptr_cpy = secondary_patterns;
	material_clrlist2_ptr_cpy = secondary_colors;
	patterns[0] = pattern;
	polygon_record[4] = RENDER_PRIMITIVE_POLYGON;
	polyinfo_reset();
	polyinfonumpolys = 1;
	polygon_next_index[400] = 0;
	polyinfoptrs[0] = polygon_record;
}

static void assert_headings(const legacy_s16 *headings, legacy_s16 a, legacy_s16 b, legacy_s16 c,
							legacy_s16 d)
{
	assert(headings[0] == a && headings[1] == b && headings[2] == c && headings[3] == d);
}

static legacy_u8 opponent_polyinfo[64];

static void queue_opponent_primitive(legacy_u8 primitive, legacy_s16 pattern)
{
	queue_polygon(pattern);
	memcpy(opponent_polyinfo + 24, polygon_record, sizeof(polygon_record));
	opponent_polyinfo[24 + 4] = primitive;
	polyinfoptr = opponent_polyinfo;
	polyinfoptrs[0] = opponent_polyinfo + 24;
}

static void assert_stopped_opponent_travel(legacy_s16 expected_x, legacy_s16 expected_z)
{
	struct PLAYER_WHEEL_MOTION motion;

	memset(&motion, 0, sizeof(motion));
	restore_stopped_wheel_headings(&state.opponentstate, &motion, OPPONENT_CAR_INDEX);
	car_to_world_rotation = *mat_rot_zxy(0, 0, 0, MATRIX_ROTATION_ORDER_ZXY);
	planindex = -1;
	prepare_wheel_plane_travel(&motion, 2, 64);
	assert(wheel_world_travel.x == expected_x);
	assert(wheel_world_travel.y == 0);
	assert(wheel_world_travel.z == expected_z);
	motion.travel = 1;
	motion.headings[2] = 75;
	restore_stopped_wheel_headings(&state.opponentstate, &motion, OPPONENT_CAR_INDEX);
	assert(motion.headings[2] == 75);
}

static void test_opponent_render_handoff(void)
{
	legacy_s16 *headings = legacy_execution_residue.wheel_angle_stack_words;

	headings[0] = 11;
	headings[1] = 22;
	headings[2] = 33;
	headings[3] = 44;
	struct SHAPE3D_LEGACY_OPPONENT_RENDER_CONTEXT context;
	context.wheel_headings = headings;
	context.polyinfo_offset = 65500U;
	context.polyinfo_segment = 256U;
	context.material_color_offset = 20628U;
	shape3d_set_legacy_render_stack(0, 0, 0, &context);
	/* The setter copies the context while retaining the caller's word buffer. */
	context.polyinfo_segment = 512U;
	polyinfo_reset();
	shape3d_render_queued_primitives();
	assert_headings(headings, 11, 22, 33, 44);

	/* Even an invisible polygon copies its vertices, advances the far offset,
	 * and leaves the segment unchanged when that offset wraps. */
	queue_opponent_primitive(RENDER_PRIMITIVE_POLYGON, 3);
	shape3d_render_queued_primitives();
	assert_headings(headings, 11, 6, 256, 44);
	assert_stopped_opponent_travel(-64, 0);

	queue_opponent_primitive(RENDER_PRIMITIVE_POLYGON, 1);
	secondary_patterns[0] = 0;
	shape3d_render_queued_primitives();
	assert_headings(headings, 0, 6, 256, 44);
	queue_opponent_primitive(RENDER_PRIMITIVE_POLYGON, 1);
	secondary_patterns[0] = 7;
	shape3d_render_queued_primitives();
	assert_headings(headings, 7, 6, 256, 44);
	queue_opponent_primitive(RENDER_PRIMITIVE_POLYGON, 2);
	opponent_polyinfo[24 + 2] = 1;
	patterns[1] = 2;
	shape3d_render_queued_primitives();
	assert_headings(headings, 2, 6, 256, 44);
	queue_opponent_primitive(RENDER_PRIMITIVE_WHEEL, 0);
	opponent_polyinfo[24 + 2] = 1;
	shape3d_render_queued_primitives();
	assert_headings(headings, 20630, 6, 256, 44);
	queue_opponent_primitive(RENDER_PRIMITIVE_LINE, 0);
	shape3d_render_queued_primitives();
	queue_opponent_primitive(RENDER_PRIMITIVE_SPHERE, 0);
	shape3d_render_queued_primitives();
	queue_opponent_primitive(RENDER_PRIMITIVE_POINT, 0);
	shape3d_render_queued_primitives();
	assert_headings(headings, 20630, 6, 256, 44);

	/* A different logical allocation changes the next stopped wheel's travel,
	 * without relying on a host pointer or a replay-specific segment value. */
	shape3d_set_legacy_render_stack(0, 0, 0, &context);
	queue_opponent_primitive(RENDER_PRIMITIVE_POLYGON, 3);
	shape3d_render_queued_primitives();
	assert_headings(headings, 20630, 6, 512, 44);
	assert_stopped_opponent_travel(0, -64);
	shape3d_set_legacy_render_stack(0, 0, 0, 0);
	queue_opponent_primitive(RENDER_PRIMITIVE_POLYGON, 1);
	shape3d_render_queued_primitives();
	assert_headings(headings, 20630, 6, 512, 44);
}

int main(void)
{
	drawing_sprite.sprite_raster_left = 13;
	drawing_sprite.sprite_raster_right = 247;
	queue_polygon(0);
	shape3d_render_queued_primitives();
	assert(solid_calls == 1);
	legacy_s16 headings[4] = {11, 22, 33, 44};
	assert_headings(headings, 11, 22, 33, 44);

	shape3d_set_legacy_render_stack(headings, 51720U, 6004U, 0);
	polyinfo_reset();
	shape3d_render_queued_primitives();
	assert_headings(headings, 11, 22, 33, 44);
	queue_polygon(3); /* An invisible material does not enter the rasterizer. */
	shape3d_render_queued_primitives();
	assert(solid_calls == 1);
	assert_headings(headings, 11, 22, 33, 44);

	queue_polygon(0);
	shape3d_render_queued_primitives();
	assert(solid_calls == 2);
	assert_headings(headings, 246, 13, -13816, 4895);

	shape3d_set_legacy_render_stack(headings, 32767U, 6005U, 0);
	drawing_sprite.sprite_raster_left = 0;
	drawing_sprite.sprite_raster_right = 320;
	queue_polygon(0);
	shape3d_render_queued_primitives();
	assert_headings(headings, 319, 0, 32767, 4895);

	queue_polygon(1);
	secondary_patterns[0] = 0;
	shape3d_render_queued_primitives();
	assert_headings(headings, 319, 0, 32767, 4895);
	queue_polygon(1);
	secondary_patterns[0] = 3;
	shape3d_render_queued_primitives();
	assert_headings(headings, 0, 32767, 4936, 6005);
	queue_polygon(2);
	shape3d_render_queued_primitives();
	assert_headings(headings, 32767, 5089, 6005, 19);

	queue_polygon(0);
	polygon_record[4] = RENDER_PRIMITIVE_LINE;
	LEGACY_WRITE_U16_LE(polygon_record + 6, 33000U);
	shape3d_render_queued_primitives();
	assert_headings(headings, 32767, 5119, 6005, -32536);

	queue_polygon(0);
	polygon_record[4] = RENDER_PRIMITIVE_WHEEL;
	shape3d_render_queued_primitives();
	assert_headings(headings, 32767, 5213, 6005, 32717);

	queue_polygon(0);
	polygon_record[4] = RENDER_PRIMITIVE_POINT;
	polyinfonumpolys = 2;
	polygon_next_index[400] = 7;
	polygon_next_index[7] = 3;
	polyinfoptrs[7] = polygon_record;
	polyinfoptrs[3] = polygon_record;
	shape3d_render_queued_primitives();
	assert_headings(headings, 3, 1, 32767, 5261);

	queue_polygon(0);
	polygon_record[4] = RENDER_PRIMITIVE_SPHERE;
	LEGACY_WRITE_U16_LE(polygon_record + 10, 0);
	shape3d_render_queued_primitives();
	assert_headings(headings, 3, 32767, 5239, 6005);
	queue_polygon(0);
	polygon_record[4] = RENDER_PRIMITIVE_SPHERE;
	LEGACY_WRITE_U16_LE(polygon_record + 10, 1);
	shape3d_render_queued_primitives();
	assert_headings(headings, 3, 32767, 5239, 6005);
	queue_polygon(0);
	polygon_record[4] = RENDER_PRIMITIVE_SPHERE;
	LEGACY_WRITE_U16_LE(polygon_record + 10, 65535U);
	shape3d_render_queued_primitives();
	assert_headings(headings, 3, 32767, 5239, 6005);
	queue_polygon(0);
	polygon_record[4] = RENDER_PRIMITIVE_SPHERE;
	LEGACY_WRITE_U16_LE(polygon_record + 10, 2);
	shape3d_render_queued_primitives();
	assert_headings(headings, 319, 32767, 5239, 6005);

	shape3d_set_legacy_render_stack(0, 0, 0, 0);
	drawing_sprite.sprite_raster_right = 80;
	queue_polygon(0);
	shape3d_render_queued_primitives();
	assert(solid_calls == 4);
	assert_headings(headings, 319, 32767, 5239, 6005);
	test_opponent_render_handoff();
	return 0;
}
