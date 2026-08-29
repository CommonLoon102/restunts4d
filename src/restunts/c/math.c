#include "externs.h"
#include "legacy.h"
#include "math.h"

void heapsort_by_order(legacy_s16 count, legacy_s16* values,
	legacy_s16* order);

legacy_s16 nopsub_19DE8(legacy_s16 value)
{
	legacy_s16 signed_value;

	signed_value = LEGACY_S16_FROM_BITS(value);
	if (signed_value < 0)
		return -1;
	return signed_value != 0;
}

legacy_s32 nopsub_26552(legacy_s32 value)
{
	legacy_u32 bits;

	bits = (legacy_u32)value;
	if ((bits & 0x80000000UL) != 0)
		bits = (legacy_u32)(0UL - bits);
	return LEGACY_S32_FROM_BITS(bits);
}

extern legacy_s32 sin80, cos80;

/*
 * Scratch matrices used only by mat_rot_zxy().  Keeping them here makes the
 * routine self-contained instead of obtaining private temporary storage from
 * dseg.asm.  They remain static rather than automatic because callers use the
 * returned pointer until the next mat_rot_zxy() call.
 */
static struct MATRIX math_mat_z_rot;
static struct MATRIX math_mat_x_rot;
static struct MATRIX math_mat_y_rot;
static struct MATRIX math_mat_rot_temp;
static legacy_u16 math_mat_y_rot_angle;

legacy_s16 sintab[] = {
	0, 101, 201, 302, 402, 503, 603, 704, 804, 904, 1005, 1105, 1205, 1306, 1406, 1506, 1606, 1706, 1806, 1906, 2006, 2105, 2205, 2305, 2404, 2503, 2603, 2702, 2801, 2900, 2999, 3098, 3196, 3295, 3393, 3492, 3590, 3688, 3786, 3883, 3981, 4078, 4176, 4273, 4370, 4467, 4563, 4660, 4756, 4852, 4948, 5044, 5139, 5235, 5330, 5425, 5520, 5614, 5708, 5803, 5897, 5990, 6084, 6177, 6270, 6363, 6455, 6547, 6639, 6731, 6823, 6914, 7005, 7096, 7186, 7276, 7366, 7456, 7545, 7635, 7723, 7812, 7900, 7988, 8076, 8163, 8250, 8337, 8423, 8509, 8595, 8680, 8765, 8850, 8935, 9019, 9102, 9186, 9269, 9352, 9434, 9516, 9598, 9679, 9760, 9841, 9921, 10001, 10080, 10159, 10238, 10316, 10394, 10471, 10549, 10625, 10702, 10778, 10853, 10928, 11003, 11077, 11151, 11224, 11297, 11370, 11442, 11514, 11585, 11656, 11727, 11797, 11866, 11935, 12004, 12072, 12140, 12207, 12274, 12340, 12406, 12472, 12537, 12601, 12665, 12729, 12792, 12854, 12916, 12978, 13039, 13100, 13160, 13219, 13279, 13337, 13395, 13453, 13510, 13567, 13623, 13678, 13733, 13788, 13842, 13896, 13949, 14001, 14053, 14104, 14155, 14206, 14256, 14305, 14354, 14402, 14449, 14497, 14543, 14589, 14635, 14680, 14724, 14768, 14811, 14854, 14896, 14937, 14978, 15019, 15059, 15098, 15137, 15175, 15213, 15250, 15286, 15322, 15357, 15392, 15426, 15460, 15493, 15525, 15557, 15588, 15619, 15649, 15679, 15707, 15736, 15763, 15791, 15817, 15843, 15868, 15893, 15917, 15941, 15964, 15986, 16008, 16029, 16049, 16069, 16088, 16107, 16125, 16143, 16160, 16176, 16192, 16207, 16221, 16235, 16248, 16261, 16273, 16284, 16295, 16305, 16315, 16324, 16332, 16340, 16347, 16353, 16359, 16364, 16369, 16373, 16376, 16379, 16381, 16383, 16384, 16384
};

extern legacy_u8 atantable[];

legacy_s16 sin_fast(legacy_u16 s) {
	legacy_u8 c = s & 0xFF;
	switch ((s >> 8) & 3) {
		case 0:
			return sintab[c];
		case 1:
			return sintab[0x100 - c];
		case 2:
			return -sintab[c];
		case 3:
			return -sintab[0x100 - c];
	}

	return 0;
}

legacy_s16 cos_fast(legacy_u16 s) {
	return sin_fast(LEGACY_U16_WRAP_ADD(s, 0x100U));
}

legacy_s16 polarAngle(legacy_s16 z, legacy_s16 y) {
	
	legacy_u16 flag;
	legacy_s16 temp, result;
	legacy_u32 index;
	
	flag = 0;
	
	if (z < 0) {
		flag |= 4;
		z = LEGACY_S16_WRAP_NEGATE(z);
	}
	
	if (y < 0) {
		flag |= 2;
		y = LEGACY_S16_WRAP_NEGATE(y);
	}
	
	if (z == y) {
		/* The legacy callers treat a zero-length direction as angle zero. */
		if (z == 0)
			return 0;
		result = 0x80;
	} else {
		if (z > y) {
			temp = z;
			z = y;
			y = temp;
			flag |= 1;
		}
		index = LEGACY_U32_DIV_OR_ZERO(
			LEGACY_U32_SHL((legacy_u16)z, 16U),
			(legacy_u16)y);
		if ((index & 0xFF) >= 0x80) // round upwards
			index += 0x100;
		result = atantable[index >> 8];
	}
	
	switch (flag) {
		case 0:
			return result;
		case 1:
			return -result + 0x100;
		case 2:
			return -result + 0x200;
		case 3:
			return result + 0x100;
		case 4:
			return -result;
		case 5:
			return result - 0x100;
		case 6:
			return result - 0x200;
		case 7:
			return -(result + 0x100);
	}

	return 0;
}

