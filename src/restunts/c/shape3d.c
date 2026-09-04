#include <stddef.h>
#include <limits.h>
#include "externs.h"
#include "fileio.h"
#include "legacy.h"
#include "memmgr.h"
#include "shape3d.h"
#include "shape3d_internal.h"
#include "shape2d.h"
#include "shape2d_internal.h"

/*

TODO:

  lines function (calls)
- ----- --------- -------
X   159 mat_rot_zxy (16)
X    60 mat_multiply (0)
X   122 mat_mul_vector (0)
X    48 mat_invert (0)
X   126 vector_op_unk2 (14)
X    97 vector_to_point (0)
X    32 rect_compare_point (0)
X    40 vector_op_unk (0)
X    86 is_facing_camera (4)
X    33 rect_adjust_from_point(0)
X    47 polarRadius2D (6)
X    13 polarRadius3D (4)
X     7 projectiondata9_times_ratio (0)
X    74 insert_newest_poly_in_poly_linked_list_40ED6 (0)
X     - polarAngle (0)
X     - mat_rot_z (4)
X     - mat_rot_x (4)
X     - mat_rot_y (4)
X     - set_projection (10)
X     - select_cliprect_rotate (10)

*/

extern legacy_s8 far* game1ptr;
extern legacy_s8 far* game2ptr;
extern legacy_s8 far* curshapeptr;

extern legacy_u32 mmgr_get_res_ofs_diff_scaled(void);

extern legacy_s8 aBarn[];

void shape3d_vertex_read(const struct SHAPE3D* shape, legacy_u16 index,
	struct VECTOR* destination)
{
	const legacy_u8 far* source;

	source = shape->shape3d_vertex_bytes +
		LEGACY_U16_WRAP_MUL(index, 6U);
	destination->x = LEGACY_READ_S16_LE(source);
	destination->y = LEGACY_READ_S16_LE(source + 2U);
	destination->z = LEGACY_READ_S16_LE(source + 4U);
}

void shape3d_vertex_write(struct SHAPE3D* shape, legacy_u16 index,
	const struct VECTOR* source)
{
	legacy_u8 far* destination;

	destination = shape->shape3d_vertex_bytes +
		LEGACY_U16_WRAP_MUL(index, 6U);
	LEGACY_WRITE_U16_LE(destination, (legacy_u16)source->x);
	LEGACY_WRITE_U16_LE(destination + 2U, (legacy_u16)source->y);
	LEGACY_WRITE_U16_LE(destination + 4U, (legacy_u16)source->z);
}

extern legacy_s8 is_facing_camera(struct POINT2D far*);
extern legacy_u16 insert_newest_poly_in_poly_linked_list_40ED6(legacy_u16, legacy_u16);
extern legacy_u16 projectiondata9_times_ratio(legacy_u16, legacy_s16);

extern legacy_u16 word_40ECE;
extern legacy_u16 transshapenumverts;
extern legacy_u8 far* transshapeprimitives;
extern legacy_u16 transshapenumpaints;
extern legacy_u8 transshapematerial;
extern legacy_u8 transshapeflags;
extern struct RECTANGLE* transshaperectptr;
extern struct MATRIX mat_temp;
extern legacy_s32 invpow2tbl[32];
extern legacy_u8 byte_4393D;

// Four iterators in the linked list. Their roles are just the best guess
// Starting index for some kinds of scans
extern legacy_u16 poly_linklist_40ED6_iter1;
// Unknown
extern legacy_u16 poly_linklist_40ED6_iter2;
// Scan counter
extern legacy_u16 poly_linklist_40ED6_iter3;
// After insertion, contains the index of the newly inserted primitive
extern legacy_u16 poly_linklist_40ED6_iter4;

extern legacy_u8 transshapenumvertscopy;
extern struct POINT2D* polyvertpointptrtab[];
extern legacy_u16 select_rect_param;
extern legacy_u8 primidxcounttab[];
extern legacy_u8 primtypetab[];
extern legacy_u8 far* transshapeprimptr;
extern legacy_u16 polyinfoptrnext;
extern legacy_u8 far* polyinfoptr;
extern legacy_u8 far* transshapepolyinfo;
extern legacy_u8 far* polyinfoptrs[];
extern legacy_u16 polyinfonumpolys;
extern legacy_s8 transprimitivepaintjob;
extern legacy_u8 far* transshapeprimindexptr;
extern legacy_s8 backlights_paint_override;

/* 14-bit fixed point: the inner radius is 37/64 of the outer radius. */
#define WHEEL_INNER_RADIUS_SCALE 0x2500U

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

