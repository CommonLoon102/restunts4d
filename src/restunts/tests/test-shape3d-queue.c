#include <assert.h>
#include <string.h>

#include "../c/externs.h"
#include "../c/shape3d.h"
#include "../c/shape3d_internal.h"
#include "../c/projection.h"

#undef memcpy

/* Identity rotations avoid the angle lookup; these are math.c's link dependencies. */
legacy_u8 atantable[257];
legacy_u8 vector_saved_z_low;
legacy_u8 vector_saved_z_high;

extern legacy_u16 polyinfoptrnext;
extern legacy_u16 polygon_buffer_full;
extern legacy_s8 backlights_paint_override;

static legacy_u8 vertices[8U * SHAPE3D_VERTEX_SIZE];
static legacy_u8 primitives[128];
static legacy_u8 masks[128];
static legacy_u8 polyinfo[10400];
static struct SHAPE3D shape;
static struct TRANSFORMEDSHAPE3D instance;
static struct RECTANGLE bounds;

static void reset_scene(void)
{
	static const struct VECTOR points[] = {{-20, -20, 100}, {20, -20, 100},	 {0, 20, 100},
										   {0, 0, 0},		{-20, -20, 150}, {20, -20, 150},
										   {0, 20, 150},	{0, 0, 100}};
	unsigned i;

	memset(&shape, 0, sizeof(shape));
	memset(&instance, 0, sizeof(instance));
	memset(primitives, 0, sizeof(primitives));
	memset(polyinfo, 0, sizeof(polyinfo));
	memset(masks, 255, sizeof(masks));
	shape.shape3d_numverts = 8;
	shape.shape3d_vertex_bytes = vertices;
	shape.shape3d_numpaints = 1;
	shape.shape3d_primitives = primitives;
	shape.shape3d_visibility_masks = masks;
	shape.shape3d_front_facing_masks = masks;
	instance.shapeptr = &shape;
	instance.rectptr = &bounds;
	instance.ts_flags = 10; /* Pretransformed translation and bounding rectangle. */
	instance.culling_distance = 1024;
	bounds.left = 32767;
	bounds.top = 32767;
	bounds.right = -32768;
	bounds.bottom = -32768;
	for (i = 0; i < 8; i++) {
		shape3d_vertex_write(&shape, i, &points[i]);
	}
	projection_focal_length_x = 256;
	projection_focal_length_y = 256;
	select_rect_rc.left = 0;
	select_rect_rc.right = 319;
	select_rect_rc.top = 0;
	select_rect_rc.bottom = 199;
	mat_temp = *mat_rot_zxy(0, 0, 0, MATRIX_ROTATION_ORDER_ZXY);
	polyinfoptr = polyinfo;
	polyinfo_reset();
}

static void check_point(const legacy_u8 *record, unsigned index, legacy_s16 x, legacy_s16 y)
{
	assert(LEGACY_READ_S16_LE(record + 6U + index * 4U) == x);
	assert(LEGACY_READ_S16_LE(record + 8U + index * 4U) == y);
}

