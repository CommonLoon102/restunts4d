#include "externs.h"
#include "math.h"

#ifdef RESTUNTS_DOS
#include <dos.h>
#endif

extern struct RECTANGLE select_rect_rc;
extern struct MATRIX mat_z_rot;
extern struct MATRIX mat_x_rot;
extern struct MATRIX mat_y_rot;
extern struct MATRIX mat_rot_temp;
extern unsigned mat_y_rot_angle;

legacy_s16 sintab[] = {
	0, 101, 201, 302, 402, 503, 603, 704, 804, 904, 1005, 1105, 1205, 1306, 1406, 1506, 1606, 1706, 1806, 1906, 2006, 2105, 2205, 2305, 2404, 2503, 2603, 2702, 2801, 2900, 2999, 3098, 3196, 3295, 3393, 3492, 3590, 3688, 3786, 3883, 3981, 4078, 4176, 4273, 4370, 4467, 4563, 4660, 4756, 4852, 4948, 5044, 5139, 5235, 5330, 5425, 5520, 5614, 5708, 5803, 5897, 5990, 6084, 6177, 6270, 6363, 6455, 6547, 6639, 6731, 6823, 6914, 7005, 7096, 7186, 7276, 7366, 7456, 7545, 7635, 7723, 7812, 7900, 7988, 8076, 8163, 8250, 8337, 8423, 8509, 8595, 8680, 8765, 8850, 8935, 9019, 9102, 9186, 9269, 9352, 9434, 9516, 9598, 9679, 9760, 9841, 9921, 10001, 10080, 10159, 10238, 10316, 10394, 10471, 10549, 10625, 10702, 10778, 10853, 10928, 11003, 11077, 11151, 11224, 11297, 11370, 11442, 11514, 11585, 11656, 11727, 11797, 11866, 11935, 12004, 12072, 12140, 12207, 12274, 12340, 12406, 12472, 12537, 12601, 12665, 12729, 12792, 12854, 12916, 12978, 13039, 13100, 13160, 13219, 13279, 13337, 13395, 13453, 13510, 13567, 13623, 13678, 13733, 13788, 13842, 13896, 13949, 14001, 14053, 14104, 14155, 14206, 14256, 14305, 14354, 14402, 14449, 14497, 14543, 14589, 14635, 14680, 14724, 14768, 14811, 14854, 14896, 14937, 14978, 15019, 15059, 15098, 15137, 15175, 15213, 15250, 15286, 15322, 15357, 15392, 15426, 15460, 15493, 15525, 15557, 15588, 15619, 15649, 15679, 15707, 15736, 15763, 15791, 15817, 15843, 15868, 15893, 15917, 15941, 15964, 15986, 16008, 16029, 16049, 16069, 16088, 16107, 16125, 16143, 16160, 16176, 16192, 16207, 16221, 16235, 16248, 16261, 16273, 16284, 16295, 16305, 16315, 16324, 16332, 16340, 16347, 16353, 16359, 16364, 16369, 16373, 16376, 16379, 16381, 16383, 16384, 16384
};

extern unsigned char atantable[];

legacy_s16 sin_fast(legacy_u16 s) {
	legacy_u16 c = (legacy_u8)s;
	switch ((s >> 8) & 3) {
		case 0:
			return sintab[c];
		case 1:
			return sintab[0x100 - c];
		case 2:
			return LEGACY_S16_FROM_BITS(
				LEGACY_U16_WRAP_NEGATE(sintab[c]));
		case 3:
			return LEGACY_S16_FROM_BITS(
				LEGACY_U16_WRAP_NEGATE(sintab[0x100U - c]));
	}
	return 0;
}

legacy_s16 cos_fast(legacy_u16 s) {
	return sin_fast(LEGACY_U16_WRAP_ADD(s, 0x100U));
}