legacy_u16 transformed_shape_op(struct TRANSFORMEDSHAPE3D* arg_transshapeptr) {
	legacy_u8 far* var_cull1;
	legacy_u8 far* var_cull2;

	legacy_u8 var_vertflagtbl[256];
	struct MATRIX* var_rotmatptr;
	struct MATRIX var_mat;
	struct MATRIX var_mat2;
	struct VECTOR var_vec;
	struct VECTOR var_vec2;
	struct VECTOR var_vec3;
	struct VECTOR var_vec4;
	legacy_s32 var_45C;
	legacy_s32 var_A;
	legacy_u16 var_45E, var_460, var_1A;
	legacy_u8 var_ptrectflag, var_primtype;
	struct VECTOR var_vecarr[255];
	legacy_u16 var_primitiveflags, var_fileprimtype, var_4;
	legacy_u16 var_polyvertcounter, var_C, var_448, var_462;
	legacy_s16 var_polyvertX, var_polyvertY;
	legacy_u16 var_transshapepolyinfoptindex;
	legacy_s32 var_18;
	struct POINT2D var_574, var_450;
	struct POINT2D polyinfo_points[4];
	struct POINT2D var_vecarr2[255];
	struct POINT2D** var_polyvertunktabptr;



	legacy_u16 i;
	legacy_u16 temp, temp0, temp1;

	//result = ported_transformed_shape_op_(arg_transshapeptr);
	//return result;

	if (word_40ECE != 0) return 1;
	transshapenumverts = arg_transshapeptr->shapeptr->shape3d_numverts;
	/* Shape files store this count in one byte.  Reject a damaged descriptor
	 * before it can overrun the fixed-size transformation work arrays. */
	if (transshapenumverts > 0xFFU)
		return 1;
	transshapeprimitives = arg_transshapeptr->shapeptr->shape3d_primitives;
	transshapenumpaints = arg_transshapeptr->shapeptr->shape3d_numpaints;
	var_cull1 = arg_transshapeptr->shapeptr->shape3d_cull1;
	var_cull2 = arg_transshapeptr->shapeptr->shape3d_cull2;
	transshapematerial = arg_transshapeptr->material;
	if (transshapematerial >= transshapenumpaints)
		transshapematerial = 0;
	transshapeflags = arg_transshapeptr->ts_flags;

	if ((transshapeflags & 8) != 0) {
		transshaperectptr = arg_transshapeptr->rectptr;
	}

	for (i = 0; i < transshapenumverts; i++) {
		var_vertflagtbl[i] = 0xff;
	}

	if ((transshapeflags & 2) == 0) {
		var_rotmatptr = mat_rot_zxy(arg_transshapeptr->rotvec.x, arg_transshapeptr->rotvec.y, arg_transshapeptr->rotvec.z, 0);
		mat_mul_vector(&arg_transshapeptr->pos, &mat_temp, &var_vec);
		mat_multiply(var_rotmatptr, &mat_temp, &var_mat2);
		mat_invert(&var_mat2, &var_mat);
		var_vec2.x = 0;
		var_vec2.y = 0;
		var_vec2.z = 0x1000;
		mat_mul_vector(&var_vec2, &var_mat, &var_vec3);
		if ((var_vec3.y <= 0 || arg_transshapeptr->pos.y >= 0) &&
			(LEGACY_S16_SHL(arg_transshapeptr->unk, 1U) <=
				absolute_word(var_vec.x) ||
			LEGACY_S16_SHL(arg_transshapeptr->unk, 1U) <=
				absolute_word(var_vec.z))) {
			byte_4393D = vector_op_unk2(&var_vec3);
			var_45C = invpow2tbl[byte_4393D];
			var_A = invpow2tbl[byte_4393D];
		} else {
			var_45C = -1;
			var_A = 0;
		}
	} else {
		var_rotmatptr = mat_rot_zxy(arg_transshapeptr->rotvec.x, arg_transshapeptr->rotvec.y, arg_transshapeptr->rotvec.z, 0);
		mat_multiply(var_rotmatptr, &mat_temp, &var_mat2);
		var_vec = arg_transshapeptr->pos;
		var_45C = -1;
		var_A = 0;
	}

	poly_linklist_40ED6_iter1 = poly_linklist_40ED6_iter2;
	poly_linklist_40ED6_iter4 = poly_linklist_40ED6_iter2;
	poly_linklist_40ED6_iter3 = 0;
	var_45E = 0;

	if (transshapenumverts <= 8) {
		transshapenumvertscopy = transshapenumverts;
	} else {
		transshapenumvertscopy = 8;
	}

	if (transshapenumvertscopy > 4) {
		shape3d_vertex_read(arg_transshapeptr->shapeptr, 0U, &var_vec2);
		shape3d_vertex_read(arg_transshapeptr->shapeptr, 4U, &var_vec3);
		if (var_vec2.y == var_vec3.y)
			transshapenumvertscopy = 4;
	}

	var_ptrectflag = 0x0f;
	var_460 = 1;
	var_1A = 0;
	for (i = 0; i < transshapenumvertscopy;
		i = LEGACY_U16_WRAP_ADD(i, 1U)) {
		polyvertpointptrtab[i] = &var_vecarr2[i];
		shape3d_transform_vertex(arg_transshapeptr->shapeptr, i,
			select_rect_param, &var_mat2, &var_vec, &var_vec3);
		var_vecarr[i] = var_vec3;
		if (var_vec3.z < 0x0C) {
			var_vertflagtbl[i] = 1;
			var_1A = 1;
			continue;
		}
		var_460 = 0;
		var_vertflagtbl[i] = 0;
		vector_to_point(&var_vec3, polyvertpointptrtab[i]);
		if (var_ptrectflag != 0)
			var_ptrectflag &= rect_compare_point(polyvertpointptrtab[i]);
		if (var_ptrectflag == 0)
			break;
	}
	if (i == transshapenumvertscopy &&
		(var_460 != 0 || var_1A == 0 ||
		LEGACY_S16_FROM_BITS(arg_transshapeptr->unk) <
			absolute_word(var_vec.x))) {
		return (legacy_u16)-1;
	}

	transshapeprimitives = arg_transshapeptr->shapeptr->shape3d_primitives;



	for (;;) {
	transshapeprimptr = transshapeprimitives + primidxcounttab[transshapeprimitives[0]] + transshapenumpaints + 2;
	var_primitiveflags = transshapeprimitives[1];
	var_4 = 0;
	if ((LEGACY_READ_U32_LE(var_cull1) & (legacy_u32)var_45C) != 0UL) {

	var_fileprimtype = transshapeprimitives[0];
	transshapenumvertscopy = primidxcounttab[var_fileprimtype];
	var_primtype = primtypetab[var_fileprimtype];

	transshapepolyinfo = polyinfoptr + polyinfoptrnext;
	polyinfoptrs[polyinfonumpolys] = transshapepolyinfo;

	transprimitivepaintjob = transshapeprimitives[2 + transshapematerial];
	transshapeprimitives += 2 + transshapenumpaints; // <- skip header and materials, -> point at indices

	var_ptrectflag = 0x0f;
	var_460 = 1;
	var_1A = 0;
	transshapeprimindexptr = transshapeprimitives;
	var_polyvertcounter = 0;
	while (var_polyvertcounter < transshapenumvertscopy) {
		temp = transshapeprimindexptr[0];
		transshapeprimindexptr++;
		polyvertpointptrtab[var_polyvertcounter] = &var_vecarr2[temp];

		if (var_vertflagtbl[temp] == 0xFFU) {
			shape3d_transform_vertex(arg_transshapeptr->shapeptr,
				temp, select_rect_param, &var_mat2, &var_vec, &var_vec3);
			var_vecarr[temp] = var_vec3;
			if (var_vec3.z >= 0x0C) {
				var_460 = 0;
				var_vertflagtbl[temp] = 0;
				vector_to_point(&var_vec3,
					polyvertpointptrtab[var_polyvertcounter]);
			} else {
				var_vertflagtbl[temp] = 1;
				var_1A = 1;
			}
		} else if (var_vertflagtbl[temp] == 0) {
			var_460 = 0;
		} else if (var_vertflagtbl[temp] == 1) {
			var_1A = 1;
		}

		if (var_vertflagtbl[temp] == 0 && var_ptrectflag != 0) {
			var_ptrectflag &= rect_compare_point(
				polyvertpointptrtab[var_polyvertcounter]);
		}
		var_polyvertcounter = LEGACY_U16_WRAP_ADD(
			var_polyvertcounter, 1U);
	}

	if (var_460 == 0 && (var_ptrectflag == 0 || var_1A != 0)) {
	if (var_primtype == 0) {
	var_transshapepolyinfoptindex = 0U;
	transshapeprimindexptr = transshapeprimitives;
	var_18 = 0;
	var_ptrectflag = 0x0f;
	if (var_1A == 0) {
		for (i = 0; i < transshapenumvertscopy; i++) {
			var_C = transshapeprimindexptr[0];
			transshapeprimindexptr++;
			var_18 = LEGACY_S32_WRAP_ADD_S16(
				var_18, var_vecarr[var_C].z);
			var_polyvertunktabptr = &polyvertpointptrtab[i];
			polyinfo_emit_point(&var_transshapepolyinfoptindex,
				&var_ptrectflag, *var_polyvertunktabptr);
		}
	} else {
		var_polyvertcounter = 0;
		var_448 = transshapeprimitives[transshapenumvertscopy - 1];
		for (i = 0; i < transshapenumvertscopy;
			i = LEGACY_U16_WRAP_ADD(i, 1U)) {
			var_C = transshapeprimindexptr[0];
			transshapeprimindexptr++;
			var_18 = LEGACY_S32_WRAP_ADD_S16(
				var_18, var_vecarr[var_C].z);

			if (var_vertflagtbl[var_C] != 0) {
				if (var_vertflagtbl[var_448] == 0) {
					vector_op_unk(&var_vecarr[var_448],
						&var_vecarr[var_C], &var_vec2, 0x0C);
					vector_to_point(&var_vec2, &var_574);
					if (var_574.px != var_vecarr2[var_448].px ||
						var_574.py != var_vecarr2[var_448].py) {
						polyinfo_emit_point(
							&var_transshapepolyinfoptindex,
							&var_ptrectflag, &var_574);
						var_polyvertcounter++;
					}
				}
			} else {
				if (var_vertflagtbl[var_448] != 0) {
					vector_op_unk(&var_vecarr[var_C],
						&var_vecarr[var_448], &var_vec2, 0x0C);
					vector_to_point(&var_vec2, &var_574);
					if (var_574.px != var_vecarr2[var_C].px ||
						var_574.py != var_vecarr2[var_C].py) {
						polyinfo_emit_point(
							&var_transshapepolyinfoptindex,
							&var_ptrectflag, &var_574);
						var_polyvertcounter = LEGACY_U16_WRAP_ADD(
							var_polyvertcounter, 1U);
					}
				}
				polyinfo_emit_point(&var_transshapepolyinfoptindex,
					&var_ptrectflag, polyvertpointptrtab[i]);
				var_polyvertcounter++;
			}
			var_448 = var_C;
		}
		transshapenumvertscopy = var_polyvertcounter;
	}

	if (transshapenumvertscopy != 0 && var_ptrectflag == 0) {
		if ((var_primitiveflags & 1) != 0 ||
			((legacy_u32)var_A & LEGACY_READ_U32_LE(var_cull2)) != 0UL ||
			polyinfo_is_facing_camera(transshapepolyinfo) != 0) {
			var_4 = LEGACY_U16_WRAP_ADD(var_4, 1U);
		}
		if (var_4 != 0 && (transshapeflags & 8) != 0) {
			for (var_polyvertcounter = 0;
				var_polyvertcounter < transshapenumvertscopy;
				var_polyvertcounter = LEGACY_U16_WRAP_ADD(
					var_polyvertcounter, 1U)) {
				polyinfo_read_point(transshapepolyinfo,
					var_polyvertcounter, &var_574);
				var_polyvertX = var_574.px;
				var_polyvertY = var_574.py;
				if (var_polyvertX < transshaperectptr->left)
					transshaperectptr->left = var_polyvertX;
				if (transshaperectptr->right < var_polyvertX + 1)
					transshaperectptr->right = var_polyvertX + 1;
				if (transshaperectptr->top > var_polyvertY)
					transshaperectptr->top = var_polyvertY;
				if (transshaperectptr->bottom < var_polyvertY + 1)
					transshaperectptr->bottom = var_polyvertY + 1;
			}
		}
	}
	} else if (var_primtype == 1) {
	temp0 = transshapeprimitives[0];
	temp1 = transshapeprimitives[1];
	if (var_vertflagtbl[temp0] + var_vertflagtbl[temp1] != 2) {
		if (var_vertflagtbl[temp0] != 0) {
			vector_op_unk(&var_vecarr[temp1], &var_vecarr[temp0],
				&var_vec2, 0x0C);
			temp = temp0;
			vector_to_point(&var_vec2, &var_vecarr2[temp]);
		} else if (var_vertflagtbl[temp1] != 0) {
			vector_op_unk(&var_vecarr[temp0], &var_vecarr[temp1],
				&var_vec2, 0x0C);
			temp = temp1;
			vector_to_point(&var_vec2, &var_vecarr2[temp]);
		}

		// NOTE: when temp0 and temp1 were negative (ie bogus var_18), there
		// was a sorting error with some of the wheels on the Lamborghini LM-002.
		var_18 = (legacy_s32)LEGACY_S16_WRAP_ADD(
			var_vecarr[temp0].z, var_vecarr[temp1].z);
		polyinfo_write_point(
			transshapepolyinfo, 0U, polyvertpointptrtab[0]);
		polyinfo_write_point(
			transshapepolyinfo, 1U, polyvertpointptrtab[1]);
		if ((transshapeflags & 8) != 0) {
			rect_adjust_from_point(
				polyvertpointptrtab[0], transshaperectptr);
			rect_adjust_from_point(
				polyvertpointptrtab[1], transshaperectptr);
		}
		transshapenumvertscopy = 2;
		var_4 = LEGACY_U16_WRAP_ADD(var_4, 1U);
	}
	} else if (var_primtype == 3) {
	if (var_1A == 0) {
		for (i = 0; i < 4; i++) {
			polyinfo_points[i] = *polyvertpointptrtab[i];
			polyinfo_write_point(transshapepolyinfo, (legacy_u16)i,
				&polyinfo_points[i]);
		}
		if (is_facing_camera(polyinfo_points) != 0) {
			var_18 = LEGACY_S32_SHL((legacy_s32)
				var_vecarr[transshapeprimitives[0]].z, 2U);
		} else {
			polyinfo_points[0] = *polyvertpointptrtab[3];
			polyinfo_points[1] = *polyvertpointptrtab[4];
			polyinfo_points[2] = *polyvertpointptrtab[5];
			polyinfo_points[3] = *polyvertpointptrtab[0];
			for (i = 0; i < 4; i++) {
				polyinfo_write_point(transshapepolyinfo, (legacy_u16)i,
					&polyinfo_points[i]);
			}
			var_18 = LEGACY_S32_SHL((legacy_s32)
				var_vecarr[transshapeprimitives[3]].z, 2U);
		}

		temp = polarRadius2D(
			LEGACY_S16_WRAP_SUB(polyinfo_points[0].px,
				polyinfo_points[1].px),
			LEGACY_S16_WRAP_SUB(polyinfo_points[0].py,
				polyinfo_points[1].py));
		temp1 = polarRadius2D(
			LEGACY_S16_WRAP_SUB(polyinfo_points[0].px,
				polyinfo_points[2].px),
			LEGACY_S16_WRAP_SUB(polyinfo_points[0].py,
				polyinfo_points[2].py));
		if (temp1 > temp)
			temp = temp1;

		if ((transshapeflags & 8) != 0) {
			var_450.px = LEGACY_S16_WRAP_SUB(LEGACY_S16_WRAP_SUB(
				polyinfo_points[0].px, temp), 1);
			var_450.py = LEGACY_S16_WRAP_SUB(LEGACY_S16_WRAP_SUB(
				polyinfo_points[0].py, temp), 1);
			rect_adjust_from_point(&var_450, transshaperectptr);
			var_450.px = LEGACY_S16_WRAP_ADD(LEGACY_S16_WRAP_ADD(
				polyinfo_points[0].px, temp), 1);
			var_450.py = LEGACY_S16_WRAP_ADD(LEGACY_S16_WRAP_ADD(
				polyinfo_points[0].py, temp), 1);
			rect_adjust_from_point(&var_450, transshaperectptr);
			var_450.px = LEGACY_S16_WRAP_SUB(LEGACY_S16_WRAP_SUB(
				polyinfo_points[3].px, temp), 1);
			var_450.py = LEGACY_S16_WRAP_SUB(LEGACY_S16_WRAP_SUB(
				polyinfo_points[3].py, temp), 1);
			rect_adjust_from_point(&var_450, transshaperectptr);
			var_450.px = LEGACY_S16_WRAP_ADD(LEGACY_S16_WRAP_ADD(
				polyinfo_points[3].px, temp), 1);
			var_450.py = LEGACY_S16_WRAP_ADD(LEGACY_S16_WRAP_ADD(
				polyinfo_points[3].py, temp), 1);
			rect_adjust_from_point(&var_450, transshaperectptr);
		}
		transshapenumvertscopy = 4;
		var_4 = 1;
	}
	} else if (var_primtype == 2) {
	temp0 = transshapeprimitives[0];
	temp1 = transshapeprimitives[1];
//fatal_error("anders: %i %i", temp0, temp1);
	var_18 = (legacy_s32)LEGACY_S16_WRAP_ADD(
		var_vecarr[temp0].z, var_vecarr[temp1].z);
	if (var_vertflagtbl[temp0] + var_vertflagtbl[temp1] == 0) {
		polyinfo_write_point(
			transshapepolyinfo, 0U, polyvertpointptrtab[0]);
		var_vec3 = var_vecarr[temp0];
		var_vec4 = var_vecarr[temp1];
		var_vec2.x = LEGACY_S16_WRAP_SUB(var_vec3.x, var_vec4.x);
		var_vec2.y = LEGACY_S16_WRAP_SUB(var_vec3.y, var_vec4.y);
		var_vec2.z = LEGACY_S16_WRAP_SUB(var_vec3.z, var_vec4.z);
		var_462 = projectiondata9_times_ratio(
			polarRadius3D(&var_vec2), var_vec3.z);
		polyinfo_write_word(transshapepolyinfo, 5U, var_462);
		if ((transshapeflags & 8) != 0) {
			var_450.py = LEGACY_S16_WRAP_SUB(
				polyvertpointptrtab[0]->py, var_462);
			var_450.px = LEGACY_S16_WRAP_SUB(
				polyvertpointptrtab[0]->px, var_462);
			rect_adjust_from_point(&var_450, transshaperectptr);
			var_450.py = LEGACY_S16_WRAP_ADD(
				polyvertpointptrtab[0]->py, var_462);
			var_450.px = LEGACY_S16_WRAP_ADD(
				polyvertpointptrtab[0]->px, var_462);
			rect_adjust_from_point(&var_450, transshaperectptr);
		}
		transshapenumvertscopy = 2;
		var_4 = LEGACY_U16_WRAP_ADD(var_4, 1U);
	}
	} else if (var_primtype == 5) {
	temp0 = transshapeprimitives[0];
	if (var_vertflagtbl[temp0] == 0) {
		var_18 = var_vecarr[temp0].z;
		polyinfo_write_point(
			transshapepolyinfo, 0U, polyvertpointptrtab[0]);
		if ((transshapeflags & 8U) != 0) {
			rect_adjust_from_point(
				polyvertpointptrtab[0], transshaperectptr);
		}
		transshapenumvertscopy = 1;
		var_4 = LEGACY_U16_WRAP_ADD(var_4, 1U);
	}
	}
	}
	}

	transshapeprimitives = transshapeprimptr;
	var_cull1 += 4U;
	var_cull2 += 4U;
	if (var_4 != 0) {
	var_45E = LEGACY_U16_WRAP_ADD(var_45E, 1U);
	transshapepolyinfo[3] = transshapenumvertscopy;
	transshapepolyinfo[4] = var_primtype;
	if (transprimitivepaintjob == 0x2D) {
		transshapepolyinfo[2] = backlights_paint_override;
	} else {
		transshapepolyinfo[2] = transprimitivepaintjob;
	}

	temp0 = shape3d_average_depth(var_18, transshapenumvertscopy);

	polyinfo_write_word(transshapepolyinfo, 0U, temp0);


	if ((transshapeflags & 1) != 0 || (var_primitiveflags & 2) != 0) {
		temp = 0;
	} else
		temp = 1;

	word_40ECE = insert_newest_poly_in_poly_linked_list_40ED6(temp0, temp);
	if (word_40ECE != 0)
		return 1;
	} else if ((var_primitiveflags & 2) == 0) {
		while ((transshapeprimitives[1] & 2) != 0) {
			transshapeprimitives +=
				primidxcounttab[transshapeprimitives[0]] +
				transshapenumpaints + 2;
			var_cull1 += 4U;
			var_cull2 += 4U;
		}
	}

	if (transshapeprimitives[0] == 0)
		return var_45E != 0 ? 0 : (legacy_u16)-1;
	}
}




