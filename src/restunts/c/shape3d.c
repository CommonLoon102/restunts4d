#include <stddef.h>
#include <limits.h>
#include "externs.h"
#include "fileio.h"
#include "legacy.h"
#include "memmgr.h"
#include "shape3d.h"
#include "shape3d_internal.h"
#include "shape2d.h"
#include "math_internal.h"
#include "car_model.h"
#include "projection.h"

/*

TODO:

  lines function (calls)
- ----- --------- -------
X   159 mat_rot_zxy (16)
X    60 mat_multiply (0)
X   122 mat_mul_vector (0)
X    48 mat_invert (0)
X   126 vector_direction_sector (14)
X    97 vector_to_point (0)
X    32 rect_compare_point (0)
X    40 vector_interpolate_at_z (0)
X    86 is_facing_camera (4)
X    33 rect_adjust_from_point(0)
X    47 polarRadius2D (6)
X    13 polarRadius3D (4)
X     7 projection_scale_x_wrapped (0)
X    74 polygon_insert_newest (0)
X     - polarAngle (0)
X     - mat_rot_z (4)
X     - mat_rot_x (4)
X     - mat_rot_y (4)
X     - set_projection (10)
X     - select_cliprect_rotate (10)

*/

void shape3d_vertex_read(const struct SHAPE3D* shape, legacy_u16 index,
	struct VECTOR* destination)
{
	const legacy_u8 far* source;

	source = shape->shape3d_vertex_bytes +
		LEGACY_U16_WRAP_MUL(index, SHAPE3D_VERTEX_SIZE);
	destination->x = LEGACY_READ_S16_LE(source + SHAPE3D_VERTEX_X_OFFSET);
	destination->y = LEGACY_READ_S16_LE(source + SHAPE3D_VERTEX_Y_OFFSET);
	destination->z = LEGACY_READ_S16_LE(source + SHAPE3D_VERTEX_Z_OFFSET);
}

void shape3d_vertex_write(struct SHAPE3D* shape, legacy_u16 index,
	const struct VECTOR* source)
{
	legacy_u8 far* destination;

	destination = shape->shape3d_vertex_bytes +
		LEGACY_U16_WRAP_MUL(index, SHAPE3D_VERTEX_SIZE);
	LEGACY_WRITE_U16_LE(destination + SHAPE3D_VERTEX_X_OFFSET,
		(legacy_u16)source->x);
	LEGACY_WRITE_U16_LE(destination + SHAPE3D_VERTEX_Y_OFFSET,
		(legacy_u16)source->y);
	LEGACY_WRITE_U16_LE(destination + SHAPE3D_VERTEX_Z_OFFSET,
		(legacy_u16)source->z);
}

extern legacy_s8 is_facing_camera(struct POINT2D far*);
extern legacy_u16 polygon_insert_newest(legacy_u16, legacy_u16);
extern legacy_u16 projection_scale_x_wrapped(legacy_u16, legacy_s16);

extern legacy_u16 polygon_buffer_full;
extern legacy_u16 transshapenumverts;
extern legacy_u8 far* transshapeprimitives;
extern legacy_u16 transshapenumpaints;
extern legacy_u8 transshapematerial;
extern legacy_u8 transshapeflags;
extern struct RECTANGLE* transshaperectptr;
extern legacy_s32 invpow2tbl[32];
extern legacy_u8 shape_view_direction_sector;

// Track the current shape's portion of the depth-sorted polygon list.
// Polygon immediately preceding this shape's first polygon.
extern legacy_u16 shape_polygon_predecessor;
// Last polygon in the full list, or its sentinel when empty.
extern legacy_u16 polygon_list_tail;
// Number of polygons queued for the current shape.
extern legacy_u16 shape_polygon_count;
// After insertion, contains the index of the newly inserted primitive
extern legacy_u16 polygon_insertion_cursor;

extern legacy_u8 transshapenumvertscopy;
extern struct POINT2D* polyvertpointptrtab[];
extern legacy_u16 shape_half_scale;
extern legacy_u8 primidxcounttab[];
extern legacy_u8 primtypetab[];
extern legacy_u8 far* transshapeprimptr;
extern legacy_u16 polyinfoptrnext;
extern legacy_u8 far* transshapepolyinfo;
extern legacy_s8 transprimitivepaintjob;
extern legacy_u8 far* transshapeprimindexptr;

/* 14-bit fixed point: the inner radius is 37/64 of the outer radius. */
#define WHEEL_INNER_RADIUS_SCALE 9472U

#define SHAPE3D_VERTEX_CAPACITY 255U
#define SHAPE3D_VERTEX_FLAG_CAPACITY 256U
#define SHAPE3D_VERTEX_UNTRANSFORMED LEGACY_U8_MAX
#define SHAPE3D_FORWARD_VECTOR_SCALE 4096
#define SHAPE3D_NEAR_CLIP_Z 12
#define SHAPE3D_ALL_RECT_CLIP_FLAGS 15U
#define SHAPE3D_USE_BOUNDING_RECT_FLAG 8U
#define SHAPE3D_PRETRANSFORMED_FLAG 2U
#define SHAPE3D_NO_DEPTH_SORT_FLAG 1U
#define SHAPE3D_PRIMITIVE_ALWAYS_VISIBLE_FLAG 1U
#define SHAPE3D_PRIMITIVE_SKIP_DEPTH_SORT_FLAG 2U

#define POLYINFO_LIST_CAPACITY 400U
#define POLYINFO_LIST_SENTINEL LEGACY_U16_MAX
#define POLYINFO_DATA_LAST_VALID_OFFSET 10354U
#define POLYINFO_DATA_ALLOCATION_SIZE 10400U
#define POLYINFO_MAX_RENDER_POINTS 13U

#define PROJECTION_EXTENT_SCALE 2048L
#define PROJECTION_EXTENT_DIVISOR 360L
#define PROJECTION_VIEW_VECTOR_LENGTH 10000
#define PROJECTION_YAW_MASK 1023U
#define PROJECTION_SINCOS_ANGLE 128