legacy_s16 polarRadius2D(legacy_s16 z, legacy_s16 y) {
	legacy_s32 result;
	
	result = polarAngle(z, y);
	
	if (result < 0) {
		result = -result;
	}
	
	if (result >= 0x100) {
		result = -(result - 0x200);
	}
	
	if (result <= 0x80) {
		result = cos_fast(result);
		if (y < 0)
			y = LEGACY_S16_WRAP_NEGATE(y);
		return LEGACY_S16_FROM_BITS((legacy_u16)
			LEGACY_U32_DIV_OR_ZERO(
				LEGACY_U32_SHL((legacy_u16)y, 14U),
				(legacy_u16)result));
	} else {
		result = sin_fast(result);
		if (z < 0)
			z = LEGACY_S16_WRAP_NEGATE(z);
		return LEGACY_S16_FROM_BITS((legacy_u16)
			LEGACY_U32_DIV_OR_ZERO(
				LEGACY_U32_SHL((legacy_u16)z, 14U),
				(legacy_u16)result));
	}
}

legacy_s16 polarRadius3D(struct VECTOR* vec) {
	return polarRadius2D( polarRadius2D(vec->x, vec->y), vec->z );
}

#ifndef RESTUNTS_HEADLESS
extern struct RECTANGLE select_rect_rc;

legacy_u16 rect_compare_point(struct POINT2D* pt) {
	legacy_s8 flag;
	if (pt->py < select_rect_rc.top)
		flag = 1;
	else if (pt->py > select_rect_rc.bottom)
		flag = 2; 
	else
		flag = 0;
	
	if (pt->px < select_rect_rc.left)
		flag |= 4;
	else if (pt->px > select_rect_rc.right)
		flag |= 8;
	return flag;
}
#endif

static legacy_s16 matrix_scaled_product(legacy_s16 left, legacy_s16 right)
{
	legacy_s32 product;
	legacy_u32 scaled_bits;

	product = LEGACY_S32_WRAP_MUL((legacy_s32)left, (legacy_s32)right);
	scaled_bits = LEGACY_U32_SHL((legacy_u32)product, 2U);
	return LEGACY_S16_FROM_BITS((legacy_u16)(scaled_bits >> 16));
}

void mat_mul_vector(struct VECTOR* invec, struct MATRIX* mat, struct VECTOR* outvec) {

	if (mat->m._11 != 0 && invec->x != 0)
		outvec->x = matrix_scaled_product(mat->m._11, invec->x);
	else
		outvec->x = 0;

	if (mat->m._12 != 0 && invec->y != 0)
		outvec->x = LEGACY_S16_WRAP_ADD(outvec->x,
			matrix_scaled_product(mat->m._12, invec->y));

	if (mat->m._13 != 0 && invec->z != 0)
		outvec->x = LEGACY_S16_WRAP_ADD(outvec->x,
			matrix_scaled_product(mat->m._13, invec->z));


	if (mat->m._21 != 0 && invec->x != 0)
		outvec->y = matrix_scaled_product(mat->m._21, invec->x);
	else
		outvec->y = 0;

	if (mat->m._22 != 0 && invec->y != 0)
		outvec->y = LEGACY_S16_WRAP_ADD(outvec->y,
			matrix_scaled_product(mat->m._22, invec->y));

	if (mat->m._23 != 0 && invec->z != 0)
		outvec->y = LEGACY_S16_WRAP_ADD(outvec->y,
			matrix_scaled_product(mat->m._23, invec->z));


	if (mat->m._31 != 0 && invec->x != 0)
		outvec->z = matrix_scaled_product(mat->m._31, invec->x);
	else
		outvec->z = 0;

	if (mat->m._32 != 0 && invec->y != 0)
		outvec->z = LEGACY_S16_WRAP_ADD(outvec->z,
			matrix_scaled_product(mat->m._32, invec->y));

	if (mat->m._33 != 0 && invec->z != 0)
		outvec->z = LEGACY_S16_WRAP_ADD(outvec->z,
			matrix_scaled_product(mat->m._33, invec->z));

}

void mat_mul_vector2(struct VECTOR* invec, struct MATRIX far* mat, struct VECTOR* outvec)
{
	struct MATRIX tmpmat = *mat;
	
	mat_mul_vector(invec, &tmpmat, outvec);
}

void mat_multiply(struct MATRIX* rmat, struct MATRIX* lmat, struct MATRIX* outmat) {
	legacy_s16 counter;
	legacy_s16* rmatvals = rmat->vals;
	legacy_s16* lmatvals = lmat->vals;
	legacy_s16* outmatvals = outmat->vals;
	
	counter = 9;
	while (counter > 0) {
		if (rmatvals[0] != 0 && lmatvals[0] != 0)
			outmatvals[0] = matrix_scaled_product(
				rmatvals[0], lmatvals[0]); else
			outmatvals[0] = 0;

		if (rmatvals[1] != 0 && lmatvals[3] != 0)
			outmatvals[0] = LEGACY_S16_WRAP_ADD(outmatvals[0],
				matrix_scaled_product(rmatvals[1], lmatvals[3]));

		if (rmatvals[2] != 0 && lmatvals[6] != 0)
			outmatvals[0] = LEGACY_S16_WRAP_ADD(outmatvals[0],
				matrix_scaled_product(rmatvals[2], lmatvals[6]));
		
		outmatvals++;
		if (counter != 7 && counter != 4) {
			lmatvals++;
		} else {
			lmatvals -= 2;
			rmatvals += 3;
		}
		counter = LEGACY_S16_WRAP_SUB(counter, 1);
	}
	
}