// parameter points to a far array of 2d points
legacy_s8 is_facing_camera(struct POINT2D far* pts) {
	legacy_s32 dx0, dy0, dx1, dy1;
	legacy_s32 temp;

	dx0 = (legacy_s32)pts[0].px - pts[1].px;
	dx1 = (legacy_s32)pts[2].px - pts[1].px;

	if (dx0 == 0 && dx1 == 0) return 0;

	dy0 = (legacy_s32)pts[0].py - pts[1].py;
	dy1 = (legacy_s32)pts[2].py - pts[1].py;

	if (dy0 == 0 && dy1 == 0) return 0;
	temp = (dx1 * dy0) - (dx0 * dy1);
	return temp <= 0 ? 0 : 1;
}

extern legacy_u16 projectiondata1;
extern legacy_u16 projectiondata2;
extern legacy_u16 projectiondata3;
extern legacy_u16 projectiondata4;
extern legacy_u16 projectiondata5;
extern legacy_u16 projectiondata6;
extern legacy_u16 projectiondata7;
extern legacy_u16 projectiondata8;
extern legacy_u16 projectiondata9;
extern legacy_u16 projectiondata10;

legacy_u16 projectiondata9_times_ratio(legacy_u16 i1, legacy_s16 i2) {
	return LEGACY_U16_DIV_OR_ZERO(
		LEGACY_U16_WRAP_MUL(projectiondata9, i1),
		(legacy_u16)i2);
}