legacy_s16 polarAngle(legacy_s16 z, legacy_s16 y) {
	legacy_u16 flag;
	legacy_s16 temp, result;
	legacy_u32 quotient;
	legacy_u16 index;
	
	flag = 0;
	result = 0;
	
	if (z < 0) {
		flag |= 4;
		z = LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_NEGATE(z));
	}
	
	if (y < 0) {
		flag |= 2;
		y = LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_NEGATE(y));
	}
	
	if (z == y) {
		if (z != 0)
			result = 0x80;
	} else {
		if (z > y) {
			temp = z;
			z = y;
			y = temp;
			flag |= 1;
		}
		if ((legacy_u16)y != 0) {
			quotient = ((legacy_u32)(legacy_u16)z << 16) /
				(legacy_u16)y;
			index = (legacy_u16)quotient >> 8;
			if (((legacy_u8)quotient & 0xFFU) >= 0x80U)
				index = LEGACY_U16_WRAP_ADD(index, 1U);
			result = atantable[index];
		}
	}
	
	switch (flag) {
		case 0:
			return result;
		case 1:
			return LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_ADD(
				LEGACY_U16_WRAP_NEGATE(result), 0x100U));
		case 2:
			return LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_ADD(
				LEGACY_U16_WRAP_NEGATE(result), 0x200U));
		case 3:
			return LEGACY_S16_FROM_BITS(
				LEGACY_U16_WRAP_ADD(result, 0x100U));
		case 4:
			return LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_NEGATE(result));
		case 5:
			return LEGACY_S16_FROM_BITS(
				LEGACY_U16_WRAP_SUB(result, 0x100U));
		case 6:
			return LEGACY_S16_FROM_BITS(
				LEGACY_U16_WRAP_SUB(result, 0x200U));
		case 7:
			return LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_NEGATE(
				LEGACY_U16_WRAP_ADD(result, 0x100U)));
	}

	return 0;
}

legacy_s16 polarRadius2D(legacy_s16 z, legacy_s16 y) {
	legacy_s16 result;
	legacy_u32 numerator;
	
	result = polarAngle(z, y);
	
	if (result < 0) {
		result = LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_NEGATE(result));
	}
	
	if (result >= 0x100) {
		result = LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_NEGATE(
			LEGACY_U16_WRAP_SUB(result, 0x200U)));
	}
	
	if (result <= 0x80) {
		result = cos_fast(result);
		if (y < 0)
			y = LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_NEGATE(y));
		numerator = LEGACY_U32_WRAP_MUL(
			(legacy_u32)(legacy_s32)y, 0x4000UL);
	} else {
		result = sin_fast(result);
		if (z < 0)
			z = LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_NEGATE(z));
		numerator = LEGACY_U32_WRAP_MUL(
			(legacy_u32)(legacy_s32)z, 0x4000UL);
	}

	return LEGACY_S16_FROM_BITS(
		(legacy_u16)(numerator / (legacy_u16)result));
}

legacy_s16 polarRadius3D(struct VECTOR* vec) {
	return polarRadius2D( polarRadius2D(vec->x, vec->y), vec->z );
}

legacy_s16 rect_compare_point(struct POINT2D* pt) {
	legacy_u8 flag;
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

static legacy_s16 multiply_and_shift_14(legacy_s16 left, legacy_s16 right)
{
	legacy_u32 product;

	product = LEGACY_U32_WRAP_MUL(
		(legacy_u32)(legacy_s32)left,
		(legacy_u32)(legacy_s32)right);
	product = LEGACY_U32_WRAP_MUL(product, 4U);
	return LEGACY_S16_FROM_BITS(LEGACY_U32_HIGH_WORD(product));
}

void mat_mul_vector(struct VECTOR* invec, struct MATRIX* mat, struct VECTOR* outvec) {

	if (mat->m._11 != 0 && invec->x != 0)
		outvec->x = multiply_and_shift_14(mat->m._11, invec->x);
	else
		outvec->x = 0;

	if (mat->m._12 != 0 && invec->y != 0)
		outvec->x = LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_ADD(
			outvec->x, multiply_and_shift_14(mat->m._12, invec->y)));

	if (mat->m._13 != 0 && invec->z != 0)
		outvec->x = LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_ADD(
			outvec->x, multiply_and_shift_14(mat->m._13, invec->z)));


	if (mat->m._21 != 0 && invec->x != 0)
		outvec->y = multiply_and_shift_14(mat->m._21, invec->x);
	else
		outvec->y = 0;

	if (mat->m._22 != 0 && invec->y != 0)
		outvec->y = LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_ADD(
			outvec->y, multiply_and_shift_14(mat->m._22, invec->y)));

	if (mat->m._23 != 0 && invec->z != 0)
		outvec->y = LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_ADD(
			outvec->y, multiply_and_shift_14(mat->m._23, invec->z)));


	if (mat->m._31 != 0 && invec->x != 0)
		outvec->z = multiply_and_shift_14(mat->m._31, invec->x);
	else
		outvec->z = 0;

	if (mat->m._32 != 0 && invec->y != 0)
		outvec->z = LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_ADD(
			outvec->z, multiply_and_shift_14(mat->m._32, invec->y)));

	if (mat->m._33 != 0 && invec->z != 0)
		outvec->z = LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_ADD(
			outvec->z, multiply_and_shift_14(mat->m._33, invec->z)));

}