void mat_invert(struct MATRIX* inmat, struct MATRIX* outmat) {
	legacy_s16 temp;
	if (inmat == outmat) {
		temp = outmat->m._21;
		outmat->m._21 = outmat->m._12;
		outmat->m._12 = temp;

		temp = outmat->m._31;
		outmat->m._31 = outmat->m._13;
		outmat->m._13 = temp;

		temp = outmat->m._32;
		outmat->m._32 = outmat->m._23;
		outmat->m._23 = temp;
	} else {
		outmat->m._11 = inmat->m._11;
		outmat->m._12 = inmat->m._21;
		outmat->m._13 = inmat->m._31;

		outmat->m._21 = inmat->m._12;
		outmat->m._22 = inmat->m._22;
		outmat->m._23 = inmat->m._32;

		outmat->m._31 = inmat->m._13;
		outmat->m._32 = inmat->m._23;
		outmat->m._33 = inmat->m._33;
	}
}


void mat_rot_x(struct MATRIX* outmat, legacy_s16 angle) {
	legacy_s16 c, s;
	
	c = cos_fast(angle);
	s = sin_fast(angle);
	outmat->m._11 = 0x4000;
	outmat->m._21 = 0;
	outmat->m._31 = 0;
	outmat->m._12 = 0;
	outmat->m._22 = c;
	outmat->m._32 = s;
	outmat->m._13 = 0;
	outmat->m._23 = -s;
	outmat->m._33 = c;
}

void mat_rot_y(struct MATRIX* outmat, legacy_s16 angle) {
	legacy_s16 c, s;
	
	c = cos_fast(angle);
	s = sin_fast(angle);
	outmat->m._11 = c;
	outmat->m._21 = 0;
	outmat->m._31 = -s;
	outmat->m._12 = 0;
	outmat->m._22 = 0x4000;
	outmat->m._32 = 0;
	outmat->m._13 = s;
	outmat->m._23 = 0;
	outmat->m._33 = c;
}

void mat_rot_z(struct MATRIX* outmat, legacy_s16 angle) {
	legacy_s16 c, s;
	
	c = cos_fast(angle);
	s = sin_fast(angle);
	outmat->m._11 = c;
	outmat->m._21 = s;
	outmat->m._31 = 0;
	outmat->m._12 = -s;
	outmat->m._22 = c;
	outmat->m._32 = 0;
	outmat->m._13 = 0;
	outmat->m._23 = 0;
	outmat->m._33 = 0x4000;
}

// mat_rot_zxy was originally optimized, using pre-calced y-matrices and only 
// multiplying the non-zero axes. currently not optimized except for the y cache:
//
// Checked against asmorig/seg006.asm:2293 and the results are identical, for
// three reasons worth writing down so the rewrite is not re-examined:
//
//  - the shortcut matrices are not constants. mat_y0/mat_y100/mat_y200/
//    mat_y300 are all zero in dseg and get filled by mat_rot_y itself in
//    init_polyinfo (shape3d.c:2672-2675), so they hold exactly what this
//    code recomputes.
//  - multiplying by the identity is exact here: mat_multiply forms
//    (a * b) >> 14 and the identity entry is 4000h, so (v * 16384) >> 14
//    is v with nothing lost. Building all three axes and both products when
//    the original would have skipped an axis therefore cannot drift.
//  - the scratch state is private. mat_y_rot, mat_y_rot_angle, mat_z_rot,
//    mat_x_rot and mat_rot_temp are referenced from nowhere else in the
//    program, so the extra writes are invisible.
//
// What does differ is which of them the returned pointer points at: the
// original returns mat_y0 / the y matrix / mat_x_rot / mat_rot_temp /
// mat_z_rot depending on which axes were live. The contents are the same and
// no caller keeps the pointer across another call.

struct MATRIX* mat_rot_zxy(legacy_s16 z, legacy_s16 x, legacy_s16 y, legacy_s16 unk) {
	mat_rot_z(&math_mat_z_rot, z);
	mat_rot_x(&math_mat_x_rot, x);
	
	// y rotation matrix cache
	/*if (mat_y_rot_angle != y) {
		mat_rot_y(&mat_y_rot, y);
		mat_y_rot_angle = y;
	}*/
	math_mat_y_rot_angle = y; // dont forget this!!
	mat_rot_y(&math_mat_y_rot, y);

	if ((unk & 1) != 0) {
		mat_multiply(&math_mat_y_rot, &math_mat_x_rot, &math_mat_rot_temp);
		mat_multiply(&math_mat_rot_temp, &math_mat_z_rot, &math_mat_x_rot);
		return &math_mat_x_rot;
	} else {
		mat_multiply(&math_mat_z_rot, &math_mat_x_rot, &math_mat_rot_temp);
		mat_multiply(&math_mat_rot_temp, &math_mat_y_rot, &math_mat_z_rot);
		return &math_mat_z_rot;
	}
}

#ifndef RESTUNTS_HEADLESS
void rect_adjust_from_point(struct POINT2D* pt, struct RECTANGLE* rc) {
	legacy_s16 temp;
	
	if (rc->left > pt->px) {
		rc->left = pt->px;
	}
	
	temp = pt->px + 1;
	if (rc->right < temp) {
		rc->right = temp;
	}
	
	if (rc->top > pt->py) {
		rc->top = pt->py;
	}
	
	temp = pt->py + 1;
	if (rc->bottom < temp) {
		rc->bottom = temp;
	}
}

void rect_union(struct RECTANGLE* r1, struct RECTANGLE* r2, struct RECTANGLE* outrc) {
	if (r1->left <= r2->left) {
		outrc->left = r1->left;
	} else {
		outrc->left = r2->left;
	}

	if (r1->right >= r2->right) {
		outrc->right = r1->right;
	} else {
		outrc->right = r2->right;
	}
	
	if (r1->top <= r2->top) {
		outrc->top = r1->top;
	} else {
		outrc->top = r2->top;
	}

	if (r1->bottom >= r2->bottom) {
		outrc->bottom = r1->bottom;
	} else {
		outrc->bottom = r2->bottom;
	}
	
	if (video_flag2_is1 == 1) {
		return ;
	}

	// Unreachable. video_flag2_is1 is written exactly once in the whole
	// program - init_main sets it to 1 (asmorig/seg031.asm:237,
	// restunts.c:1260) - and video_flag3_isFFFF alongside it to 0FFFFh.
	// The suppressed tail is `right = (right + video_flag2_is1 - 1) &
	// video_flag3_isFFFF`, which at those values is the identity anyway, so
	// the port loses nothing by not carrying it.
	fatal_error((const legacy_s8*)"rect_union: unexpected code path");
	/*
	mov     bx, [bp+arg_outrectptr]
	mov     si, bx
	mov     ax, [si+RECTANGLE.rc_right]
	add     ax, video_flag2_is1
	dec     ax
	and     ax, video_flag3_isFFFF
	mov     [bx+RECTANGLE.rc_right], ax
*/
}