legacy_u16 nopsub_32738(legacy_u32 dividend, legacy_u16 divisor) {
	return (legacy_u16)LEGACY_U32_DIV_OR_ZERO(dividend, divisor);
}

legacy_u32 nopsub_32746(legacy_u16 value) {
	return (legacy_u32)projectiondata9 * value;
}

legacy_u32 nopsub_32751(legacy_u16 value) {
	return (legacy_u32)projectiondata10 * value;
}

legacy_u16 nopsub_3276A(legacy_u16 value, legacy_u16 divisor) {
	return (legacy_u16)LEGACY_U32_DIV_OR_ZERO(
		LEGACY_U32_WRAP_MUL(projectiondata10, value), divisor);
}

extern legacy_s16 poly_linked_list_40ED6[];

extern legacy_u16 insert_newest_poly_in_poly_linked_list_40ED6(legacy_u16 arg_0, legacy_u16 arg_2) {
	legacy_s16 regdi, regsi, regax;

	//return ported_insert_newest_poly_in_poly_linked_list_40ED6_(arg_0, arg_2);

	if (arg_2 == 0) {
		regdi = poly_linked_list_40ED6[poly_linklist_40ED6_iter4];
	} else {
		poly_linklist_40ED6_iter4 = poly_linklist_40ED6_iter1;
		regdi = poly_linked_list_40ED6[poly_linklist_40ED6_iter1];
		regsi = poly_linklist_40ED6_iter3;

		while (regdi >= 0) {
			regax = regsi;
			regsi--;
			if (regax == 0) break;
			if (LEGACY_READ_S16_LE(polyinfoptrs[regdi]) <
				(legacy_s16)arg_0) break;
			poly_linklist_40ED6_iter4 = regdi;
			regdi = poly_linked_list_40ED6[regdi];
		}
	}

	poly_linked_list_40ED6[polyinfonumpolys] = regdi;
	poly_linked_list_40ED6[poly_linklist_40ED6_iter4] = polyinfonumpolys;
	poly_linklist_40ED6_iter3 = LEGACY_U16_WRAP_ADD(
		poly_linklist_40ED6_iter3, 1U);
	if (regdi < 0) {
		poly_linklist_40ED6_iter2 = polyinfonumpolys;
	}
	poly_linklist_40ED6_iter4 = poly_linked_list_40ED6[poly_linklist_40ED6_iter4];
	polyinfonumpolys = LEGACY_U16_WRAP_ADD(polyinfonumpolys, 1U);
	polyinfoptrnext = LEGACY_U16_WRAP_ADD(polyinfoptrnext,
		LEGACY_U16_WRAP_ADD(LEGACY_U16_WRAP_MUL(
			transshapenumvertscopy, sizeof(struct POINT2D)), 6U));
	if (polyinfonumpolys == 0x190) return 1;
	if (polyinfoptrnext <= 0x2872) return 0;
	return 1;
}

