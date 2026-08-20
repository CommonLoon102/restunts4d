#ifndef RESTUNTS_SHAPE3D_H
#define RESTUNTS_SHAPE3D_H

#include "math.h"

#pragma pack (push, 1)

struct SHAPE3D {
	legacy_u16 shape3d_numverts;
	struct VECTOR far* shape3d_verts;
	legacy_u16 shape3d_numprimitives;
	legacy_u16 shape3d_numpaints;
	char far* shape3d_primitives;
	char far* shape3d_cull1;
	char far* shape3d_cull2;
};

#ifdef RESTUNTS_DOS
typedef char legacy_shape3d_dos_layout_must_be_22_bytes[
	(sizeof(struct SHAPE3D) == 22) ? 1 : -1
];
#endif

struct SHAPE3DHEADER {
	legacy_u8 header_numverts;
	legacy_u8 header_numprimitives;
	legacy_u8 header_numpaints;
	legacy_u8 header_reserved;
};

typedef char legacy_shape3d_header_must_be_4_bytes[
	(sizeof(struct SHAPE3DHEADER) == 4) ? 1 : -1
];

struct TRANSFORMEDSHAPE3D {
	struct VECTOR pos;
	struct SHAPE3D* shapeptr;
	struct RECTANGLE* rectptr;
	struct VECTOR rotvec;
	legacy_u16 unk;
	legacy_u8 ts_flags;
	legacy_u8 material;
};

#ifdef RESTUNTS_DOS
typedef char legacy_transformed_shape3d_dos_layout_must_be_20_bytes[
	(sizeof(struct TRANSFORMEDSHAPE3D) == 20) ? 1 : -1
];
#endif

#pragma pack (pop)

int shape3d_load_all(void);
void shape3d_free_all(void);
void shape3d_init_shape(char far* shapeptr, struct SHAPE3D* gameshape);
unsigned transformed_shape_op(struct TRANSFORMEDSHAPE3D* arg_transshapeptr);
void set_projection(
	legacy_u16 i1, legacy_u16 i2, legacy_u16 i3, legacy_u16 i4);
legacy_s16 polarAngle(legacy_s16 z, legacy_s16 y);
legacy_u16 select_cliprect_rotate(
	legacy_u16 angZ,
	legacy_u16 angX,
	legacy_u16 angY,
	struct RECTANGLE* cliprect,
	legacy_u16 unk);
void init_polyinfo(void);
void polyinfo_reset(void);
void get_a_poly_info(void);
void sub_204AE(struct VECTOR far* arg_verts, int arg_4, short* arg_6, short* arg_8, struct VECTOR* arg_vecarray, struct VECTOR* arg_vecptr);

#endif