void mat_mul_vector2(struct VECTOR* invec, struct MATRIX far* mat, struct VECTOR* outvec)
{
	struct MATRIX tmpmat = *mat;
	
	mat_mul_vector(invec, &tmpmat, outvec);
}

void mat_multiply(struct MATRIX* rmat, struct MATRIX* lmat, struct MATRIX* outmat) {
	int counter;
	legacy_s16* rmatvals = rmat->vals;
	legacy_s16* lmatvals = lmat->vals;
	legacy_s16* outmatvals = outmat->vals;
	
	counter = 9;
	while (counter > 0) {
		if (rmatvals[0] != 0 && lmatvals[0] != 0)
			outmatvals[0] = multiply_and_shift_14(rmatvals[0], lmatvals[0]); else
			outmatvals[0] = 0;

		if (rmatvals[1] != 0 && lmatvals[3] != 0)
			outmatvals[0] = LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_ADD(
				outmatvals[0], multiply_and_shift_14(rmatvals[1], lmatvals[3])));

		if (rmatvals[2] != 0 && lmatvals[6] != 0)
			outmatvals[0] = LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_ADD(
				outmatvals[0], multiply_and_shift_14(rmatvals[2], lmatvals[6])));
		
		outmatvals++;
		if (counter != 7 && counter != 4) {
			lmatvals++;
		} else {
			lmatvals -= 2;
			rmatvals += 3;
		}
		counter--;
	}
	
}

void mat_invert(struct MATRIX* inmat, struct MATRIX* outmat) {
	int temp;
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


void mat_rot_x(struct MATRIX* outmat, legacy_u16 angle) {
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
	outmat->m._23 = LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_NEGATE(s));
	outmat->m._33 = c;
}

void mat_rot_y(struct MATRIX* outmat, legacy_u16 angle) {
	legacy_s16 c, s;
	
	c = cos_fast(angle);
	s = sin_fast(angle);
	outmat->m._11 = c;
	outmat->m._21 = 0;
	outmat->m._31 = LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_NEGATE(s));
	outmat->m._12 = 0;
	outmat->m._22 = 0x4000;
	outmat->m._32 = 0;
	outmat->m._13 = s;
	outmat->m._23 = 0;
	outmat->m._33 = c;
}

void mat_rot_z(struct MATRIX* outmat, legacy_u16 angle) {
	legacy_s16 c, s;
	
	c = cos_fast(angle);
	s = sin_fast(angle);
	outmat->m._11 = c;
	outmat->m._21 = s;
	outmat->m._31 = 0;
	outmat->m._12 = LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_NEGATE(s));
	outmat->m._22 = c;
	outmat->m._32 = 0;
	outmat->m._13 = 0;
	outmat->m._23 = 0;
	outmat->m._33 = 0x4000;
}

// mat_rot_zxy was originally optimized, using pre-calced y-matrices and only 
// multiplying the non-zero axes. currently not optimized except for the y cache:

#ifdef RESTUNTS_DOS
static void seed_legacy_opponent_wheel_angles(unsigned caller_bp) {
	legacy_u16 far* caller_frame;
	int front_wheel_delta;
	int scaled_speed;
	int i;

	caller_frame = (legacy_u16 far*)MK_FP(_SS, caller_bp);
	if (caller_frame[3] != FP_OFF(&state.opponentstate) ||
		caller_frame[4] != FP_OFF(&simd_opponent) ||
		caller_frame[5] != FP_OFF(&state.playerstate) ||
		caller_frame[6] != FP_OFF(&simd_player) ||
		caller_frame[7] != 1)
		return;
	if (framespersec == 0xA) {
		scaled_speed = ((long)state.opponentstate.car_speed2 * 0x580) / 0x1E00;
	} else {
		scaled_speed = ((long)state.opponentstate.car_speed2 * 0x580) / 0x3C00;
	}
	if (scaled_speed != 0) {
		front_wheel_delta = 0;
		if (state.opponentstate.car_sumSurfAllWheels != 0)
			front_wheel_delta = state.opponentstate.car_40MfrontWhlAngle >> 2;

		for (i = 0; i < 4; i++) {
			legacy_wheel_angle_stack_words[i] =
				state.opponentstate.car_36MwhlAngle;
			if (i < 2)
				legacy_wheel_angle_stack_words[i] -= front_wheel_delta;
		}
		return;
	}

	for (i = 0; i < 4; i++) {
		caller_frame[-160 + i] = legacy_wheel_angle_stack_words[i];
	}
}
#endif

struct MATRIX* mat_rot_zxy(int z, int x, int y, int unk) {
	int flag;

#ifdef RESTUNTS_DOS
	unsigned caller_bp;

	caller_bp = *(unsigned short far*)MK_FP(
		_SS, FP_OFF(&caller_bp) + sizeof(caller_bp)
	);
	seed_legacy_opponent_wheel_angles(caller_bp);
#endif
	
	mat_rot_z(&mat_z_rot, z);
	mat_rot_x(&mat_x_rot, x);
	
	// y rotation matrix cache
	/*if (mat_y_rot_angle != y) {
		mat_rot_y(&mat_y_rot, y);
		mat_y_rot_angle = y;
	}*/
	mat_y_rot_angle = y; // dont forget this!!
	mat_rot_y(&mat_y_rot, y);
	
	if ((unk & 1) != 0) {
		mat_multiply(&mat_y_rot, &mat_x_rot, &mat_rot_temp);
		mat_multiply(&mat_rot_temp, &mat_z_rot, &mat_x_rot);
		return &mat_x_rot;
	} else {
		mat_multiply(&mat_z_rot, &mat_x_rot, &mat_rot_temp);
		mat_multiply(&mat_rot_temp, &mat_y_rot, &mat_z_rot);
		return &mat_z_rot;
	}
}

void rect_adjust_from_point(struct POINT2D* pt, struct RECTANGLE* rc) {
	legacy_s16 temp;
	
	if (rc->left > pt->px) {
		rc->left = pt->px;
	}
	
	temp = LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_ADD(pt->px, 1U));
	if (rc->right < temp) {
		rc->right = temp;
	}
	
	if (rc->top > pt->py) {
		rc->top = pt->py;
	}
	
	temp = LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_ADD(pt->py, 1U));
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

	fatal_error("rect_union: unexpected code path");
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

void rectlist_add_rect(
	legacy_s8* arg_rect_array_length_ptr,
	struct RECTANGLE* arg_rect_array_ptr,
	struct RECTANGLE* rect)
{
	legacy_s8 var_counter;
	struct RECTANGLE var_rect;
	struct RECTANGLE var_rect2;
	struct RECTANGLE var_rect3;
	struct RECTANGLE* var_rectptr;
	legacy_u8 var_22;
	legacy_u8 var_18;
	legacy_s8 var_12;

	if (video_flag2_is1 != 1) {
		fatal_error("rectlist_add_rect: unexpected code path");
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
				var_12 = LEGACY_S8_FROM_BITS(
					(legacy_u8)((legacy_u8)var_12 + 1U));
			}
			*arg_rect_array_length_ptr = LEGACY_S8_FROM_BITS(
				(legacy_u8)((legacy_u8)*arg_rect_array_length_ptr - 1U));
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
			var_12 = LEGACY_S8_FROM_BITS(
				(legacy_u8)((legacy_u8)var_12 + 1U));
		}
		*arg_rect_array_length_ptr = LEGACY_S8_FROM_BITS(
			(legacy_u8)((legacy_u8)*arg_rect_array_length_ptr - 1U));
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
			var_12 = LEGACY_S8_FROM_BITS(
				(legacy_u8)((legacy_u8)var_12 + 1U));
		}
		*arg_rect_array_length_ptr = LEGACY_S8_FROM_BITS(
			(legacy_u8)((legacy_u8)*arg_rect_array_length_ptr - 1U));
		rectlist_add_rect(arg_rect_array_length_ptr, arg_rect_array_ptr, &var_rect);
		return ;
	}

	arg_rect_array_ptr[(legacy_s16)*arg_rect_array_length_ptr] = *rect;
	*arg_rect_array_length_ptr = LEGACY_S8_FROM_BITS(
		(legacy_u8)((legacy_u8)*arg_rect_array_length_ptr + 1U));
}