static legacy_u16 projection_angle_from_extent(legacy_s16 extent)
{
	legacy_s32 scaled;
	legacy_s32 quotient;

	scaled = LEGACY_S32_WRAP_MUL((legacy_s32)extent, 0x800L);
	quotient = LEGACY_S32_DIV_OR_ZERO(scaled, 0x168L);
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
	projectiondata5 = LEGACY_U16_WRAP_ADD(
		projectiondata3, projectiondata4);
	projectiondata8 = LEGACY_U16_WRAP_ADD(
		projectiondata6, projectiondata7);
	projectiondata9 = projection_scale_for_angle(
		projectiondata1, projectiondata3);
	if (projectiondata2 != 0) {
		projectiondata10 = projection_scale_for_angle(
			projectiondata2, projectiondata6);
	} else {
		projectiondata10 = LEGACY_U16_WRAP_SUB(LEGACY_U16_WRAP_SUB(
			projectiondata9, projectiondata9 >> 3),
			projectiondata9 >> 4);
		projectiondata2 = polarAngle(projectiondata10, projectiondata6);
	}
}

void set_projection(legacy_s16 i1, legacy_s16 i2, legacy_s16 i3, legacy_s16 i4) {

	projectiondata1 = projection_angle_from_extent(i1);
	projectiondata2 = projection_angle_from_extent(i2);
	projectiondata3 = (legacy_u16)LEGACY_S16_SAR(i3, 1U);
	projectiondata6 = (legacy_u16)LEGACY_S16_SAR(i4, 1U);
	projection_update_derived();
}