static void test_primitive_records(void)
{
	static const struct {
		legacy_u8 primitive[9];
		legacy_u16 depth;
		legacy_u8 type;
		legacy_u8 count;
		struct POINT2D points[4];
		legacy_s16 left, right, top, bottom;
	} cases[] = {{{1, 1, 7, 0}, 100, RENDER_PRIMITIVE_POINT, 1, {{109, 151}}, 109, 110, 151, 152},
				 {{2, 1, 7, 0, 3},
				  50,
				  RENDER_PRIMITIVE_LINE,
				  2,
				  {{109, 151}, {118, 142}},
				  109,
				  119,
				  142,
				  152},
				 /* Both near-plane intersections precede the associated visible edge.
		 * The legacy depth average divides by the clipped output count. */
				 {{3, 1, 7, 0, 1, 3},
				  50,
				  RENDER_PRIMITIVE_POLYGON,
				  4,
				  {{118, 142}, {109, 151}, {211, 151}, {202, 142}},
				  109,
				  212,
				  142,
				  152},
				 {{11, 1, 7, 7, 1},
				  100,
				  RENDER_PRIMITIVE_SPHERE,
				  2,
				  {{160, 100}, {71, 0}},
				  89,
				  232,
				  29,
				  172},
				 {{12, 1, 7, 0, 2, 1, 4, 6, 5},
				  100,
				  RENDER_PRIMITIVE_WHEEL,
				  4,
				  {{109, 151}, {160, 49}, {211, 151}, {126, 134}},
				  6,
				  230,
				  31,
				  255},
				 /* Reversing the first face selects the wheel's opposite face and depth. */
				 {{12, 1, 7, 0, 1, 2, 4, 5, 6},
				  150,
				  RENDER_PRIMITIVE_WHEEL,
				  4,
				  {{126, 134}, {194, 134}, {160, 66}, {109, 151}},
				  40,
				  196,
				  65,
				  221}};
	unsigned i, point;

	for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
		reset_scene();
		memcpy(primitives, cases[i].primitive, sizeof(cases[i].primitive));
		assert(shape3d_transform_and_queue(&instance) == 0);
		assert(polyinfonumpolys == 1);
		assert(polyinfoptrnext == 6U + 4U * cases[i].count);
		assert(LEGACY_READ_U16_LE(polyinfo) == cases[i].depth);
		assert(polyinfo[2] == 7);
		assert(polyinfo[3] == cases[i].count);
		assert(polyinfo[4] == cases[i].type);
		for (point = 0; point < cases[i].count; point++) {
			check_point(polyinfo, point, cases[i].points[point].px, cases[i].points[point].py);
		}
		assert(bounds.left == cases[i].left);
		assert(bounds.right == cases[i].right);
		assert(bounds.top == cases[i].top);
		assert(bounds.bottom == cases[i].bottom);
	}
}

static void test_shared_clipped_vertices(void)
{
	static const legacy_u8 records[] = {
		2, 1, 7, 0, 3,	  /* Clipping a line projects its shared behind-plane vertex. */
		3, 1, 8, 7, 1, 3, /* Polygon clipping must still use its original clip flag. */
		0, 0};
	const legacy_u8 *polygon;

	reset_scene();
	memcpy(primitives, records, sizeof(records));
	assert(shape3d_transform_and_queue(&instance) == 0);
	assert(polyinfonumpolys == 2);
	polygon = polyinfoptrs[1];
	/* The intersection at the center equals vertex 7 and must not be duplicated. */
	assert(polygon[3] == 3);
	assert(LEGACY_READ_U16_LE(polygon) == 66);
	check_point(polygon, 0, 160, 100);
	check_point(polygon, 1, 211, 151);
	check_point(polygon, 2, 202, 142);
}

static void test_hidden_primitive_children(void)
{
	static const legacy_u8 records[] = {
		1, 1, 7, 3, /* A point behind the near plane. */
		1, 2, 8, 7, /* Its attached child must also be skipped. */
		1, 1, 9, 1, /* The next independent primitive remains visible. */
		0, 0};

	reset_scene();
	memcpy(primitives, records, sizeof(records));
	assert(shape3d_transform_and_queue(&instance) == 0);
	assert(polyinfonumpolys == 1);
	assert(polyinfo[2] == 9);
	check_point(polyinfo, 0, 211, 151);
}

static void test_depth_order_and_attached_primitive(void)
{
	static const legacy_u8 records[] = {
		1, 1, 7, 0, /* Depth 100. */
		1, 1, 8, 4, /* Depth 150 sorts before the first primitive. */
		1, 2, 9, 0, /* Attached child stays immediately after its parent. */
		0, 0};

	reset_scene();
	memcpy(primitives, records, sizeof(records));
	assert(shape3d_transform_and_queue(&instance) == 0);
	assert(polyinfonumpolys == 3);
	assert(polygon_next_index[400] == 1);
	assert(polygon_next_index[1] == 2);
	assert(polygon_next_index[2] == 0);
	assert(polygon_next_index[0] == -1);
}

