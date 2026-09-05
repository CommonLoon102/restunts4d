#include "fileio.h"
#include "memmgr.h"
#include "shape3d.h"

extern legacy_s8 far* game1ptr;
extern legacy_s8 far* game2ptr;
extern legacy_s8 far* curshapeptr;
extern struct SHAPE3D game3dshapes[130];
extern legacy_s8 aBarn[];

#define SHAPE3D_REQUIRED_ARENA_BYTES 65000UL
#define SHAPE3D_REQUIRED_HIGHPOOL_PARAGRAPHS 4062U
#define SHAPE3D_BASE_TRACK_SHAPE_COUNT 116
#define SHAPE3D_TRACK_SHAPE_NAME_SIZE 5
#define SHAPE3D_VERTEX_RECORD_SIZE 6U
#define SHAPE3D_PRIMITIVE_CULL_RECORD_SIZE 4U
#define SHAPE3D_PRIMITIVE_RECORD_SIZE 8U

legacy_s16 shape3d_load_all() {
	legacy_s16 i;
	legacy_u32 mmgrofsdiff;
	legacy_s8* shapename;

	game1ptr = 0;
	game2ptr = 0;

	mmgrofsdiff = mmgr_get_res_ofs_diff_scaled();

	// The original only had the arena to draw on. The track shapes loaded
	// below can come out of upper memory instead, so the arena check only
	// has to hold when the pool cannot cover the same amount.
	if (mmgrofsdiff < SHAPE3D_REQUIRED_ARENA_BYTES &&
		!highpool_can_fit(SHAPE3D_REQUIRED_HIGHPOOL_PARAGRAPHS))
		return 1;

	game1ptr = file_load_3dres("game1");
	game2ptr = file_load_3dres("game2");

	for (i = 0; i < SHAPE3D_BASE_TRACK_SHAPE_COUNT; i++) {
		shapename = &aBarn[i * SHAPE3D_TRACK_SHAPE_NAME_SIZE];
		curshapeptr = locate_shape_nofatal(game1ptr, shapename);
		if (curshapeptr == 0)
			curshapeptr = locate_shape_fatal(game2ptr, shapename);
		shape3d_init_shape(curshapeptr, &game3dshapes[i]);
	}
	return 0;
}

void shape3d_free_all() {
	if (game1ptr != 0)
		mmgr_free(game1ptr);
	if (game2ptr != 0)
		mmgr_free(game2ptr);
}

void shape3d_init_shape(legacy_s8 far* shapeptr, struct SHAPE3D* gameshape) {
	legacy_u16 vertex_bytes;
	legacy_u16 primitive_count;

	gameshape->shape3d_numverts =
		(legacy_u8)shapeptr[SHAPE3D_VERTEX_COUNT_OFFSET];
	primitive_count =
		(legacy_u8)shapeptr[SHAPE3D_PRIMITIVE_COUNT_OFFSET];
	gameshape->shape3d_numprimitives = primitive_count;
	// The original stores this one as a byte - `mov byte ptr
	// [bx+SHAPE3D.shape3d_numpaints], al` - leaving the field's high byte
	// alone, where this writes the whole word and zeroes it. The field is
	// only ever read as a count and the shape structs start out zeroed.
	gameshape->shape3d_numpaints =
		(legacy_u8)shapeptr[SHAPE3D_PAINT_COUNT_OFFSET];
	vertex_bytes = LEGACY_U16_WRAP_MUL(gameshape->shape3d_numverts,
		SHAPE3D_VERTEX_RECORD_SIZE);
	gameshape->shape3d_vertex_bytes = (legacy_u8 far*)shapeptr +
		SHAPE3D_HEADER_SIZE;
	gameshape->shape3d_cull1 = (legacy_u8 far*)shapeptr +
		vertex_bytes + SHAPE3D_HEADER_SIZE;
	gameshape->shape3d_cull2 = (legacy_u8 far*)shapeptr +
		LEGACY_U16_WRAP_MUL(primitive_count,
			SHAPE3D_PRIMITIVE_CULL_RECORD_SIZE) + vertex_bytes +
		SHAPE3D_HEADER_SIZE;
	gameshape->shape3d_primitives = (legacy_u8 far*)shapeptr +
		LEGACY_U16_WRAP_MUL(primitive_count,
			SHAPE3D_PRIMITIVE_RECORD_SIZE) + vertex_bytes +
		SHAPE3D_HEADER_SIZE;
}