void nopsub_322C0(legacy_u16 i1, legacy_u16 i2) {
	projectiondata4 = i1;
	projectiondata5 = LEGACY_U16_WRAP_ADD(projectiondata3, i1);
	projectiondata7 = i2;
	projectiondata8 = LEGACY_U16_WRAP_ADD(projectiondata6, i2);
}

void nopsub_322DF(legacy_u16 i1, legacy_u16 i2, legacy_u16 i3, legacy_u16 i4) {
	projectiondata1 = i1;
	projectiondata2 = i2;
	projectiondata3 = i3 >> 1;
	projectiondata6 = i4 >> 1;
	projection_update_derived();
}

extern struct RECTANGLE select_rect_rc;
//extern unsigned word_411F6;
extern struct MATRIX mat_y0, mat_y100, mat_y200, mat_y300;
extern legacy_s32 sin80, cos80, sin80_2, cos80_2;

legacy_u16 select_cliprect_rotate(legacy_s16 angZ, legacy_s16 angX, legacy_s16 angY, struct RECTANGLE* cliprect, legacy_s16 unk) {
	struct MATRIX* matptr;
	struct VECTOR vec, vec2;

	//return ported_select_cliprect_rotate_(angX, angY, angZ, cliprect, unk);

	mat_temp = *mat_rot_zxy(angZ, angX, angY, 1);
	polyinfo_reset();
	select_rect_rc = *cliprect;
	select_rect_param = unk;
	matptr = mat_rot_zxy(-angZ, -angX, -angY, 0);
	vec.z = 0x2710;
	vec.y = 0;
	vec.x = 0;
	mat_mul_vector(&vec, matptr, &vec2);
	return polarAngle(vec2.x, vec2.z) & 0x3FF;
}