void rectlist_add_rects(
	legacy_u8 arg_rectcount,
	legacy_u8* arg_rectarray_indices,
	struct RECTANGLE* arg_rectarray1, struct RECTANGLE* arg_rectarray2, 
	struct RECTANGLE* arg_rectptr,
	legacy_s8* arg_rect_array_length_ptr,
	struct RECTANGLE* arg_rect_array_ptr)
{
	struct RECTANGLE* var_rectptr3;
	struct RECTANGLE* var_rectptr;
	struct RECTANGLE* var_rectptr2;
	struct RECTANGLE var_rect;
	struct RECTANGLE var_rect2;
	legacy_u8 var_2;
	legacy_u8 var_rectcounter;
	legacy_u8 var_rectarray_index;
	legacy_s16 var_rectarray_offset;
/*
	return ported_rect_clip_combined_(
		arg_rectcount, arg_rectarray_indices, arg_rectarray1, arg_rectarray2, arg_rectptr,
		arg_rect_array_length_ptr, arg_rect_array_ptr);
	*/
	for (var_rectcounter = 0; var_rectcounter < arg_rectcount; var_rectcounter++) {
		var_rectarray_offset = (legacy_s16)LEGACY_S8_FROM_BITS(
			var_rectcounter);

		var_rectarray_index = arg_rectarray_indices[var_rectarray_offset];
		if ((var_rectarray_index & 1) != 0) {
			var_rectptr = &arg_rectarray1[var_rectarray_offset];
		}

		if ((var_rectarray_index & 2) != 0) {
			var_rectptr3 = &arg_rectarray2[var_rectarray_offset];
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

void rect_array_sort_by_top(
	legacy_s8 arg_array_length,
	struct RECTANGLE* arg_rect_array,
	legacy_s16* arg_array_indices)
{
	legacy_s16 i;
	legacy_s16 intbuffer[256];
	//return ported_rect_array_indexed_op_(arg_array_length, arg_rect_array, arg_array_indices);
	if (arg_array_length > 1) {
		for (i = 0; i < arg_array_length; i++) {
			intbuffer[i] = LEGACY_S16_FROM_BITS(
				LEGACY_U16_WRAP_NEGATE(arg_rect_array[i].top));
			arg_array_indices[i] = i;
		}
		heapsort_by_order(arg_array_length, intbuffer, arg_array_indices);
	} else {
		arg_array_indices[0] = 0;
	}
}


static legacy_s16 legacy_abs_word(legacy_s16 value)
{
	if (value < 0)
		return LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_NEGATE(value));
	return value;
}

legacy_s16 vector_op_unk2(struct VECTOR* vec) {
	legacy_u32 y_bits;
	legacy_u32 temp_bits;
	legacy_u32 angle_bits;
	legacy_u16 angle_word;
	legacy_u8 result;
	int flag;

	y_bits = (legacy_u32)(legacy_s32)legacy_abs_word(vec->y);
	temp_bits = (legacy_u16)polarRadius2D(
		legacy_abs_word(vec->x), legacy_abs_word(vec->z));
	
	if (sin80 != cos80) {
		y_bits = LEGACY_U32_WRAP_MUL(y_bits, (legacy_u32)sin80);
		temp_bits = LEGACY_U32_WRAP_MUL(temp_bits, (legacy_u32)cos80);
	} 

	if (LEGACY_S32_FROM_BITS(temp_bits) >= LEGACY_S32_FROM_BITS(y_bits)) {
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
	
	angle_word = LEGACY_U16_WRAP_NEGATE(polarAngle(
		vec->z, LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_NEGATE(vec->x))));
	if (LEGACY_S16_FROM_BITS(angle_word) < 0)
		angle_word = LEGACY_U16_WRAP_ADD(angle_word, 0x400U);
	angle_bits = (legacy_u32)(legacy_s32)LEGACY_S16_FROM_BITS(angle_word);
	angle_bits = LEGACY_U32_WRAP_SUB(
		LEGACY_U32_WRAP_MUL(angle_bits, 16U), angle_bits);
	angle_bits >>= 10;
	result = (legacy_u8)(result + (legacy_u8)angle_bits);
	return (legacy_s16)LEGACY_S8_FROM_BITS(result);
}

extern legacy_u16 projectiondata5;
extern legacy_u16 projectiondata8;
extern legacy_u16 projectiondata9;
extern legacy_u16 projectiondata10;

static int legacy_s16_add_overflow(
	legacy_u16 left, legacy_u16 right, legacy_u16 result)
{
	return ((~(left ^ right) & (left ^ result) & 0x8000U) != 0);
}

static legacy_s16 project_coordinate(
	legacy_s16 coordinate,
	legacy_s16 depth,
	legacy_u16 scale,
	legacy_u16 center,
	int reverse_direction)
{
	legacy_u16 magnitude;
	legacy_u16 limit;
	legacy_u16 delta;
	legacy_u16 result;
	legacy_u32 product;
	legacy_u32 quotient;
	int negative_delta;

	negative_delta = coordinate < 0;
	if (reverse_direction)
		negative_delta = !negative_delta;

	if (coordinate < 0)
		magnitude = LEGACY_U16_WRAP_NEGATE(coordinate);
	else
		magnitude = (legacy_u16)coordinate;

	product = LEGACY_U32_WRAP_MUL(magnitude, scale);
	limit = LEGACY_U16_WRAP_ADD(
		LEGACY_U32_HIGH_WORD(product),
		LEGACY_U32_HIGH_WORD(product));
	if ((LEGACY_U32_LOW_WORD(product) & 0x8000U) != 0)
		limit = LEGACY_U16_WRAP_ADD(limit, 1U);

	if (depth <= LEGACY_S16_FROM_BITS(limit))
		return negative_delta ?
			LEGACY_S16_FROM_BITS(0x8300U) : (legacy_s16)0x7D00;

	quotient = product / (legacy_u16)depth;
	delta = (legacy_u16)quotient;
	if (negative_delta)
		delta = LEGACY_U16_WRAP_NEGATE(delta);
	result = LEGACY_U16_WRAP_ADD(delta, center);

	if (legacy_s16_add_overflow(delta, center, result))
		return (result & 0x8000U) != 0 ?
			(legacy_s16)0x7D00 : LEGACY_S16_FROM_BITS(0x8300U);

	return LEGACY_S16_FROM_BITS(result);
}


void vector_to_point(struct VECTOR* vec, struct POINT2D* outpt) {
	if (vec->z <= 0) {
		outpt->px = LEGACY_S16_FROM_BITS(0x8000U);
		outpt->py = LEGACY_S16_FROM_BITS(0x8000U);
		return;
	}

	outpt->px = project_coordinate(
		vec->x, vec->z, projectiondata9, projectiondata5, 0);
	outpt->py = project_coordinate(
		vec->y, vec->z, projectiondata10, projectiondata8, 1);
}

static legacy_s16 interpolate_vector_component(
	legacy_s16 first,
	legacy_s16 second,
	legacy_s16 numerator,
	legacy_s16 denominator)
{
	legacy_s16 delta;
	legacy_s32 product;
	legacy_s32 quotient;

	delta = LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_SUB(first, second));
	product = (legacy_s32)delta * (legacy_s32)numerator;
	quotient = product / (legacy_s32)denominator;
	return LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_ADD(
		(legacy_u16)quotient, second));
}