legacy_s16 rect_intersect(struct RECTANGLE* r1, struct RECTANGLE* r2) {
	if (r1->right < r1->left) return 1;
	if (r2->right <= r1->left) return 1;
	if (r1->right <= r2->left) return 1;
	if (r1->top >= r2->bottom) return 1;
	if (r1->bottom <= r2->top) return 1;
	
	if (r1->left < r2->left) {
		r1->left = r2->left;
	}
	
	if (r1->right > r2->right) {
		r1->right = r2->right;
	}
	
	if (r1->top < r2->top) {
		r1->top = r2->top;
	}
	
	if (r1->bottom > r2->bottom) {
		r1->bottom = r2->bottom;
	}
	return 0;
}

legacy_s16 rect_is_inside(struct RECTANGLE* r1, struct RECTANGLE* r2) {
	if (r1->right > r2->right) {
		return 0;
	}
	
	if (r1->left < r2->left) {
		return 0;
	}
	
	if (r1->top < r2->top) {
		return 0;
	}
	
	if (r1->bottom > r2->bottom) {
		return 0;
	}
	
	return 1;
}

legacy_s16 rect_is_overlapping(struct RECTANGLE* r1, struct RECTANGLE* r2) {
	if (r1->right <= r2->left) {
		return 0;
	}
	
	if (r2->right <= r1->left) {
		return 0;
	}
	
	if (r1->top >= r2->bottom) {
		return 0;
	}
	
	if (r1->bottom <= r2->top) {
		return 0;
	}
	
	return 1;
}

legacy_s16 rect_is_adjacent(struct RECTANGLE* r1, struct RECTANGLE* r2) {
	if (r1->bottom == r2->top || r1->top == r2->bottom) {
		if (r1->left == r2->left && r1->right == r2->right)
			return 1;
		return 0;
	}

	if (r1->right == r2->left || r2->right == r1->left) {
		if (r1->top == r2->top && r1->bottom == r2->bottom)
			return 1;
	}
	return 0;
}

void rectlist_add_rect(legacy_s8* arg_rect_array_length_ptr, struct RECTANGLE* arg_rect_array_ptr, struct RECTANGLE* rect) {	
	legacy_s16 var_counter;
	struct RECTANGLE var_rect;
	struct RECTANGLE var_rect2;
	struct RECTANGLE var_rect3;
	struct RECTANGLE* var_rectptr;
	legacy_s16 var_22, var_18, var_12;

	if (video_flag2_is1 != 1) {
		// Unreachable, for the same reason as the one in rect_union above:
		// video_flag2_is1 is only ever set to 1, in init_main.
		fatal_error((const legacy_s8*)
			"rectlist_add_rect: unexpected code path");
		/*
		mov     bx, [bp+arg_rectptr]
		mov     si, bx
		mov     ax, [si+RECTANGLE.rc_right]
		add     ax, video_flag2_is1
		dec     ax
		and     ax, video_flag3_isFFFF
		mov     [bx+RECTANGLE.rc_right], ax*/
	}
	
	for (var_counter = 0; var_counter < *arg_rect_array_length_ptr; var_counter++) {
		var_rectptr = &arg_rect_array_ptr[var_counter];
		if (rect_is_overlapping(rect, var_rectptr) == 0)
			continue;
		if (rect_is_inside(rect, var_rectptr) != 0)
			return ;

		if (rect_is_inside(var_rectptr, rect) != 0) {
			var_12 = var_counter;

			while (((*arg_rect_array_length_ptr) - 1) > var_12) {
				arg_rect_array_ptr[var_12] = arg_rect_array_ptr[var_12 + 1];
				var_12++;
			}
			(*arg_rect_array_length_ptr)--;
			continue;
		}

		var_rect = *var_rectptr;
		if (var_rectptr->top >= rect->top) {
			if (rect->top < var_rectptr->top) {
				var_rect2 = *rect;
				var_rect2.bottom = var_rectptr->top;
				var_18 = 1;
			} else {
				var_18 = 0;
			}
		} else {	
			var_rect2 = *var_rectptr;
			var_rect2.bottom = rect->top;
			var_rect.top = rect->top;
			var_18 = 1;
		}

		if (var_rectptr->bottom <= rect->bottom) {
			if (rect->bottom > var_rectptr->bottom) {
				var_rect3 = *rect;
				var_rect3.top = var_rectptr->bottom;
				var_22 = 1;
			} else {
				var_22 = 0;
			}
		} else {
			var_rect3 = *var_rectptr;
			var_rect3.top = rect->bottom;
			var_rect.bottom = rect->bottom;
			var_22 = 1;
		}

		if (rect->left <= var_rectptr->left)
			var_rect.left = rect->left;
		else
			var_rect.left = var_rectptr->left;

		if (rect->right >= var_rectptr->right)
			var_rect.right = rect->right;
		else
			var_rect.right = var_rectptr->right;

		var_12 = var_counter;

		while (((*arg_rect_array_length_ptr) - 1) > var_12) {
			arg_rect_array_ptr[var_12] = arg_rect_array_ptr[var_12 + 1];
			var_12 ++;
		}
		(*arg_rect_array_length_ptr)--;
		if (var_18 != 0) {
			rectlist_add_rect(arg_rect_array_length_ptr, arg_rect_array_ptr, &var_rect2);
		}

		rectlist_add_rect(arg_rect_array_length_ptr, arg_rect_array_ptr, &var_rect);
		if (var_22 != 0) {
			rectlist_add_rect(arg_rect_array_length_ptr, arg_rect_array_ptr, &var_rect3);
			return ;
		}
		return ;
	}

	for (var_counter = 0; var_counter < *arg_rect_array_length_ptr; var_counter++) {
		var_rectptr = &arg_rect_array_ptr[var_counter];

		if (rect_is_adjacent(var_rectptr, rect) == 0) {
			continue;
		}
		if (var_rectptr->left <= rect->left)
			var_rect.left = var_rectptr->left;
		else
			var_rect.left = rect->left;

		if (var_rectptr->right >= rect->right)
			var_rect.right = var_rectptr->right;
		else
			var_rect.right = rect->right;

		if (var_rectptr->top <= rect->top)
			var_rect.top = var_rectptr->top;
		else
			var_rect.top = rect->top;
		
		if (var_rectptr->bottom >= rect->bottom)
			var_rect.bottom = var_rectptr->bottom;
		else
			var_rect.bottom = rect->bottom;

		var_12 = var_counter;

		while (((*arg_rect_array_length_ptr) - 1) > var_12) {
			arg_rect_array_ptr[var_12] = arg_rect_array_ptr[var_12 + 1];
			var_12 ++;
		}
		(*arg_rect_array_length_ptr)--;
		rectlist_add_rect(arg_rect_array_length_ptr, arg_rect_array_ptr, &var_rect);
		return ;
	}

	arg_rect_array_ptr[*arg_rect_array_length_ptr] = *rect;
	(*arg_rect_array_length_ptr)++;
}