static legacy_u16 shape3d_average_depth(legacy_s32 sum,
	legacy_u16 vertex_count)
{
	legacy_u32 divisor_bits;
	legacy_s32 divisor;

	switch (vertex_count) {
	case 1U:
		return (legacy_u16)sum;
	case 2U:
		return (legacy_u16)LEGACY_S32_SAR(sum, 1U);
	case 4U:
		return (legacy_u16)LEGACY_S32_SAR(sum, 2U);
	case 8U:
		return (legacy_u16)LEGACY_S32_SAR(sum, 3U);
	default:
		divisor_bits = (legacy_u32)vertex_count;
		divisor = LEGACY_S32_FROM_BITS(divisor_bits);
		return (legacy_u16)LEGACY_S32_DIV_OR_ZERO(sum, divisor);
	}
}

static legacy_u16 polyinfo_read_word(const legacy_u8 far* record,
	legacy_u16 word_index)
{
	return LEGACY_READ_U16_LE(record +
		LEGACY_U16_WRAP_MUL(word_index, 2U));
}

static void polyinfo_write_word(legacy_u8 far* record,
	legacy_u16 word_index, legacy_u16 value)
{
	LEGACY_WRITE_U16_LE(record +
		LEGACY_U16_WRAP_MUL(word_index, 2U), value);
}

static void polyinfo_read_point(const legacy_u8 far* record,
	legacy_u16 point_index, struct POINT2D* point)
{
	legacy_u16 word_index;

	word_index = LEGACY_U16_WRAP_ADD(3U,
		LEGACY_U16_WRAP_MUL(point_index, 2U));
	point->px = LEGACY_S16_FROM_BITS(
		polyinfo_read_word(record, word_index));
	point->py = LEGACY_S16_FROM_BITS(polyinfo_read_word(record,
		LEGACY_U16_WRAP_ADD(word_index, 1U)));
}

static void polyinfo_read_points(const legacy_u8 far* record,
	struct POINT2D* points, legacy_u16 point_count)
{
	legacy_u16 index;

	for (index = 0; index < point_count; index++)
		polyinfo_read_point(record, index, &points[index]);
}

static void polyinfo_write_point(legacy_u8 far* record,
	legacy_u16 point_index, const struct POINT2D* point)
{
	legacy_u16 word_index;

	word_index = LEGACY_U16_WRAP_ADD(3U,
		LEGACY_U16_WRAP_MUL(point_index, 2U));
	polyinfo_write_word(record, word_index, (legacy_u16)point->px);
	polyinfo_write_word(record, LEGACY_U16_WRAP_ADD(word_index, 1U),
		(legacy_u16)point->py);
}

/* Emitting a polygon point always appends it to the polyinfo record and
   narrows the clip flags by the same test. */
static void polyinfo_emit_point(legacy_u16* point_index,
	legacy_u8* rect_flags, struct POINT2D* point)
{
	polyinfo_write_point(transshapepolyinfo, *point_index, point);
	if (*rect_flags != 0)
		*rect_flags &= rect_compare_point(point);
	*point_index = LEGACY_U16_WRAP_ADD(*point_index, 1U);
}

static legacy_s8 polyinfo_is_facing_camera(const legacy_u8 far* record)
{
	struct POINT2D points[3];
	legacy_u16 index;

	for (index = 0; index < 3U; index++)
		polyinfo_read_point(record, index, &points[index]);
	return is_facing_camera(points);
}

static void shape3d_transform_vertex(const struct SHAPE3D* shape,
	legacy_u16 index, legacy_s16 half_scale, struct MATRIX* matrix,
	struct VECTOR* translation, struct VECTOR* transformed)
{
	struct VECTOR source;

	shape3d_vertex_read(shape, index, &source);
	if (half_scale != 0) {
		/* Arithmetic shifts preserve the original flooring for negatives. */
		source.x = LEGACY_S16_SAR(source.x, 1U);
		source.y = LEGACY_S16_SAR(source.y, 1U);
		source.z = LEGACY_S16_SAR(source.z, 1U);
	}
	mat_mul_vector(&source, matrix, transformed);
	transformed->x = LEGACY_S16_WRAP_ADD(transformed->x, translation->x);
	transformed->y = LEGACY_S16_WRAP_ADD(transformed->y, translation->y);
	transformed->z = LEGACY_S16_WRAP_ADD(transformed->z, translation->z);
}