void vector_op_unk(
	struct VECTOR* vec1,
	struct VECTOR* vec2,
	struct VECTOR* outvec,
	legacy_s16 i)
{
	legacy_s16 numerator;
	legacy_s16 denominator;

	outvec->z = i;

	numerator = LEGACY_S16_FROM_BITS(
		LEGACY_U16_WRAP_SUB(outvec->z, vec2->z));
	denominator = LEGACY_S16_FROM_BITS(
		LEGACY_U16_WRAP_SUB(vec1->z, vec2->z));
	if (denominator < 0) {
		numerator = (legacy_s16)((legacy_u16)numerator >> 1);
		denominator = (legacy_s16)((legacy_u16)denominator >> 1);
	}

	outvec->x = interpolate_vector_component(
		vec1->x, vec2->x, numerator, denominator);
	outvec->y = interpolate_vector_component(
		vec1->y, vec2->y, numerator, denominator);
}

legacy_s16 multiply_and_scale(legacy_s16 a1, legacy_s16 a2)
{
	legacy_u32 scaled_product;
	legacy_u16 result_bits;

	scaled_product = LEGACY_U32_WRAP_MUL(
		(legacy_u32)(legacy_s32)a1,
		(legacy_u32)(legacy_s32)a2);
	scaled_product = LEGACY_U32_WRAP_MUL(scaled_product, 4U);
	result_bits = LEGACY_U32_HIGH_WORD(scaled_product);
	if ((LEGACY_U32_LOW_WORD(scaled_product) & 0x8000U) != 0)
		result_bits = LEGACY_U16_WRAP_ADD(result_bits, 1U);

	return LEGACY_S16_FROM_BITS(result_bits);
}