void rectlist_add_rects(legacy_s8 arg_rectcount, legacy_s8* arg_rectarray_indices, 
	struct RECTANGLE* arg_rectarray1, struct RECTANGLE* arg_rectarray2, 
	struct RECTANGLE* arg_rectptr, legacy_s8* arg_rect_array_length_ptr, struct RECTANGLE* arg_rect_array_ptr) 
{
	struct RECTANGLE* var_rectptr3;
	struct RECTANGLE* var_rectptr;
	struct RECTANGLE* var_rectptr2;
	struct RECTANGLE var_rect;
	struct RECTANGLE var_rect2;
	legacy_s16 var_2, var_rectcounter;
	legacy_s16 var_rectarray_index;
/*
	return ported_rect_clip_combined_(
		arg_rectcount, arg_rectarray_indices, arg_rectarray1, arg_rectarray2, arg_rectptr,
		arg_rect_array_length_ptr, arg_rect_array_ptr);
	*/
	for (var_rectcounter = 0; var_rectcounter < arg_rectcount; var_rectcounter++) {

		var_rectarray_index = arg_rectarray_indices[var_rectcounter];
		if ((var_rectarray_index & 1) != 0) {
			var_rectptr = &arg_rectarray1[var_rectcounter];
		}

		if ((var_rectarray_index & 2) != 0) {
			var_rectptr3 = &arg_rectarray2[var_rectcounter];
		}

		if (((var_rectarray_index & 1) == 0) || var_rectptr->right <= var_rectptr->left) {
			if (((var_rectarray_index & 2) == 0) || var_rectptr3->right <= var_rectptr3->left) {
				var_2 = 0;
			} else {
				var_rectptr2 = var_rectptr3;
				var_2 = 1;
			}
		} else if ((var_rectarray_index & 2) == 0) {
			var_rectptr2 = var_rectptr;
			var_2 = 1;
		} else if (var_rectptr3->right <= var_rectptr3->left) {
			var_rectptr2 = var_rectptr;
			var_2 = 1;
		} else {
			rect_union(var_rectptr, var_rectptr3, &var_rect2);
			var_rectptr2 = &var_rect2;
			var_2 = 1;
		}

		if (var_2 != 0) {
			var_rect = *var_rectptr2;
			if (rect_intersect(&var_rect, arg_rectptr) == 0) {
				rectlist_add_rect(arg_rect_array_length_ptr, arg_rect_array_ptr, &var_rect);
			}
		}
	}

}

void rect_array_sort_by_top(legacy_s8 arg_array_length, struct RECTANGLE* arg_rect_array, legacy_s16* arg_array_indices) {
	legacy_s16 i;
	legacy_s16 intbuffer[256];
	//return ported_rect_array_indexed_op_(arg_array_length, arg_rect_array, arg_array_indices);
	if (arg_array_length > 1) {
		for (i = 0; i < arg_array_length; i++) {
			intbuffer[i] = -arg_rect_array[i].top;
			arg_array_indices[i] = i;
		}
		heapsort_by_order(arg_array_length, intbuffer, arg_array_indices);
	} else {
		arg_array_indices[0] = 0;
	}
}

static legacy_u16 math_word_magnitude(legacy_s16 value)
{
	if (value < 0)
		return (legacy_u16)(0U - (legacy_u16)value);
	return (legacy_u16)value;
}

