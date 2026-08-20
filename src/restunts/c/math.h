#ifndef RESTUNTS_MATH_H
#define RESTUNTS_MATH_H

#include "legacy.h"

#pragma pack (push, 1)

struct RECTANGLE {
	legacy_s16 left, right;
	legacy_s16 top, bottom;
	//int x1, y1;
	//int x2, y2;
};

struct VECTOR {
	legacy_s16 x, y, z;
};

struct VECTORLONG {
	legacy_s32 lx, ly, lz;
};

struct POINT2D {
	legacy_s16 px, py;
};

struct MATRIX {
	union {
		legacy_s16 vals[9];
		struct {
			legacy_s16 _11, _21, _31;
			legacy_s16 _12, _22, _32;
			legacy_s16 _13, _23, _33;
		} m;
	};
};

struct PLANE {
	legacy_s16 plane_yz;
	legacy_s16 plane_xy;
	struct VECTOR plane_origin;
	struct VECTOR plane_normal;
	struct MATRIX plane_rotation;
};

#pragma pack (pop)

/* These sizes are part of both the resource format and the assembly ABI. */
typedef char legacy_rectangle_must_be_8_bytes[
	(sizeof(struct RECTANGLE) == 8) ? 1 : -1
];
typedef char legacy_vector_must_be_6_bytes[
	(sizeof(struct VECTOR) == 6) ? 1 : -1
];
typedef char legacy_vectorlong_must_be_12_bytes[
	(sizeof(struct VECTORLONG) == 12) ? 1 : -1
];
typedef char legacy_point2d_must_be_4_bytes[
	(sizeof(struct POINT2D) == 4) ? 1 : -1
];
typedef char legacy_matrix_must_be_18_bytes[
	(sizeof(struct MATRIX) == 18) ? 1 : -1
];
typedef char legacy_plane_must_be_34_bytes[
	(sizeof(struct PLANE) == 34) ? 1 : -1
];

short sin_fast(unsigned short s);
short cos_fast(unsigned short s);

int polarAngle(int z, int y);
int polarRadius2D(int z, int y);
int polarRadius3D(struct VECTOR* vec);

unsigned rect_compare_point(struct POINT2D* pt);

void mat_mul_vector(struct VECTOR* invec, struct MATRIX* mat, struct VECTOR* outvec);
void mat_mul_vector2(struct VECTOR* invec, struct MATRIX far* mat, struct VECTOR* outvec);
void mat_multiply(struct MATRIX* rmat, struct MATRIX* lmat, struct MATRIX* outmat);
void mat_invert(struct MATRIX* inmat, struct MATRIX* outmat);
void mat_rot_x(struct MATRIX* outmat, int angle);
void mat_rot_y(struct MATRIX* outmat, int angle);
void mat_rot_z(struct MATRIX* outmat, int angle);
struct MATRIX* mat_rot_zxy(int z, int x, int y, int unk);

void rect_adjust_from_point(struct POINT2D* pt, struct RECTANGLE* rc);

int vector_op_unk2(struct VECTOR* vec);
void vector_to_point(struct VECTOR* vec, struct POINT2D* outpt);
void vector_op_unk(struct VECTOR* vec1, struct VECTOR* vec2, struct VECTOR* outvec, short i);

short multiply_and_scale(short a1, short a2);

void rect_union(struct RECTANGLE* r1, struct RECTANGLE* r2, struct RECTANGLE* outrc);
int rect_intersect(struct RECTANGLE* r1, struct RECTANGLE* r2);

void plane_rotate_op(void);
int plane_origin_op(int index, int b, int c, int d);

#endif