static void test_clipped_depth_signedness(void)
{
	static const struct {
		legacy_u8 source_count;
		legacy_u8 output_count;
		legacy_s16 depths[4];
		legacy_s16 average;
		legacy_u16 first_index;
	} cases[] = {{3, 3, {-100, -100, 100, 0}, 21812, 0},
				 {3, 4, {100, 100, -401, 0}, -51, 1},
				 {4, 5, {40, 50, 60, -600}, 13017, 0}};
	static const struct VECTOR points[] = {{-20, -20, 0}, {20, -20, 0}, {20, 20, 0}, {-20, 20, 0}};
	struct VECTOR vertex;
	unsigned i, j, next;

	for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
		reset_scene();
		primitives[0] = cases[i].source_count;
		primitives[1] = 1;
		primitives[2] = 7;
		for (j = 0; j < cases[i].source_count; j++) {
			vertex = points[j];
			vertex.z = cases[i].depths[j];
			shape3d_vertex_write(&shape, j, &vertex);
			primitives[3U + j] = (legacy_u8)j;
		}
		next = 3U + cases[i].source_count;
		primitives[next] = 1;
		primitives[next + 1U] = 1;
		primitives[next + 2U] = 8;
		primitives[next + 3U] = 4;

		assert(shape3d_transform_and_queue(&instance) == 0);
		assert(polyinfonumpolys == 2);
		assert(polyinfoptrs[0][3] == cases[i].output_count);
		/* Preserve the original's mixed signedness: negative depth sums use
		 * unsigned division for counts 3 and 5, but signed shifts for count 4.
		 * The resulting low word determines the signed painter-order key,
		 * even when this puts a near-clipped polygon behind a farther one. */
		assert(LEGACY_READ_S16_LE(polyinfoptrs[0]) == cases[i].average);
		assert(LEGACY_READ_S16_LE(polyinfoptrs[1]) == 150);
		assert(polygon_next_index[400] == cases[i].first_index);
		assert(polygon_next_index[cases[i].first_index] == (legacy_s16)(1U - cases[i].first_index));
		assert(polygon_next_index[1U - cases[i].first_index] == -1);
	}
}

static void test_backface_and_material_override(void)
{
	static const legacy_u8 triangle[] = {3, 0, 7, 0, 1, 2, 0, 0};

	reset_scene();
	memcpy(primitives, triangle, sizeof(triangle));
	assert(shape3d_transform_and_queue(&instance) == LEGACY_U16_MAX);
	assert(polyinfonumpolys == 0);
	primitives[1] = 1; /* Always-visible material side. */
	primitives[2] = BACKLIGHT_PAINT_DEFAULT;
	backlights_paint_override = 17;
	instance.material = 255; /* Invalid paint selection falls back to paint zero. */
	assert(shape3d_transform_and_queue(&instance) == 0);
	assert(polyinfonumpolys == 1);
	assert(polyinfo[2] == 17);
}

static void test_queue_limits(void)
{
	static const legacy_u8 point[] = {1, 1, 7, 0, 0, 0};
	static const legacy_u8 polygon[] = {10, 1, 7, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 0, 0};
	unsigned i;

	reset_scene();
	memcpy(primitives, point, sizeof(point));
	for (i = 0; i < 399; i++) {
		assert(shape3d_transform_and_queue(&instance) == 0);
	}
	assert(shape3d_transform_and_queue(&instance) == 1);
	assert(polyinfonumpolys == 400);
	assert(polygon_buffer_full == 1);
	assert(shape3d_transform_and_queue(0) == 1);
	reset_scene();
	memcpy(primitives, polygon, sizeof(polygon));
	for (i = 0; i < 225; i++) {
		assert(shape3d_transform_and_queue(&instance) == 0);
	}
	assert(shape3d_transform_and_queue(&instance) == 1);
	assert(polyinfonumpolys == 226);
	assert(polyinfoptrnext == 10396);
	reset_scene();
	shape.shape3d_numverts = 256;
	assert(shape3d_transform_and_queue(&instance) == 1);
	assert(polyinfonumpolys == 0);
}

int main(void)
{
	test_primitive_records();
	test_shared_clipped_vertices();
	test_hidden_primitive_children();
	test_depth_order_and_attached_primitive();
	test_clipped_depth_signedness();
	test_backface_and_material_override();
	test_queue_limits();
	return 0;
}