legacy_u16 shape3d_transform_and_queue(struct TRANSFORMEDSHAPE3D* instance) {
	legacy_u8 far* visibility_masks;
	legacy_u8 far* front_facing_masks;

	legacy_u8 vertex_clip_flags[SHAPE3D_VERTEX_FLAG_CAPACITY];
	struct MATRIX* object_rotation;
	struct MATRIX inverse_view_rotation;
	struct MATRIX object_to_view_rotation;
	struct VECTOR view_translation;
	struct VECTOR scratch_vector;
	struct VECTOR transformed_vector;
	struct VECTOR sphere_radius_endpoint;
	legacy_s32 visibility_mask;
	legacy_s32 front_facing_mask;
	legacy_u16 queued_primitive_count, all_vertices_behind, any_vertex_behind;
	legacy_u8 common_clip_flags, primitive_type;
	struct VECTOR view_vertices[SHAPE3D_VERTEX_CAPACITY];
	legacy_u16 primitive_flags, resource_primitive_type, primitive_visible;
	legacy_u16 polygon_vertex_count, vertex_index, previous_vertex_index, sphere_screen_radius;
	legacy_s16 point_x, point_y;
	legacy_u16 output_point_index;
	legacy_s32 depth_sum;
	struct POINT2D projected_point, bounds_point;
	struct POINT2D polyinfo_points[4];
	struct POINT2D projected_vertices[SHAPE3D_VERTEX_CAPACITY];
	struct POINT2D** projected_point_pointer;

	legacy_u16 i;
	legacy_u16 vertex_radius_or_sort_flag, vertex_index_or_depth, vertex_index_or_radius;

	//result = ported_transformed_shape_op_(instance);
	//return result;

	if (polygon_buffer_full != 0) return 1;
	transshapenumverts = instance->shapeptr->shape3d_numverts;
	/* Shape files store this count in one byte.  Reject a damaged descriptor
	 * before it can overrun the fixed-size transformation work arrays. */
	if (transshapenumverts > SHAPE3D_VERTEX_CAPACITY)
		return 1;
	transshapeprimitives = instance->shapeptr->shape3d_primitives;
	transshapenumpaints = instance->shapeptr->shape3d_numpaints;
	visibility_masks = instance->shapeptr->shape3d_visibility_masks;
	front_facing_masks = instance->shapeptr->shape3d_front_facing_masks;
	transshapematerial = instance->material;
	if (transshapematerial >= transshapenumpaints)
		transshapematerial = 0;
	transshapeflags = instance->ts_flags;

	if ((transshapeflags & SHAPE3D_USE_BOUNDING_RECT_FLAG) != 0) {
		transshaperectptr = instance->rectptr;
	}

	for (i = 0; i < transshapenumverts; i++) {
		vertex_clip_flags[i] = SHAPE3D_VERTEX_UNTRANSFORMED;
	}

	if ((transshapeflags & SHAPE3D_PRETRANSFORMED_FLAG) == 0) {
		object_rotation = mat_rot_zxy(instance->rotvec.x,
			instance->rotvec.y, instance->rotvec.z,
			MATRIX_ROTATION_ORDER_ZXY);
		mat_mul_vector(&instance->pos, &mat_temp, &view_translation);
		mat_multiply(object_rotation, &mat_temp, &object_to_view_rotation);
		mat_invert(&object_to_view_rotation, &inverse_view_rotation);
		scratch_vector.x = 0;
		scratch_vector.y = 0;
		scratch_vector.z = SHAPE3D_FORWARD_VECTOR_SCALE;
		mat_mul_vector(&scratch_vector, &inverse_view_rotation, &transformed_vector);
		if ((transformed_vector.y <= 0 || instance->pos.y >= 0) &&
			(LEGACY_S16_SHL(instance->culling_distance, 1U) <=
				absolute_word(view_translation.x) ||
			LEGACY_S16_SHL(instance->culling_distance, 1U) <=
				absolute_word(view_translation.z))) {
			shape_view_direction_sector = vector_direction_sector(&transformed_vector);
			visibility_mask = invpow2tbl[shape_view_direction_sector];
			front_facing_mask = invpow2tbl[shape_view_direction_sector];
		} else {
			visibility_mask = -1;
			front_facing_mask = 0;
		}
	} else {
		object_rotation = mat_rot_zxy(instance->rotvec.x,
			instance->rotvec.y, instance->rotvec.z,
			MATRIX_ROTATION_ORDER_ZXY);
		mat_multiply(object_rotation, &mat_temp, &object_to_view_rotation);
		view_translation = instance->pos;
		visibility_mask = -1;
		front_facing_mask = 0;
	}

	shape_polygon_predecessor = polygon_list_tail;
	polygon_insertion_cursor = polygon_list_tail;
	shape_polygon_count = 0;
	queued_primitive_count = 0;

	if (transshapenumverts <= 8) {
		transshapenumvertscopy = transshapenumverts;
	} else {
		transshapenumvertscopy = 8;
	}

	if (transshapenumvertscopy > 4) {
		shape3d_vertex_read(instance->shapeptr, 0U, &scratch_vector);
		shape3d_vertex_read(instance->shapeptr, 4U, &transformed_vector);
		if (scratch_vector.y == transformed_vector.y)
			transshapenumvertscopy = 4;
	}

	common_clip_flags = SHAPE3D_ALL_RECT_CLIP_FLAGS;
	all_vertices_behind = 1;
	any_vertex_behind = 0;
	for (i = 0; i < transshapenumvertscopy;
		i = LEGACY_U16_WRAP_ADD(i, 1U)) {
		polyvertpointptrtab[i] = &projected_vertices[i];
		shape3d_transform_vertex(instance->shapeptr, i,
			shape_half_scale, &object_to_view_rotation, &view_translation, &transformed_vector);
		view_vertices[i] = transformed_vector;
		if (transformed_vector.z < SHAPE3D_NEAR_CLIP_Z) {
			vertex_clip_flags[i] = 1;
			any_vertex_behind = 1;
			continue;
		}
		all_vertices_behind = 0;
		vertex_clip_flags[i] = 0;
		vector_to_point(&transformed_vector, polyvertpointptrtab[i]);
		if (common_clip_flags != 0)
			common_clip_flags &= rect_compare_point(polyvertpointptrtab[i]);
		if (common_clip_flags == 0)
			break;
	}
	if (i == transshapenumvertscopy &&
		(all_vertices_behind != 0 || any_vertex_behind == 0 ||
		LEGACY_S16_FROM_BITS(instance->culling_distance) <
			absolute_word(view_translation.x))) {
		return (legacy_u16)-1;
	}

	transshapeprimitives = instance->shapeptr->shape3d_primitives;

	for (;;) {
	transshapeprimptr = transshapeprimitives + primidxcounttab[transshapeprimitives[0]] + transshapenumpaints + 2;
	primitive_flags = transshapeprimitives[1];
	primitive_visible = 0;
	if ((LEGACY_READ_U32_LE(visibility_masks) & (legacy_u32)visibility_mask) != 0UL) {

	resource_primitive_type = transshapeprimitives[0];
	transshapenumvertscopy = primidxcounttab[resource_primitive_type];
	primitive_type = primtypetab[resource_primitive_type];

	transshapepolyinfo = polyinfoptr + polyinfoptrnext;
	polyinfoptrs[polyinfonumpolys] = transshapepolyinfo;

	transprimitivepaintjob = transshapeprimitives[2 + transshapematerial];
	transshapeprimitives += 2 + transshapenumpaints; // <- skip header and materials, -> point at indices

	common_clip_flags = SHAPE3D_ALL_RECT_CLIP_FLAGS;
	all_vertices_behind = 1;
	any_vertex_behind = 0;
	transshapeprimindexptr = transshapeprimitives;
	polygon_vertex_count = 0;
	while (polygon_vertex_count < transshapenumvertscopy) {
		vertex_radius_or_sort_flag = transshapeprimindexptr[0];
		transshapeprimindexptr++;
		polyvertpointptrtab[polygon_vertex_count] = &projected_vertices[vertex_radius_or_sort_flag];

		if (vertex_clip_flags[vertex_radius_or_sort_flag] == SHAPE3D_VERTEX_UNTRANSFORMED) {
			shape3d_transform_vertex(instance->shapeptr,
				vertex_radius_or_sort_flag, shape_half_scale, &object_to_view_rotation, &view_translation, &transformed_vector);
			view_vertices[vertex_radius_or_sort_flag] = transformed_vector;
			if (transformed_vector.z >= SHAPE3D_NEAR_CLIP_Z) {
				all_vertices_behind = 0;
				vertex_clip_flags[vertex_radius_or_sort_flag] = 0;
				vector_to_point(&transformed_vector,
					polyvertpointptrtab[polygon_vertex_count]);
			} else {
				vertex_clip_flags[vertex_radius_or_sort_flag] = 1;
				any_vertex_behind = 1;
			}
		} else if (vertex_clip_flags[vertex_radius_or_sort_flag] == 0) {
			all_vertices_behind = 0;
		} else if (vertex_clip_flags[vertex_radius_or_sort_flag] == 1) {
			any_vertex_behind = 1;
		}

		if (vertex_clip_flags[vertex_radius_or_sort_flag] == 0 && common_clip_flags != 0) {
			common_clip_flags &= rect_compare_point(
				polyvertpointptrtab[polygon_vertex_count]);
		}
		polygon_vertex_count = LEGACY_U16_WRAP_ADD(
			polygon_vertex_count, 1U);
	}

	if (all_vertices_behind == 0 && (common_clip_flags == 0 || any_vertex_behind != 0)) {
	if (primitive_type == RENDER_PRIMITIVE_POLYGON) {
	output_point_index = 0U;
	transshapeprimindexptr = transshapeprimitives;
	depth_sum = 0;
	common_clip_flags = SHAPE3D_ALL_RECT_CLIP_FLAGS;
	if (any_vertex_behind == 0) {
		for (i = 0; i < transshapenumvertscopy; i++) {
			vertex_index = transshapeprimindexptr[0];
			transshapeprimindexptr++;
			depth_sum = LEGACY_S32_WRAP_ADD_S16(
				depth_sum, view_vertices[vertex_index].z);
			projected_point_pointer = &polyvertpointptrtab[i];
			polyinfo_emit_point(&output_point_index,
				&common_clip_flags, *projected_point_pointer);
		}
	} else {
		polygon_vertex_count = 0;
		previous_vertex_index = transshapeprimitives[transshapenumvertscopy - 1];
		for (i = 0; i < transshapenumvertscopy;
			i = LEGACY_U16_WRAP_ADD(i, 1U)) {
			vertex_index = transshapeprimindexptr[0];
			transshapeprimindexptr++;
			depth_sum = LEGACY_S32_WRAP_ADD_S16(
				depth_sum, view_vertices[vertex_index].z);

			if (vertex_clip_flags[vertex_index] != 0) {
				if (vertex_clip_flags[previous_vertex_index] == 0) {
					vector_interpolate_at_z(&view_vertices[previous_vertex_index],
						&view_vertices[vertex_index], &scratch_vector,
						SHAPE3D_NEAR_CLIP_Z);
					vector_to_point(&scratch_vector, &projected_point);
					if (projected_point.px != projected_vertices[previous_vertex_index].px ||
						projected_point.py != projected_vertices[previous_vertex_index].py) {
						polyinfo_emit_point(
							&output_point_index,
							&common_clip_flags, &projected_point);
						polygon_vertex_count++;
					}
				}
			} else {
				if (vertex_clip_flags[previous_vertex_index] != 0) {
					vector_interpolate_at_z(&view_vertices[vertex_index],
						&view_vertices[previous_vertex_index], &scratch_vector,
						SHAPE3D_NEAR_CLIP_Z);
					vector_to_point(&scratch_vector, &projected_point);
					if (projected_point.px != projected_vertices[vertex_index].px ||
						projected_point.py != projected_vertices[vertex_index].py) {
						polyinfo_emit_point(
							&output_point_index,
							&common_clip_flags, &projected_point);
						polygon_vertex_count = LEGACY_U16_WRAP_ADD(
							polygon_vertex_count, 1U);
					}
				}
				polyinfo_emit_point(&output_point_index,
					&common_clip_flags, polyvertpointptrtab[i]);
				polygon_vertex_count++;
			}
			previous_vertex_index = vertex_index;
		}
		transshapenumvertscopy = polygon_vertex_count;
	}

	if (transshapenumvertscopy != 0 && common_clip_flags == 0) {
		if ((primitive_flags &
			SHAPE3D_PRIMITIVE_ALWAYS_VISIBLE_FLAG) != 0 ||
			((legacy_u32)front_facing_mask & LEGACY_READ_U32_LE(front_facing_masks)) != 0UL ||
			polyinfo_is_facing_camera(transshapepolyinfo) != 0) {
			primitive_visible = LEGACY_U16_WRAP_ADD(primitive_visible, 1U);
		}
		if (primitive_visible != 0 &&
			(transshapeflags & SHAPE3D_USE_BOUNDING_RECT_FLAG) != 0) {
			for (polygon_vertex_count = 0;
				polygon_vertex_count < transshapenumvertscopy;
				polygon_vertex_count = LEGACY_U16_WRAP_ADD(
					polygon_vertex_count, 1U)) {
				polyinfo_read_point(transshapepolyinfo,
					polygon_vertex_count, &projected_point);
				point_x = projected_point.px;
				point_y = projected_point.py;
				if (point_x < transshaperectptr->left)
					transshaperectptr->left = point_x;
				if (transshaperectptr->right < point_x + 1)
					transshaperectptr->right = point_x + 1;
				if (transshaperectptr->top > point_y)
					transshaperectptr->top = point_y;
				if (transshaperectptr->bottom < point_y + 1)
					transshaperectptr->bottom = point_y + 1;
			}
		}
	}
	} else if (primitive_type == RENDER_PRIMITIVE_LINE) {
	vertex_index_or_depth = transshapeprimitives[0];
	vertex_index_or_radius = transshapeprimitives[1];
	if (vertex_clip_flags[vertex_index_or_depth] + vertex_clip_flags[vertex_index_or_radius] != 2) {
		if (vertex_clip_flags[vertex_index_or_depth] != 0) {
			vector_interpolate_at_z(&view_vertices[vertex_index_or_radius], &view_vertices[vertex_index_or_depth],
				&scratch_vector, SHAPE3D_NEAR_CLIP_Z);
			vertex_radius_or_sort_flag = vertex_index_or_depth;
			vector_to_point(&scratch_vector, &projected_vertices[vertex_radius_or_sort_flag]);
		} else if (vertex_clip_flags[vertex_index_or_radius] != 0) {
			vector_interpolate_at_z(&view_vertices[vertex_index_or_depth], &view_vertices[vertex_index_or_radius],
				&scratch_vector, SHAPE3D_NEAR_CLIP_Z);
			vertex_radius_or_sort_flag = vertex_index_or_radius;
			vector_to_point(&scratch_vector, &projected_vertices[vertex_radius_or_sort_flag]);
		}

		// NOTE: when vertex_index_or_depth and vertex_index_or_radius were negative (ie bogus depth_sum), there
		// was a sorting error with some of the wheels on the Lamborghini LM-002.
		depth_sum = (legacy_s32)LEGACY_S16_WRAP_ADD(
			view_vertices[vertex_index_or_depth].z, view_vertices[vertex_index_or_radius].z);
		polyinfo_write_point(
			transshapepolyinfo, 0U, polyvertpointptrtab[0]);
		polyinfo_write_point(
			transshapepolyinfo, 1U, polyvertpointptrtab[1]);
		if ((transshapeflags & SHAPE3D_USE_BOUNDING_RECT_FLAG) != 0) {
			rect_adjust_from_point(
				polyvertpointptrtab[0], transshaperectptr);
			rect_adjust_from_point(
				polyvertpointptrtab[1], transshaperectptr);
		}
		transshapenumvertscopy = 2;
		primitive_visible = LEGACY_U16_WRAP_ADD(primitive_visible, 1U);
	}
	} else if (primitive_type == RENDER_PRIMITIVE_WHEEL) {
	if (any_vertex_behind == 0) {
		for (i = 0; i < 4; i++) {
			polyinfo_points[i] = *polyvertpointptrtab[i];
			polyinfo_write_point(transshapepolyinfo, (legacy_u16)i,
				&polyinfo_points[i]);
		}
		if (is_facing_camera(polyinfo_points) != 0) {
			depth_sum = LEGACY_S32_SHL((legacy_s32)
				view_vertices[transshapeprimitives[0]].z, 2U);
		} else {
			polyinfo_points[0] = *polyvertpointptrtab[3];
			polyinfo_points[1] = *polyvertpointptrtab[4];
			polyinfo_points[2] = *polyvertpointptrtab[5];
			polyinfo_points[3] = *polyvertpointptrtab[0];
			for (i = 0; i < 4; i++) {
				polyinfo_write_point(transshapepolyinfo, (legacy_u16)i,
					&polyinfo_points[i]);
			}
			depth_sum = LEGACY_S32_SHL((legacy_s32)
				view_vertices[transshapeprimitives[3]].z, 2U);
		}

		vertex_radius_or_sort_flag = polarRadius2D(
			LEGACY_S16_WRAP_SUB(polyinfo_points[0].px,
				polyinfo_points[1].px),
			LEGACY_S16_WRAP_SUB(polyinfo_points[0].py,
				polyinfo_points[1].py));
		vertex_index_or_radius = polarRadius2D(
			LEGACY_S16_WRAP_SUB(polyinfo_points[0].px,
				polyinfo_points[2].px),
			LEGACY_S16_WRAP_SUB(polyinfo_points[0].py,
				polyinfo_points[2].py));
		if (vertex_index_or_radius > vertex_radius_or_sort_flag)
			vertex_radius_or_sort_flag = vertex_index_or_radius;

		if ((transshapeflags & SHAPE3D_USE_BOUNDING_RECT_FLAG) != 0) {
			bounds_point.px = LEGACY_S16_WRAP_SUB(LEGACY_S16_WRAP_SUB(
				polyinfo_points[0].px, vertex_radius_or_sort_flag), 1);
			bounds_point.py = LEGACY_S16_WRAP_SUB(LEGACY_S16_WRAP_SUB(
				polyinfo_points[0].py, vertex_radius_or_sort_flag), 1);
			rect_adjust_from_point(&bounds_point, transshaperectptr);
			bounds_point.px = LEGACY_S16_WRAP_ADD(LEGACY_S16_WRAP_ADD(
				polyinfo_points[0].px, vertex_radius_or_sort_flag), 1);
			bounds_point.py = LEGACY_S16_WRAP_ADD(LEGACY_S16_WRAP_ADD(
				polyinfo_points[0].py, vertex_radius_or_sort_flag), 1);
			rect_adjust_from_point(&bounds_point, transshaperectptr);
			bounds_point.px = LEGACY_S16_WRAP_SUB(LEGACY_S16_WRAP_SUB(
				polyinfo_points[3].px, vertex_radius_or_sort_flag), 1);
			bounds_point.py = LEGACY_S16_WRAP_SUB(LEGACY_S16_WRAP_SUB(
				polyinfo_points[3].py, vertex_radius_or_sort_flag), 1);
			rect_adjust_from_point(&bounds_point, transshaperectptr);
			bounds_point.px = LEGACY_S16_WRAP_ADD(LEGACY_S16_WRAP_ADD(
				polyinfo_points[3].px, vertex_radius_or_sort_flag), 1);
			bounds_point.py = LEGACY_S16_WRAP_ADD(LEGACY_S16_WRAP_ADD(
				polyinfo_points[3].py, vertex_radius_or_sort_flag), 1);
			rect_adjust_from_point(&bounds_point, transshaperectptr);
		}
		transshapenumvertscopy = 4;
		primitive_visible = 1;
	}
	} else if (primitive_type == RENDER_PRIMITIVE_SPHERE) {
	vertex_index_or_depth = transshapeprimitives[0];
	vertex_index_or_radius = transshapeprimitives[1];
//fatal_error("anders: %i %i", vertex_index_or_depth, vertex_index_or_radius);
	depth_sum = (legacy_s32)LEGACY_S16_WRAP_ADD(
		view_vertices[vertex_index_or_depth].z, view_vertices[vertex_index_or_radius].z);
	if (vertex_clip_flags[vertex_index_or_depth] + vertex_clip_flags[vertex_index_or_radius] == 0) {
		polyinfo_write_point(
			transshapepolyinfo, 0U, polyvertpointptrtab[0]);
		transformed_vector = view_vertices[vertex_index_or_depth];
		sphere_radius_endpoint = view_vertices[vertex_index_or_radius];
		scratch_vector.x = LEGACY_S16_WRAP_SUB(transformed_vector.x, sphere_radius_endpoint.x);
		scratch_vector.y = LEGACY_S16_WRAP_SUB(transformed_vector.y, sphere_radius_endpoint.y);
		scratch_vector.z = LEGACY_S16_WRAP_SUB(transformed_vector.z, sphere_radius_endpoint.z);
		sphere_screen_radius = projection_scale_x_wrapped(
			polarRadius3D(&scratch_vector), transformed_vector.z);
		polyinfo_write_word(transshapepolyinfo, 5U, sphere_screen_radius);
		if ((transshapeflags & SHAPE3D_USE_BOUNDING_RECT_FLAG) != 0) {
			bounds_point.py = LEGACY_S16_WRAP_SUB(
				polyvertpointptrtab[0]->py, sphere_screen_radius);
			bounds_point.px = LEGACY_S16_WRAP_SUB(
				polyvertpointptrtab[0]->px, sphere_screen_radius);
			rect_adjust_from_point(&bounds_point, transshaperectptr);
			bounds_point.py = LEGACY_S16_WRAP_ADD(
				polyvertpointptrtab[0]->py, sphere_screen_radius);
			bounds_point.px = LEGACY_S16_WRAP_ADD(
				polyvertpointptrtab[0]->px, sphere_screen_radius);
			rect_adjust_from_point(&bounds_point, transshaperectptr);
		}
		transshapenumvertscopy = 2;
		primitive_visible = LEGACY_U16_WRAP_ADD(primitive_visible, 1U);
	}
	} else if (primitive_type == RENDER_PRIMITIVE_POINT) {
	vertex_index_or_depth = transshapeprimitives[0];
	if (vertex_clip_flags[vertex_index_or_depth] == 0) {
		depth_sum = view_vertices[vertex_index_or_depth].z;
		polyinfo_write_point(
			transshapepolyinfo, 0U, polyvertpointptrtab[0]);
		if ((transshapeflags & SHAPE3D_USE_BOUNDING_RECT_FLAG) != 0) {
			rect_adjust_from_point(
				polyvertpointptrtab[0], transshaperectptr);
		}
		transshapenumvertscopy = 1;
		primitive_visible = LEGACY_U16_WRAP_ADD(primitive_visible, 1U);
	}
	}
	}
	}

	transshapeprimitives = transshapeprimptr;
	visibility_masks += 4U;
	front_facing_masks += 4U;
	if (primitive_visible != 0) {
	queued_primitive_count = LEGACY_U16_WRAP_ADD(queued_primitive_count, 1U);
	transshapepolyinfo[3] = transshapenumvertscopy;
	transshapepolyinfo[4] = primitive_type;
	if (transprimitivepaintjob == BACKLIGHT_PAINT_DEFAULT) {
		transshapepolyinfo[2] = backlights_paint_override;
	} else {
		transshapepolyinfo[2] = transprimitivepaintjob;
	}

	vertex_index_or_depth = shape3d_average_depth(depth_sum, transshapenumvertscopy);

	polyinfo_write_word(transshapepolyinfo, 0U, vertex_index_or_depth);

	if ((transshapeflags & SHAPE3D_NO_DEPTH_SORT_FLAG) != 0 ||
		(primitive_flags & SHAPE3D_PRIMITIVE_SKIP_DEPTH_SORT_FLAG) != 0) {
		vertex_radius_or_sort_flag = 0;
	} else
		vertex_radius_or_sort_flag = 1;

	polygon_buffer_full = polygon_insert_newest(vertex_index_or_depth, vertex_radius_or_sort_flag);
	if (polygon_buffer_full != 0)
		return 1;
	} else if ((primitive_flags &
		SHAPE3D_PRIMITIVE_SKIP_DEPTH_SORT_FLAG) == 0) {
		while ((transshapeprimitives[1] &
			SHAPE3D_PRIMITIVE_SKIP_DEPTH_SORT_FLAG) != 0) {
			transshapeprimitives +=
				primidxcounttab[transshapeprimitives[0]] +
				transshapenumpaints + 2;
			visibility_masks += 4U;
			front_facing_masks += 4U;
		}
	}

	if (transshapeprimitives[0] == 0)
		return queued_primitive_count != 0 ? 0 : (legacy_u16)-1;
	}
}