legacy_s16 vector_op_unk2(struct VECTOR* vec) {
	legacy_s32 y;
	legacy_s32 temp;
	legacy_s32 scaled_angle;
	legacy_s16 flag;
	legacy_s16 result;
	legacy_s32 angle;
	
	y = (legacy_s32)math_word_magnitude(vec->y);
	
	// The original widens the 16-bit radius with an explicit zero high word
	// (mov [bp+var_4], ax / mov [bp+var_2], 0), not with a sign extension.
	temp = (legacy_u16)polarRadius2D(
		LEGACY_S16_FROM_BITS(math_word_magnitude(vec->x)),
		LEGACY_S16_FROM_BITS(math_word_magnitude(vec->z)));
	
	if (sin80 != cos80) {
		//fatal_error("sin80 != cos80 - not observed");
		y = y * sin80;
		temp = temp * cos80;
	} 

	if (temp >= y) {
		flag = 0;
	} else {
		flag = 1;
	}
	
	if (vec->y < 0) {
		if (flag != 0) return 0x1E;
	} else
	if (vec->y > 0) {
		if (flag != 0) return 0x1F;
	}

	if (vec->y > 0) {
		result = 0x0F;
	} else {
		result = 0;
	}
	
	angle = -polarAngle(vec->z, -vec->x);
	if (angle < 0) {
		angle += 0x400;
	}
	
	scaled_angle = LEGACY_S32_WRAP_SUB(
		LEGACY_S32_SHL(angle, 4U), angle);
	result = LEGACY_S16_WRAP_ADD(result,
		(legacy_s16)LEGACY_S32_SAR(scaled_angle, 10U));
	
	return result;
}

// All ten of these are `dw` in dseg and are declared unsigned in shape3d.c,
// which is also how set_projection produces them. math.c used to declare this
// subset as int, so projectiondata9/10 - the only ones that can grow past
// 7FFFh - reached the projection multiply as negative values.
//
// The two roles are not the same, and the casts at the use sites say which is
// which: 9 and 10 are the operands of `mul`, so they stay unsigned there,
// while 5 and 8 are added with `add ax, .. / jo`, whose overflow test reads
// both operands as signed words.
extern legacy_u16 projectiondata5, projectiondata8, projectiondata9, projectiondata10;


// Each `add ax, projectiondataN` in the original is followed by `jo`, and on
// overflow the sum is replaced by the rail it ran past: 7D00h when the true
// sum was too positive (the wrapped word comes back negative, `or ax,ax / jl`)
// and 8300h = -7D00h when it was too negative.
static legacy_s16 saturate_projection(legacy_s32 sum) {
	if (sum > 32767L)
		return 0x7D00;
	if (sum < -32768L)
		return -0x7D00;
	return (legacy_s16)sum;
}

// NEG in the original wraps in AX and is followed by unsigned MUL. In
// particular, -(-32768) remains 8000h and is used as the magnitude 32768.
static legacy_u16 projection_magnitude(legacy_s16 value) {
	return (legacy_u16)(0U - (legacy_u16)value);
}

void vector_to_point(struct VECTOR* vec, struct POINT2D* outpt) {

	legacy_u32 proj;
	// bx in the original: (proj >> 16) << 1 plus the top bit of the low word,
	// which is exactly proj >> 15 kept in 16 bits. `cmp cx, bx / jle` then
	// weighs it against z as a signed word.
	legacy_s16 comp;

	if (vec->z <= 0) {
		outpt->px = 0x8000;
		outpt->py = 0x8000;
		return;
	}
	
	if (vec->x < 0) {
		proj = (legacy_u32)projection_magnitude(vec->x) * (legacy_u16)projectiondata9;
		comp = (legacy_s16)(proj >> 15);

		if (vec->z > comp) { 
			outpt->px = saturate_projection(
				-(legacy_s32)LEGACY_U32_DIV_OR_ZERO(
					proj, (legacy_u16)vec->z) +
				(legacy_s16)projectiondata5);
		} else
			outpt->px = -0x7D00;
	} else {
		proj = (legacy_u32)(legacy_u16)vec->x * (legacy_u16)projectiondata9;
		comp = (legacy_s16)(proj >> 15);

		if (vec->z > comp) 
			outpt->px = saturate_projection(
				(legacy_s32)LEGACY_U32_DIV_OR_ZERO(
					proj, (legacy_u16)vec->z) +
				(legacy_s16)projectiondata5);
		else
			outpt->px = 0x7D00;
	}

	if (vec->y < 0) {
		proj = (legacy_u32)projection_magnitude(vec->y) * (legacy_u16)projectiondata10;
		comp = (legacy_s16)(proj >> 15);

		if (vec->z > comp) 
			outpt->py = saturate_projection(
				(legacy_s32)LEGACY_U32_DIV_OR_ZERO(
					proj, (legacy_u16)vec->z) +
				(legacy_s16)projectiondata8);
		else
			outpt->py = 0x7D00;
	} else {
		proj = (legacy_u32)(legacy_u16)vec->y * (legacy_u16)projectiondata10;
		comp = (legacy_s16)(proj >> 15);

		if (vec->z > comp) 
			outpt->py = saturate_projection(
				-(legacy_s32)LEGACY_U32_DIV_OR_ZERO(
					proj, (legacy_u16)vec->z) +
				(legacy_s16)projectiondata8);
		else
			outpt->py = -0x7D00;
	}
}
#endif

static legacy_s16 vector_interpolate_axis(legacy_s16 first,
	legacy_s16 second, legacy_s16 factor, legacy_s16 divisor)
{
	legacy_s32 product;
	legacy_s32 quotient;

	product = LEGACY_S32_WRAP_MUL(
		(legacy_s32)LEGACY_S16_WRAP_SUB(first, second),
		(legacy_s32)factor);
	quotient = LEGACY_S32_DIV_OR_ZERO(product, (legacy_s32)divisor);
	return LEGACY_S16_WRAP_ADD(
		LEGACY_S16_FROM_BITS((legacy_u16)quotient), second);
}

void vector_op_unk(struct VECTOR* vec1, struct VECTOR* vec2, struct VECTOR* outvec, legacy_s16 i) {
	legacy_s16 var_4, var_2;
	
	outvec->z = i;

	var_4 = LEGACY_S16_WRAP_SUB(outvec->z, vec2->z);
	var_2 = LEGACY_S16_WRAP_SUB(vec1->z, vec2->z);
	if (var_2 < 0) {
		/* The original uses a 16-bit logical SHR for both values. */
		var_4 = LEGACY_S16_FROM_BITS((legacy_u16)var_4 >> 1);
		var_2 = LEGACY_S16_FROM_BITS((legacy_u16)var_2 >> 1);
	}
	
	outvec->x = vector_interpolate_axis(
		vec1->x, vec2->x, var_4, var_2);
	outvec->y = vector_interpolate_axis(
		vec1->y, vec2->y, var_4, var_2);
}