void polyinfo_reset(void) {
	polyinfonumpolys = 0;
	polyinfoptrnext = 0;
	word_40ECE = 0;
	poly_linked_list_40ED6[0x190] = 0xFFFF;
	poly_linklist_40ED6_iter2 = 0x190;
}

void calc_sincos80(void) {
	sin80 = sin_fast(0x80);
	cos80 = cos_fast(0x80);
	sin80_2 = sin_fast(0x80);
	cos80_2 = cos_fast(0x80);
}

void init_polyinfo(void) {
	polyinfoptr = mmgr_alloc_resbytes("polyinfo", 0x28A0);

	mat_rot_y(&mat_y0, 0);
	mat_rot_y(&mat_y100, 0x100);
	mat_rot_y(&mat_y200, 0x200);
	mat_rot_y(&mat_y300, 0x300);
	calc_sincos80();
}

void get_a_poly_info(void)
{
	legacy_u8 far* record;
	struct POINT2D points[13];
	legacy_u16 record_index;
	legacy_u16 primitive_index;
	legacy_u16 material_type;
	legacy_u16 material_color;
	legacy_u16 primitive_type;
	legacy_u16 vertex_count;
	legacy_u16 pattern_type;

	record_index = 0x190U;
	for (primitive_index = 0; primitive_index < polyinfonumpolys;
		primitive_index++) {
		record_index = (legacy_u16)poly_linked_list_40ED6[record_index];
		record = polyinfoptrs[record_index];
		material_type = record[2];
		material_color = (legacy_u16)
			material_clrlist_ptr_cpy[material_type];
		primitive_type = record[4];

		if (primitive_type == 0U) {
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
				preRender_unk((legacy_u16)
					material_patlist2_ptr_cpy[material_type],
					(legacy_u16)material_clrlist2_ptr_cpy[material_type],
					material_color, vertex_count,
						points);
			}
		} else if (primitive_type == 1U) {
			preRender_line(polyinfo_read_word(record, 3U),
				polyinfo_read_word(record, 4U),
				polyinfo_read_word(record, 5U),
				polyinfo_read_word(record, 6U), material_color);
		} else if (primitive_type == 2U) {
			preRender_sphere(LEGACY_S16_FROM_BITS(
				polyinfo_read_word(record, 3U)),
				LEGACY_S16_FROM_BITS(polyinfo_read_word(record, 4U)),
				polyinfo_read_word(record, 5U), material_color);
		} else if (primitive_type == 3U) {
			polyinfo_read_points(record, points, 4U);
			preRender_wheel(points, WHEEL_INNER_RADIUS_SCALE, material_color,
				(legacy_u16)material_clrlist_ptr_cpy[material_type + 1U],
				(legacy_u16)material_clrlist_ptr_cpy[material_type + 2U]);
		} else if (primitive_type == 5U) {
			putpixel_single_maybe(
				LEGACY_S16_FROM_BITS(polyinfo_read_word(record, 3U)),
				LEGACY_S16_FROM_BITS(polyinfo_read_word(record, 4U)),
				material_color);
		}
	}
	polyinfo_reset();
}

// generate_poly_edges is called preRender_helper in the IDB.
// aka preRender_helper3 in the IDB
