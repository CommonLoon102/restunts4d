#include <assert.h>

#include "../c/externs.h"
#include "../c/shape2d.h"
#include "../c/shape2d_internal.h"
#include "../c/shape3d.h"
#include "../c/shape3d_internal.h"

static unsigned solid_calls;
static legacy_s16 colors[3] = {7, 8, 9};
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

int main(void)
{
	legacy_s16 headings[4] = {11, 22, 33, 44};

	drawing_sprite.sprite_raster_left = 13;
	drawing_sprite.sprite_raster_right = 247;
	queue_polygon(0);
	shape3d_render_queued_primitives();
	assert(solid_calls == 1);
	assert_headings(headings, 11, 22, 33, 44);

	shape3d_set_legacy_render_stack(headings, 51720U, 6004U);
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

	shape3d_set_legacy_render_stack(headings, 32767U, 6005U);
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

	shape3d_set_legacy_render_stack(0, 0, 0);
	drawing_sprite.sprite_raster_right = 80;
	queue_polygon(0);
	shape3d_render_queued_primitives();
	assert(solid_calls == 4);
	assert_headings(headings, 319, 32767, 5239, 6005);
	return 0;
}