extern legacy_u8 byte_4032A;
extern legacy_u8 byte_4032B;

void nopsub_33006(struct VECTOR* vec1, struct VECTOR* vec2,
	struct VECTOR* outvec)
{
	legacy_u16 interpolation_z;

	interpolation_z = (legacy_u16)(byte_4032A |
		LEGACY_U16_SHL(byte_4032B, 8U));
	vector_op_unk(vec1, vec2, outvec,
		LEGACY_S16_FROM_BITS(interpolation_z));
}

legacy_s16 multiply_and_scale(legacy_s16 a1, legacy_s16 a2)
{
	legacy_s32 product;
	legacy_u32 scaled_bits;
	legacy_u16 high_word;
	legacy_u16 round_up;

	product = LEGACY_S32_WRAP_MUL((legacy_s32)a1, (legacy_s32)a2);
	scaled_bits = LEGACY_U32_SHL((legacy_u32)product, 2U);
	high_word = (legacy_u16)(scaled_bits >> 16);
	round_up = (legacy_u16)((scaled_bits & 0x8000UL) >> 15);
	return LEGACY_S16_FROM_BITS(
		LEGACY_U16_WRAP_ADD(high_word, round_up));
}

extern legacy_s16 planindex;
extern struct PLANE far* planptr;
extern struct PLANE far* current_planptr;
extern legacy_s16 elem_xCenter;
extern legacy_s16 elem_zCenter;
extern legacy_s16 terrainHeight;

legacy_s16 vec_normalInnerProduct(legacy_s16 x, legacy_s16 y, legacy_s16 z, struct VECTOR far* normal) {
	legacy_s32 x_product;
	legacy_s32 y_product;
	legacy_s32 z_product;
	legacy_s32 sum;
	legacy_s32 quotient;

	x_product = LEGACY_S32_WRAP_MUL(
		(legacy_s32)normal->x, (legacy_s32)x);
	y_product = LEGACY_S32_WRAP_MUL(
		(legacy_s32)normal->y, (legacy_s32)y);
	z_product = LEGACY_S32_WRAP_MUL(
		(legacy_s32)normal->z, (legacy_s32)z);
	sum = LEGACY_S32_WRAP_ADD(
		LEGACY_S32_WRAP_ADD(x_product, z_product), y_product);
	quotient = LEGACY_S32_DIV_OR_ZERO(sum, 0x2000L);
	return LEGACY_S16_FROM_BITS((legacy_u16)quotient);
}

legacy_s16 plane_origin_op(legacy_s16 arg_planindex, legacy_s16 x, legacy_s16 y, legacy_s16 z) {
	struct PLANE far* curplane;
	struct VECTOR a;
	struct VECTOR b;
	
	if (arg_planindex == planindex) {
		curplane = current_planptr;
	} else {
		curplane = &planptr[arg_planindex];
	}

	b.y = curplane->plane_origin.y + terrainHeight;
	a.y = y - b.y;
	if (arg_planindex < 4) {
		// NOTE: what is this
		return a.y;
	}
	b.x = curplane->plane_origin.x + elem_xCenter;
	b.z = curplane->plane_origin.z + elem_zCenter;
	a.x = x - b.x;
	a.z = z - b.z;
	return vec_normalInnerProduct(a.x, a.y, a.z, &curplane->plane_normal);
}

extern legacy_s16 planindex_copy;
extern legacy_s16 pState_minusRotate_z_2;
extern legacy_s16 pState_minusRotate_y_2;
extern legacy_s16 pState_minusRotate_x_2;
extern struct MATRIX mat_unk;
extern struct MATRIX mat_unk2;
extern struct VECTOR vec_unk2;
extern legacy_s16 pState_f36Mminf40sar2;
extern struct VECTOR vec_planerotopresult;
extern legacy_s16 word_3BE16;
extern struct MATRIX mat_planetmp;
extern legacy_s16 pState_f36Mminf40sar2;
extern legacy_s16 f36f40_whlData;