extern legacy_s16 planindex;
extern struct PLANE far* planptr;
extern struct PLANE far* current_planptr;
extern legacy_s16 elem_xCenter;
extern legacy_s16 elem_zCenter;
extern legacy_s16 terrainHeight;

legacy_s16 vec_normalInnerProduct(
	legacy_s16 x,
	legacy_s16 y,
	legacy_s16 z,
	struct VECTOR far* normal)
{
	legacy_u32 sum;
	legacy_s32 quotient;

	sum = LEGACY_U32_WRAP_MUL(
		(legacy_u32)(legacy_s32)normal->x,
		(legacy_u32)(legacy_s32)x);
	sum = LEGACY_U32_WRAP_ADD(sum, LEGACY_U32_WRAP_MUL(
		(legacy_u32)(legacy_s32)normal->z,
		(legacy_u32)(legacy_s32)z));
	sum = LEGACY_U32_WRAP_ADD(sum, LEGACY_U32_WRAP_MUL(
		(legacy_u32)(legacy_s32)normal->y,
		(legacy_u32)(legacy_s32)y));
	quotient = LEGACY_S32_FROM_BITS(sum) / (legacy_s32)0x2000;
	return LEGACY_S16_FROM_BITS((legacy_u16)quotient);
}

legacy_s16 plane_origin_op(
	legacy_s16 arg_planindex,
	legacy_s16 x,
	legacy_s16 y,
	legacy_s16 z)
{
	struct PLANE far* curplane;
	struct VECTOR a;
	struct VECTOR b;
	
	if (arg_planindex == planindex) {
		curplane = current_planptr;
	} else {
		curplane = &planptr[arg_planindex];
	}

	b.y = LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_ADD(
		curplane->plane_origin.y, terrainHeight));
	a.y = LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_SUB(y, b.y));
	if (arg_planindex < 4) {
		return a.y;
	}
	b.x = LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_ADD(
		curplane->plane_origin.x, elem_xCenter));
	b.z = LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_ADD(
		curplane->plane_origin.z, elem_zCenter));
	a.x = LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_SUB(x, b.x));
	a.z = LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_SUB(z, b.z));
	return vec_normalInnerProduct(a.x, a.y, a.z, &curplane->plane_normal);
}

extern int planindex_copy;
extern int pState_minusRotate_z_2;
extern int pState_minusRotate_y_2;
extern int pState_minusRotate_x_2;
extern struct MATRIX mat_unk;
extern struct MATRIX mat_unk2;
extern struct VECTOR vec_unk2;
extern int pState_f36Mminf40sar2;
extern struct VECTOR vec_planerotopresult;
extern int word_3BE16;
extern struct MATRIX mat_planetmp;
extern int pState_f36Mminf40sar2;
extern int f36f40_whlData;

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
	int si;

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