// parameter points to a far array of 2d points
legacy_s8 is_facing_camera(struct POINT2D far* pts) {
	legacy_s32 dx0, dy0, dx1, dy1;
	legacy_s32 signed_area;

	dx0 = (legacy_s32)pts[0].px - pts[1].px;
	dx1 = (legacy_s32)pts[2].px - pts[1].px;

	if (dx0 == 0 && dx1 == 0) return 0;

	dy0 = (legacy_s32)pts[0].py - pts[1].py;
	dy1 = (legacy_s32)pts[2].py - pts[1].py;

	if (dy0 == 0 && dy1 == 0) return 0;
	signed_area = (dx1 * dy0) - (dx0 * dy1);
	return signed_area <= 0 ? 0 : 1;
}

extern legacy_u16 projection_horizontal_half_fov;
extern legacy_u16 projection_vertical_half_fov;
extern legacy_u16 projection_half_width;
extern legacy_u16 projection_origin_x;
extern legacy_u16 projection_half_height;
extern legacy_u16 projection_origin_y;

legacy_u16 projection_scale_x_wrapped(legacy_u16 numerator, legacy_s16 divisor) {
	return LEGACY_U16_DIV_OR_ZERO(
		LEGACY_U16_WRAP_MUL(projection_focal_length_x, numerator),
		(legacy_u16)divisor);
}