void plane_rotate_op(void) {
/*    var_planptr = dword ptr -54
    var_32 = word ptr -50
    var_2E = word ptr -46
    var_2C = byte ptr -44
    var_1A = byte ptr -26
    var_8 = byte ptr -8
     s = byte ptr 0
     r = byte ptr 2
*/
    struct PLANE far* var_planptr;
    struct VECTOR var_32;
    struct MATRIX var_2C;
    struct MATRIX var_1A;
    struct VECTOR var_8;
	legacy_s16 si;

	//return ported_plane_rotate_op_();
	if (planindex_copy != -1)
		goto loc_197A6;
	goto loc_198C2;
/*    push    bp
    mov     bp, sp
    sub     sp, 36h
    push    di
    push    si
    cmp     planindex_copy, 0FFFFh
    jnz     short loc_197A6
    jmp     loc_198C2
*/
loc_197A6:
	var_planptr = &planptr[planindex_copy];
	if (var_planptr->plane_xy != pState_minusRotate_x_2)
		goto loc_197D6;
	if (var_planptr->plane_yz != pState_minusRotate_z_2)
		goto loc_197D6;
	si = pState_minusRotate_y_2;
	goto loc_19845;
/*    mov     ax, 22h ; '"'
    imul    planindex_copy
    add     ax, word ptr planptr
    mov     dx, word ptr planptr+2
    mov     word ptr [bp+var_planptr], ax
    mov     word ptr [bp+var_planptr+2], dx
    les     bx, [bp+var_planptr]
    mov     ax, pState_minusRotate_x_2
    cmp     es:[bx+2], ax
    jnz     short loc_197D6
    mov     ax, pState_minusRotate_z_2
    cmp     es:[bx], ax
    jnz     short loc_197D6
    mov     si, pState_minusRotate_y_2
    jmp     short loc_19845
    ; align 2
    db 144*/
loc_197D6:
	mat_mul_vector(&vec_unk2, &mat_unk, &var_8);
	var_1A = planptr[planindex_copy].plane_rotation;
	mat_invert(&var_1A, &var_2C);
/*    lea     ax, [bp+var_8]
    push    ax
    mov     ax, offset mat_unk
    push    ax
    mov     ax, offset vec_unk2
    push    ax
    call    mat_mul_vector
    add     sp, 6
    mov     ax, 22h ; '"'
    imul    planindex_copy
    add     ax, word ptr planptr
    mov     dx, word ptr planptr+2
    add     ax, 10h         ; plane rotation matrix
    push    si
    lea     di, [bp+var_1A]
    mov     si, ax
    push    ss
    pop     es
    push    ds
    mov     ds, dx
    mov     cx, 9
    repne movsw
    pop     ds
    pop     si
    lea     ax, [bp+var_2C]
    push    ax
    lea     ax, [bp+var_1A]
    push    ax
    call    mat_invert
    add     sp, 4*/

	mat_mul_vector(&var_8, &var_2C, &var_32);
	si = polarAngle(-var_32.x, var_32.z);
/*    lea     ax, [bp+var_32]
    push    ax
    lea     ax, [bp+var_2C]
    push    ax
    lea     ax, [bp+var_8]
    push    ax
    call    mat_mul_vector
    add     sp, 6
    push    [bp+var_32.vz]
    mov     ax, [bp+var_32.vx]
    neg     ax
    push    ax
    call    polarAngle
    add     sp, 4
    mov     si, ax*/
loc_19845:
	si += pState_f36Mminf40sar2;
	if (si == 0)
		goto loc_198A4;
	if (word_3BE16 == si)
		goto loc_19866;
	mat_rot_y(&mat_planetmp, -si);
	word_3BE16 = si;
    /*add     si, pState_f36Mminf40sar2
    jz      short loc_198A4
    cmp     word_3BE16, si
    jz      short loc_19866
    mov     ax, si
    neg     ax
    push    ax
    mov     ax, offset unk_40D58
    push    ax
    call    mat_rot_y
    add     sp, 4
    mov     word_3BE16, si*/
loc_19866:
	mat_mul_vector(&vec_unk2, &mat_planetmp, &var_32);
	mat_mul_vector2(&var_32, &planptr[planindex_copy].plane_rotation, &vec_planerotopresult);
	return ;
/*    lea     ax, [bp+var_32]
    push    ax
    mov     ax, offset unk_40D58
    push    ax
    mov     ax, offset vec_unk2
    push    ax
    call    mat_mul_vector
    add     sp, 6
    mov     ax, offset vec_planerotopresult
    push    ax
    mov     ax, 22h ; '"'
    imul    planindex_copy
    add     ax, word ptr planptr
    mov     dx, word ptr planptr+2
    add     ax, 10h         ; plane rotation matrix.
    push    dx
    push    ax
    lea     ax, [bp+var_32]
loc_19895:
    push    ax
    push    cs
    call near ptr ported_mat_mul_vector2_
    add     sp, 8
    pop     si
    pop     di
    mov     sp, bp
    pop     bp
    retf
    ; align 2
    db 144*/
loc_198A4:
/*    mov     ax, offset vec_planerotopresult
    push    ax
    mov     ax, 22h         ; sizeof plane
    imul    planindex_copy
    add     ax, word ptr planptr
    mov     dx, word ptr planptr+2
    add     ax, 10h
    push    dx
    push    ax
    mov     ax, offset vec_unk2
	*/
	mat_mul_vector2(&vec_unk2, &planptr[planindex_copy].plane_rotation, &vec_planerotopresult);
	return ;
/*    jmp     short loc_19895
    ; align 2
    db 144*/
loc_198C2:
	if (pState_f36Mminf40sar2 == 0)
		goto loc_1990C;
	if (pState_f36Mminf40sar2 == f36f40_whlData)
		goto loc_198EA;
	mat_rot_y(&mat_unk2, -pState_f36Mminf40sar2);
	f36f40_whlData = pState_f36Mminf40sar2;
/*    cmp     pState_f36Mminf40sar2, 0
    jz      short loc_1990C
    mov     ax, f36f40_whlData
    cmp     pState_f36Mminf40sar2, ax
    jz      short loc_198EA
    mov     ax, pState_f36Mminf40sar2
    neg     ax
    push    ax
    mov     ax, offset mat_unk2
    push    ax
    call    mat_rot_y
    add     sp, 4
    mov     ax, pState_f36Mminf40sar2
    mov     f36f40_whlData, ax*/
loc_198EA:
	mat_mul_vector(&vec_unk2, &mat_unk2, &var_32);
	mat_mul_vector(&var_32, &mat_unk, &vec_planerotopresult);
	return ;
/*    lea     ax, [bp+var_32]
    push    ax
    mov     ax, offset mat_unk2
    push    ax
    mov     ax, offset vec_unk2
    push    ax
    call    mat_mul_vector
    add     sp, 6
    mov     ax, offset vec_planerotopresult
    push    ax
    mov     ax, offset mat_unk
    push    ax
    lea     ax, [bp+var_32]
    jmp     short loc_19917
    ; align 2
    db 144*/
loc_1990C:
	mat_mul_vector(&vec_unk2, &mat_unk, &vec_planerotopresult);
}
/*
    mov     ax, offset vec_planerotopresult
    push    ax
    mov     ax, offset mat_unk
    push    ax
    mov     ax, offset vec_unk2
loc_19917:
    push    ax
    call    mat_mul_vector
    add     sp, 6
    pop     si
    pop     di
    mov     sp, bp
    pop     bp
    retf
plane_rotate_op endp
*/
