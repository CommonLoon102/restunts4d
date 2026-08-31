#ifndef RESTUNTS_SHAPE3D_H
#define RESTUNTS_SHAPE3D_H

#include "math.h"

#define SHAPE3D_HEADER_SIZE              4U
#define SHAPE3D_VERTEX_COUNT_OFFSET      0U
#define SHAPE3D_PRIMITIVE_COUNT_OFFSET   1U
#define SHAPE3D_PAINT_COUNT_OFFSET       2U

#pragma pack (push, 1)

struct SHAPE3D {
	legacy_u16 shape3d_numverts;
	legacy_u8 far* shape3d_vertex_bytes;
	legacy_u16 shape3d_numprimitives;
	legacy_u16 shape3d_numpaints;
	legacy_u8 far* shape3d_primitives;
	legacy_u8 far* shape3d_cull1;
	legacy_u8 far* shape3d_cull2;
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

/* These records mix 16-bit near and far pointers, so their original layout is
 * a DOS-compiler property rather than a portable resource representation. */
#if defined(RESTUNTS_16BIT_DOS_COMPILER)
typedef char legacy_shape3d_must_be_22_bytes[
	(sizeof(struct SHAPE3D) == 22) ? 1 : -1];
typedef char legacy_transformedshape3d_must_be_20_bytes[
	(sizeof(struct TRANSFORMEDSHAPE3D) == 20) ? 1 : -1];
#endif

legacy_s16 shape3d_load_all(void);
void shape3d_free_all(void);
void shape3d_vertex_read(const struct SHAPE3D* shape, legacy_u16 index,
	struct VECTOR* destination);
void shape3d_vertex_write(struct SHAPE3D* shape, legacy_u16 index,
	const struct VECTOR* source);
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
	const struct POINT2D* vertices);
void preRender_default_alt(legacy_u16 color, legacy_u16 vertex_count,
	const struct POINT2D* vertices);
void preRender_patterned(legacy_u16 pattern, legacy_u16 color,
	legacy_u16 vertex_count, const struct POINT2D* vertices);
void preRender_unk(legacy_u16 pattern, legacy_u16 alternate_color,
	legacy_u16 color, legacy_u16 vertex_count,
	const struct POINT2D* vertices);
void preRender_line(legacy_u16 start_x, legacy_u16 start_y,
	legacy_u16 end_x, legacy_u16 end_y, legacy_u16 color);
legacy_u16 draw_line_related(legacy_u16 start_x, legacy_u16 start_y,
	legacy_u16 end_x, legacy_u16 end_y, legacy_u16* line_data);
legacy_u16 draw_line_related_alt(legacy_u16 start_x, legacy_u16 start_y,
	legacy_u16 end_x, legacy_u16 end_y, legacy_u16* line_data);
void skybox_op_helper(legacy_u16 color, legacy_u16 vertex_count,
	struct POINT2D vertices[]);
void preRender_sphere_helper2(legacy_u16* source, legacy_u16* destination);
void preRender_sphere_helper(legacy_u16* source, legacy_u16 color);
void preRender_wheel_helper3(legacy_u16* source, legacy_u16* destination);
void preRender_wheel_helper2(legacy_u16* source, legacy_u16* destination,
	legacy_u16 scale);
void preRender_wheel_helper(legacy_u16* source, legacy_u16* destination,
	legacy_u16 scale);
void preRender_wheel(const struct POINT2D* source, legacy_u16 scale,
	legacy_u16 outer_color, legacy_u16 side_color, legacy_u16 inner_color);
void preRender_sphere(legacy_s16 x, legacy_s16 y, legacy_u16 size, legacy_u16 color);
void draw_beveled_border(legacy_s16 x, legacy_s16 y,
	legacy_s16 width, legacy_s16 height,
	legacy_s16 top_outer_color, legacy_s16 top_inner_color,
	legacy_s16 bottom_outer_color, legacy_s16 bottom_inner_color);
void draw_lines_unk(legacy_s16 x, legacy_s16 y, legacy_s16 width, legacy_s16 height,
	legacy_s16 outer_color, legacy_s16 inner_color, legacy_s16 opposite_color);
void sub_204AE(struct SHAPE3D* shape, legacy_u16 first_vertex,
	legacy_s16 arg_4, legacy_s16* arg_6, legacy_s16* arg_8,
	struct VECTOR* arg_vecarray, struct VECTOR* arg_vecptr);

#endif