legacy_u16 divide_u32_to_u16(legacy_u32 dividend, legacy_u16 divisor) {
	return (legacy_u16)LEGACY_U32_DIV_OR_ZERO(dividend, divisor);
}

legacy_u32 projection_multiply_x(legacy_u16 value) {
	return (legacy_u32)projection_focal_length_x * value;
}

legacy_u32 projection_multiply_y(legacy_u16 value) {
	return (legacy_u32)projection_focal_length_y * value;
}

legacy_u16 projection_scale_y(legacy_u16 value, legacy_u16 divisor) {
	return (legacy_u16)LEGACY_U32_DIV_OR_ZERO(
		LEGACY_U32_WRAP_MUL(projection_focal_length_y, value), divisor);
}

extern legacy_u16 polygon_insert_newest(legacy_u16 depth, legacy_u16 sort_by_depth) {
	legacy_s16 next_polygon, remaining_polygons, previous_remaining_count;

	//return ported_insert_newest_poly_in_poly_linked_list_40ED6_(depth, sort_by_depth);

	if (sort_by_depth == 0) {
		next_polygon = polygon_next_index[polygon_insertion_cursor];
	} else {
		polygon_insertion_cursor = shape_polygon_predecessor;
		next_polygon = polygon_next_index[shape_polygon_predecessor];
		remaining_polygons = shape_polygon_count;

		while (next_polygon >= 0) {
			previous_remaining_count = remaining_polygons;
			remaining_polygons--;
			if (previous_remaining_count == 0) break;
			if (LEGACY_READ_S16_LE(polyinfoptrs[next_polygon]) <
				(legacy_s16)depth) break;
			polygon_insertion_cursor = next_polygon;
			next_polygon = polygon_next_index[next_polygon];
		}
	}

	polygon_next_index[polyinfonumpolys] = next_polygon;
	polygon_next_index[polygon_insertion_cursor] = polyinfonumpolys;
	shape_polygon_count = LEGACY_U16_WRAP_ADD(
		shape_polygon_count, 1U);
	if (next_polygon < 0) {
		polygon_list_tail = polyinfonumpolys;
	}
	polygon_insertion_cursor = polygon_next_index[polygon_insertion_cursor];
	polyinfonumpolys = LEGACY_U16_WRAP_ADD(polyinfonumpolys, 1U);
	polyinfoptrnext = LEGACY_U16_WRAP_ADD(polyinfoptrnext,
		LEGACY_U16_WRAP_ADD(LEGACY_U16_WRAP_MUL(
			transshapenumvertscopy, sizeof(struct POINT2D)), 6U));
	if (polyinfonumpolys == POLYINFO_LIST_CAPACITY) return 1;
	if (polyinfoptrnext <= POLYINFO_DATA_LAST_VALID_OFFSET) return 0;
	return 1;
}

