#ifndef RESTUNTS_SHAPE3D_H
#define RESTUNTS_SHAPE3D_H

#include "math.h"

#pragma pack (push, 1)

struct SHAPE3D {
	legacy_u16 shape3d_numverts;
	struct VECTOR far* shape3d_verts;
	legacy_u16 shape3d_numprimitives;
	legacy_u16 shape3d_numpaints;
	legacy_s8 far* shape3d_primitives;
	legacy_s8 far* shape3d_cull1;
	legacy_s8 far* shape3d_cull2;
};

struct SHAPE3DHEADER {
	legacy_u8 header_numverts;
	legacy_u8 header_numprimitives;
	legacy_u8 header_numpaints;
	legacy_u8 header_reserved;
};

struct TRANSFORMEDSHAPE3D {
	struct VECTOR pos;
	struct SHAPE3D* shapeptr;
	struct RECTANGLE* rectptr;
	struct VECTOR rotvec;
	legacy_u16 unk;
	legacy_u8 ts_flags;
	legacy_u8 material;
};

#pragma pack (pop)

typedef char legacy_shape3dheader_must_be_4_bytes[
	(sizeof(struct SHAPE3DHEADER) == 4) ? 1 : -1];

#ifdef RESTUNTS_DOS
typedef char legacy_shape3d_must_be_22_bytes[
	(sizeof(struct SHAPE3D) == 22) ? 1 : -1];
typedef char legacy_transformedshape3d_must_be_20_bytes[
	(sizeof(struct TRANSFORMEDSHAPE3D) == 20) ? 1 : -1];
#endif

legacy_s16 shape3d_load_all(void);
void shape3d_free_all(void);
void shape3d_load_car_shapes(legacy_s8* carid, legacy_s8* opponent_carid);
void shape3d_free_car_shapes(void);
void shape3d_init_shape(legacy_s8 far* shapeptr, struct SHAPE3D* gameshape);
legacy_u16 transformed_shape_op(struct TRANSFORMEDSHAPE3D* arg_transshapeptr);
void set_projection(legacy_s16 i1, legacy_s16 i2, legacy_s16 i3, legacy_s16 i4);
legacy_s16 polarAngle(legacy_s16 z, legacy_s16 y);
legacy_u16 select_cliprect_rotate(legacy_s16 angZ, legacy_s16 angX, legacy_s16 angY, struct RECTANGLE* cliprect, legacy_s16 unk);
void init_polyinfo(void);
void polyinfo_reset(void);
void get_a_poly_info(void);
void preRender_default(legacy_u16 color, legacy_u16 vertex_count,
	legacy_s16* vertices);
void preRender_default_alt(legacy_u16 color, legacy_u16 vertex_count,
	legacy_s16* vertices);
void preRender_patterned(legacy_u16 pattern, legacy_u16 color,
	legacy_u16 vertex_count, legacy_s16* vertices);
void preRender_unk(legacy_u16 pattern, legacy_u16 alternate_color,
	legacy_u16 color, legacy_u16 vertex_count, legacy_s16* vertices);
void preRender_line(legacy_u16 start_x, legacy_u16 start_y,
	legacy_u16 end_x, legacy_u16 end_y, legacy_u16 color);
legacy_u16 draw_line_related(legacy_u16 start_x, legacy_u16 start_y,
	legacy_u16 end_x, legacy_u16 end_y, legacy_s16* line_data);
legacy_u16 draw_line_related_alt(legacy_u16 start_x, legacy_u16 start_y,
	legacy_u16 end_x, legacy_u16 end_y, legacy_s16* line_data);
void skybox_op_helper(legacy_u16 color, legacy_u16 vertex_count,
	struct POINT2D vertices[]);
void preRender_sphere_helper2(legacy_u16* source, legacy_u16* destination);
void preRender_sphere_helper(legacy_u16* source, legacy_u16 color);
void preRender_wheel_helper3(legacy_u16* source, legacy_u16* destination);
void preRender_wheel_helper2(legacy_u16* source, legacy_u16* destination,
	legacy_u16 scale);
void preRender_wheel_helper(legacy_u16* source, legacy_u16* destination,
	legacy_u16 scale);
void preRender_wheel(legacy_u16* source, legacy_u16 scale,
	legacy_u16 outer_color, legacy_u16 side_color, legacy_u16 inner_color);
void preRender_sphere(legacy_s16 x, legacy_s16 y, legacy_u16 size, legacy_u16 color);
void draw_lines_unk(legacy_s16 x, legacy_s16 y, legacy_s16 width, legacy_s16 height,
	legacy_s16 outer_color, legacy_s16 inner_color, legacy_s16 opposite_color);
void sub_204AE(struct VECTOR far* arg_verts, legacy_s16 arg_4, legacy_s16* arg_6, legacy_s16* arg_8, struct VECTOR* arg_vecarray, struct VECTOR* arg_vecptr);

#endif