static legacy_u16 projection_angle_from_extent(legacy_s16 extent)
{
	legacy_s32 scaled;
	legacy_s32 quotient;

	scaled = LEGACY_S32_WRAP_MUL((legacy_s32)extent,
		PROJECTION_EXTENT_SCALE);
	quotient = LEGACY_S32_DIV_OR_ZERO(scaled,
		PROJECTION_EXTENT_DIVISOR);
	return (legacy_u16)LEGACY_S32_SAR(quotient, 1U);
}

static legacy_u16 projection_scale_for_angle(legacy_u16 angle,
	legacy_u16 extent)
{
	legacy_s32 product;
	legacy_s32 quotient;

	product = LEGACY_S32_WRAP_MUL(
		(legacy_s32)cos_fast(angle), (legacy_s32)extent);
	quotient = LEGACY_S32_DIV_OR_ZERO(
		product, (legacy_s32)sin_fast(angle));
	return (legacy_u16)quotient;
}

static void projection_update_derived(void)
{
	projection_center_x = LEGACY_U16_WRAP_ADD(
		projection_half_width, projection_origin_x);
	projection_center_y = LEGACY_U16_WRAP_ADD(
		projection_half_height, projection_origin_y);
	projection_focal_length_x = projection_scale_for_angle(
		projection_horizontal_half_fov, projection_half_width);
	if (projection_vertical_half_fov != 0) {
		projection_focal_length_y = projection_scale_for_angle(
			projection_vertical_half_fov, projection_half_height);
	} else {
		projection_focal_length_y = LEGACY_U16_WRAP_SUB(LEGACY_U16_WRAP_SUB(
			projection_focal_length_x, projection_focal_length_x >> 3),
			projection_focal_length_x >> 4);
		projection_vertical_half_fov = polarAngle(projection_focal_length_y, projection_half_height);
	}
}

void set_projection(legacy_s16 horizontal_fov_degrees, legacy_s16 vertical_fov_degrees, legacy_s16 width, legacy_s16 height) {

	projection_horizontal_half_fov = projection_angle_from_extent(horizontal_fov_degrees);
	projection_vertical_half_fov = projection_angle_from_extent(vertical_fov_degrees);
	projection_half_width = (legacy_u16)LEGACY_S16_SAR(width, 1U);
	projection_half_height = (legacy_u16)LEGACY_S16_SAR(height, 1U);
	projection_update_derived();
}

void projection_set_origin(legacy_u16 x, legacy_u16 y) {
	projection_origin_x = x;
	projection_center_x = LEGACY_U16_WRAP_ADD(projection_half_width, x);
	projection_origin_y = y;
	projection_center_y = LEGACY_U16_WRAP_ADD(projection_half_height, y);
}

void projection_set_half_fov(legacy_u16 horizontal_half_fov, legacy_u16 vertical_half_fov, legacy_u16 width, legacy_u16 height) {
	projection_horizontal_half_fov = horizontal_half_fov;
	projection_vertical_half_fov = vertical_half_fov;
	projection_half_width = width >> 1;
	projection_half_height = height >> 1;
	projection_update_derived();
}

//extern unsigned word_411F6;
extern struct MATRIX mat_y0, mat_y100, mat_y200, mat_y300;
extern legacy_s32 direction_sector_sine_copy;
extern legacy_s32 direction_sector_cosine_copy;

legacy_u16 select_cliprect_rotate(legacy_s16 angZ, legacy_s16 angX, legacy_s16 angY, struct RECTANGLE* cliprect, legacy_s16 half_scale) {
	struct MATRIX* inverse_view_rotation;
	struct VECTOR forward_axis, view_direction;

	//return ported_select_cliprect_rotate_(angX, angY, angZ, cliprect, half_scale);

	mat_temp = *mat_rot_zxy(angZ, angX, angY,
		MATRIX_ROTATION_ORDER_YXZ);
	polyinfo_reset();
	select_rect_rc = *cliprect;
	shape_half_scale = half_scale;
	inverse_view_rotation = mat_rot_zxy(-angZ, -angX, -angY,
		MATRIX_ROTATION_ORDER_ZXY);
	forward_axis.z = PROJECTION_VIEW_VECTOR_LENGTH;
	forward_axis.y = 0;
	forward_axis.x = 0;
	mat_mul_vector(&forward_axis, inverse_view_rotation, &view_direction);
	return polarAngle(view_direction.x, view_direction.z) & PROJECTION_YAW_MASK;
}

void polyinfo_reset(void) {
	polyinfonumpolys = 0;
	polyinfoptrnext = 0;
	polygon_buffer_full = 0;
	polygon_next_index[POLYINFO_LIST_CAPACITY] =
		POLYINFO_LIST_SENTINEL;
	polygon_list_tail = POLYINFO_LIST_CAPACITY;
}

void init_direction_sector_thresholds(void) {
	direction_sector_sine = sin_fast(PROJECTION_SINCOS_ANGLE);
	direction_sector_cosine = cos_fast(PROJECTION_SINCOS_ANGLE);
	direction_sector_sine_copy = sin_fast(PROJECTION_SINCOS_ANGLE);
	direction_sector_cosine_copy = cos_fast(PROJECTION_SINCOS_ANGLE);
}

void init_polyinfo(void) {
	polyinfoptr = mmgr_alloc_resbytes("polyinfo",
		POLYINFO_DATA_ALLOCATION_SIZE);

	mat_rot_y(&mat_y0, 0);
	mat_rot_y(&mat_y100, ANGLE_QUARTER_TURN);
	mat_rot_y(&mat_y200, ANGLE_HALF_TURN);
	mat_rot_y(&mat_y300, ANGLE_THREE_QUARTER_TURN);
	init_direction_sector_thresholds();
}

void shape3d_render_queued_primitives(void)
{
	legacy_u8 far* record;
	struct POINT2D points[POLYINFO_MAX_RENDER_POINTS];
	legacy_u16 record_index;
	legacy_u16 primitive_index;
	legacy_u16 material_type;
	legacy_u16 material_color;
	legacy_u16 primitive_type;
	legacy_u16 vertex_count;
	legacy_u16 pattern_type;

	record_index = POLYINFO_LIST_CAPACITY;
	for (primitive_index = 0; primitive_index < polyinfonumpolys;
		primitive_index++) {
		record_index = (legacy_u16)polygon_next_index[record_index];
		record = polyinfoptrs[record_index];
		material_type = record[2];
		material_color = (legacy_u16)
			material_clrlist_ptr_cpy[material_type];
		primitive_type = record[4];

		if (primitive_type == RENDER_PRIMITIVE_POLYGON) {
			vertex_count = record[3];
			polyinfo_read_points(record, points, vertex_count);
			pattern_type = (legacy_u16)
				material_patlist_ptr_cpy[material_type];
			if (pattern_type == 0U) {
				preRender_default(material_color, vertex_count, points);
			} else if (pattern_type == 1U) {
				pattern_type = (legacy_u16)
					material_patlist2_ptr_cpy[material_type];
				if (pattern_type != 0U)
					preRender_patterned(pattern_type, material_color,
						vertex_count, points);
			} else if (pattern_type == 2U) {
				preRender_two_color((legacy_u16)
					material_patlist2_ptr_cpy[material_type],
					(legacy_u16)material_clrlist2_ptr_cpy[material_type],
					material_color, vertex_count,
						points);
			}
		} else if (primitive_type == RENDER_PRIMITIVE_LINE) {
			preRender_line(polyinfo_read_word(record, 3U),
				polyinfo_read_word(record, 4U),
				polyinfo_read_word(record, 5U),
				polyinfo_read_word(record, 6U), material_color);
		} else if (primitive_type == RENDER_PRIMITIVE_SPHERE) {
			preRender_sphere(LEGACY_S16_FROM_BITS(
				polyinfo_read_word(record, 3U)),
				LEGACY_S16_FROM_BITS(polyinfo_read_word(record, 4U)),
				polyinfo_read_word(record, 5U), material_color);
		} else if (primitive_type == RENDER_PRIMITIVE_WHEEL) {
			polyinfo_read_points(record, points, 4U);
			preRender_wheel(points, WHEEL_INNER_RADIUS_SCALE, material_color,
				(legacy_u16)material_clrlist_ptr_cpy[material_type + 1U],
				(legacy_u16)material_clrlist_ptr_cpy[material_type + 2U]);
		} else if (primitive_type == RENDER_PRIMITIVE_POINT) {
			sprite_putpixel_clipped(
				LEGACY_S16_FROM_BITS(polyinfo_read_word(record, 3U)),
				LEGACY_S16_FROM_BITS(polyinfo_read_word(record, 4U)),
				material_color);
		}
	}
	polyinfo_reset();
}

// generate_poly_edges is called preRender_helper in the IDB.
// aka preRender_helper3 in the IDB
