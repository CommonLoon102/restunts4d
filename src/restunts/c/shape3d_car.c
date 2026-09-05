#include "fileio.h"
#include "memmgr.h"
#include "shape3d.h"
#include "shape3d_internal.h"

#define CAR_RESOURCE_ID_OFFSET 2U
#define CAR_ID_LENGTH 4
#define CAR_WHEEL_CENTER_COUNT 2U
#define CAR_WHEEL_COUNT 4
#define CAR_WHEEL_VERTEX_GROUP_SIZE 6
#define CAR_WHEEL_VERTEX_COUNT 24U
#define CAR_WHEEL_CENTER_SAMPLE_OFFSET 3U
#define CAR_FIRST_WHEEL_VERTEX 8U
#define CAR_STEERED_WHEEL_VERTEX_COUNT 12
#define CAR_WHEEL_STATE_CACHE_SIZE 5
#define CAR_WHEEL_STEERING_CACHE_INDEX 4
#define CAR_WHEEL_VERTICAL_SCALE_SHIFT 6U
#define PLAYER_EXPLOSION_SHAPE_FIRST 116
#define OPPONENT_EXPLOSION_SHAPE_FIRST 120
#define PLAYER_CAR_LOW_SHAPE 124
#define OPPONENT_CAR_LOW_SHAPE 125
#define PLAYER_CAR_WHEEL_SHAPE 126
#define OPPONENT_CAR_WHEEL_SHAPE 127
#define PLAYER_CAR_HIGH_SHAPE 128
#define OPPONENT_CAR_HIGH_SHAPE 129

static void shape3d_init_car_wheel_vertices(const struct SHAPE3D* shape,
	struct VECTOR centers[CAR_WHEEL_CENTER_COUNT],
	struct VECTOR vertices[CAR_WHEEL_VERTEX_COUNT])
{
	legacy_s16 i;
	struct VECTOR resource_vertex;

	shape3d_vertex_read(shape, CAR_FIRST_WHEEL_VERTEX, &resource_vertex);
	centers[0].x = resource_vertex.x;
	centers[0].z = resource_vertex.z;
	shape3d_vertex_read(shape,
		CAR_FIRST_WHEEL_VERTEX + CAR_WHEEL_CENTER_SAMPLE_OFFSET,
		&resource_vertex);
	centers[0].x = LEGACY_S16_SAR(
		LEGACY_S16_WRAP_ADD(centers[0].x, resource_vertex.x), 1U);

	shape3d_vertex_read(shape,
		CAR_FIRST_WHEEL_VERTEX + CAR_WHEEL_VERTEX_GROUP_SIZE,
		&resource_vertex);
	centers[1].x = resource_vertex.x;
	centers[1].z = resource_vertex.z;
	shape3d_vertex_read(shape,
		CAR_FIRST_WHEEL_VERTEX + CAR_WHEEL_VERTEX_GROUP_SIZE +
		CAR_WHEEL_CENTER_SAMPLE_OFFSET, &resource_vertex);
	centers[1].x = LEGACY_S16_SAR(
		LEGACY_S16_WRAP_ADD(centers[1].x, resource_vertex.x), 1U);

	for (i = 0; i < CAR_WHEEL_VERTEX_GROUP_SIZE; i++) {
		shape3d_vertex_read(shape,
			LEGACY_U16_WRAP_ADD(CAR_FIRST_WHEEL_VERTEX, i),
			&resource_vertex);
		vertices[i].x = LEGACY_S16_WRAP_SUB(
			centers[0].x, resource_vertex.x);
		vertices[i].y = resource_vertex.y;
		vertices[i].z = LEGACY_S16_WRAP_SUB(
			centers[0].z, resource_vertex.z);

		shape3d_vertex_read(shape, LEGACY_U16_WRAP_ADD(
			CAR_FIRST_WHEEL_VERTEX + CAR_WHEEL_VERTEX_GROUP_SIZE, i),
			&resource_vertex);
		vertices[i + CAR_WHEEL_VERTEX_GROUP_SIZE].x = LEGACY_S16_WRAP_SUB(
			centers[1].x, resource_vertex.x);
		vertices[i + CAR_WHEEL_VERTEX_GROUP_SIZE].y = resource_vertex.y;
		vertices[i + CAR_WHEEL_VERTEX_GROUP_SIZE].z = LEGACY_S16_WRAP_SUB(
			centers[1].z, resource_vertex.z);

		shape3d_vertex_read(shape, LEGACY_U16_WRAP_ADD(
			CAR_FIRST_WHEEL_VERTEX + 2U * CAR_WHEEL_VERTEX_GROUP_SIZE, i),
			&vertices[i + 2 * CAR_WHEEL_VERTEX_GROUP_SIZE]);
		shape3d_vertex_read(shape, LEGACY_U16_WRAP_ADD(
			CAR_FIRST_WHEEL_VERTEX + 3U * CAR_WHEEL_VERTEX_GROUP_SIZE, i),
			&vertices[i + 3 * CAR_WHEEL_VERTEX_GROUP_SIZE]);
	}
}

void shape3d_load_car_shapes(legacy_s8 arg_playercarid[], legacy_s8 arg_opponentcarid[]) {
	legacy_s16 i;
	legacy_u32 var_6;
	legacy_u32 copy_index;
	legacy_u8 far* source_bytes;
	legacy_u8 far* destination_bytes;
	for (i = 0; i < CAR_ID_LENGTH; i++)
		aStxxx[CAR_RESOURCE_ID_OFFSET + i] = arg_playercarid[i];
	carresptr = file_load_3dres(aStxxx);
	shape3d_init_shape(locate_shape_fatal(carresptr, "car0"),
		&game3dshapes[PLAYER_CAR_LOW_SHAPE]);
	shape3d_init_shape(locate_shape_fatal(carresptr, "car1"),
		&game3dshapes[PLAYER_CAR_WHEEL_SHAPE]);

	shape3d_init_car_wheel_vertices(&game3dshapes[PLAYER_CAR_WHEEL_SHAPE],
		carshapevec, carshapevecs);

	for (i = 0; i < CAR_WHEEL_STATE_CACHE_SIZE; i++) {
		word_443E8[i] = 0;
	}

	shape3d_init_shape(locate_shape_fatal(carresptr, "car2"),
		&game3dshapes[PLAYER_CAR_HIGH_SHAPE]);
	shape3d_init_shape(locate_shape_fatal(carresptr, "exp0"),
		&game3dshapes[PLAYER_EXPLOSION_SHAPE_FIRST]);
	shape3d_init_shape(locate_shape_fatal(carresptr, "exp1"),
		&game3dshapes[PLAYER_EXPLOSION_SHAPE_FIRST + 1]);
	shape3d_init_shape(locate_shape_fatal(carresptr, "exp2"),
		&game3dshapes[PLAYER_EXPLOSION_SHAPE_FIRST + 2]);
	shape3d_init_shape(locate_shape_fatal(carresptr, "exp3"),
		&game3dshapes[PLAYER_EXPLOSION_SHAPE_FIRST + 3]);

	if (arg_opponentcarid[0] != -1) {
		if (arg_playercarid[0] == arg_opponentcarid[0] && arg_playercarid[1] == arg_opponentcarid[1] &&
			arg_playercarid[2] == arg_opponentcarid[2] && arg_playercarid[3] == arg_opponentcarid[3])
		{
			var_6 = mmgr_get_chunk_size_bytes(carresptr);
			car2resptr = mmgr_alloc_resbytes("car2", var_6);
			source_bytes = (legacy_u8 far*)carresptr;
			destination_bytes = (legacy_u8 far*)car2resptr;

			for (copy_index = 0; copy_index < var_6; copy_index++) {
				destination_bytes[(legacy_u16)copy_index] = source_bytes[(legacy_u16)copy_index];
			}
		} else {
			for (i = 0; i < CAR_ID_LENGTH; i++)
				aStxxx[CAR_RESOURCE_ID_OFFSET + i] = arg_opponentcarid[i];
			car2resptr = file_load_3dres(aStxxx);
		}

		shape3d_init_shape(locate_shape_fatal(car2resptr, "car0"),
			&game3dshapes[OPPONENT_CAR_LOW_SHAPE]);
		shape3d_init_shape(locate_shape_fatal(car2resptr, "car1"),
			&game3dshapes[OPPONENT_CAR_WHEEL_SHAPE]);

		shape3d_init_car_wheel_vertices(
			&game3dshapes[OPPONENT_CAR_WHEEL_SHAPE],
			oppcarshapevec, oppcarshapevecs);
		for (i = 0; i < CAR_WHEEL_STATE_CACHE_SIZE; i++) {
			word_4448A[i] = 0;
		}
		shape3d_init_shape(locate_shape_fatal(car2resptr, "car2"),
			&game3dshapes[OPPONENT_CAR_HIGH_SHAPE]);
		shape3d_init_shape(locate_shape_fatal(car2resptr, "exp0"),
			&game3dshapes[OPPONENT_EXPLOSION_SHAPE_FIRST]);
		shape3d_init_shape(locate_shape_fatal(car2resptr, "exp1"),
			&game3dshapes[OPPONENT_EXPLOSION_SHAPE_FIRST + 1]);
		shape3d_init_shape(locate_shape_fatal(car2resptr, "exp2"),
			&game3dshapes[OPPONENT_EXPLOSION_SHAPE_FIRST + 2]);
		shape3d_init_shape(locate_shape_fatal(car2resptr, "exp3"),
			&game3dshapes[OPPONENT_EXPLOSION_SHAPE_FIRST + 3]);
	} else {
		car2resptr = 0;
	}
}

void sub_204AE(struct SHAPE3D* shape, legacy_u16 first_vertex,
	legacy_s16 arg_4, legacy_s16* arg_6, legacy_s16* arg_8,
	struct VECTOR* arg_vecarray, struct VECTOR* arg_vecptr) {
	legacy_s16 i, j;
	legacy_s16 var_C;
	legacy_s16 var_2;
	legacy_s16 var_14;
	legacy_s16 var_10;
	legacy_s16 var_8;
	legacy_s16 var_4;
	struct VECTOR vertex;
	//return ported_sub_204AE_(arg_verts, arg_4, arg_6, arg_8, arg_vecarray, arg_vecptr);
	// arg_8[4] caches the steering angle the wheel vertices were last built
	// for, so the test is against arg_4, not against zero.
	if (arg_8[CAR_WHEEL_STEERING_CACHE_INDEX] != arg_4) {
		var_C = sin_fast(LEGACY_S16_SAR(arg_4, 1U));
		var_2 = cos_fast(LEGACY_S16_SAR(arg_4, 1U));

		for (i = 0; i < CAR_WHEEL_VERTEX_GROUP_SIZE; i++) {
			shape3d_vertex_read(shape,
				LEGACY_U16_WRAP_ADD(first_vertex, i), &vertex);
			var_14 = multiply_and_scale(arg_vecarray[i].x, var_2);
			vertex.x = LEGACY_S16_WRAP_ADD(
				LEGACY_S16_WRAP_ADD(arg_vecptr[0].x,
					multiply_and_scale(arg_vecarray[i].z, var_C)),
				var_14);
			var_14 = multiply_and_scale(arg_vecarray[i].z, var_2);
			vertex.z = LEGACY_S16_WRAP_ADD(
				LEGACY_S16_WRAP_ADD(arg_vecptr[0].z,
					multiply_and_scale(arg_vecarray[i].x, var_C)),
				var_14);
			shape3d_vertex_write(shape,
				LEGACY_U16_WRAP_ADD(first_vertex, i), &vertex);
		}
		for (i = CAR_WHEEL_VERTEX_GROUP_SIZE;
			i < CAR_STEERED_WHEEL_VERTEX_COUNT; i++) {
			shape3d_vertex_read(shape,
				LEGACY_U16_WRAP_ADD(first_vertex, i), &vertex);
			var_10 = multiply_and_scale(arg_vecarray[i].x, var_2);
			vertex.x = LEGACY_S16_WRAP_ADD(
				LEGACY_S16_WRAP_ADD(arg_vecptr[1].x,
					multiply_and_scale(arg_vecarray[i].z, var_C)),
				var_10);
			var_10 = multiply_and_scale(arg_vecarray[i].z, var_2);
			vertex.z = LEGACY_S16_WRAP_ADD(
				LEGACY_S16_WRAP_ADD(arg_vecptr[1].z,
					multiply_and_scale(arg_vecarray[i].x, var_C)),
				var_10);
			shape3d_vertex_write(shape,
				LEGACY_U16_WRAP_ADD(first_vertex, i), &vertex);
		}
		arg_8[CAR_WHEEL_STEERING_CACHE_INDEX] = arg_4;
	}

	for (j = 0; j < CAR_WHEEL_COUNT; j++) {

		// The original takes |x|, shifts that right six, then re-applies the
		// sign of x (loc_2069F: cwd / xor / sub, sar ax,6, xor / sub).
		var_8 = arg_6[j];
		if (var_8 < 0)
			var_8 = LEGACY_S16_WRAP_NEGATE(var_8);
		var_8 = LEGACY_S16_SAR(var_8, CAR_WHEEL_VERTICAL_SCALE_SHIFT);
		if (arg_6[j] < 0)
			var_8 = LEGACY_S16_WRAP_NEGATE(var_8);

		if (arg_8[j] == var_8)
			continue;
		i = j * CAR_WHEEL_VERTEX_GROUP_SIZE;
		var_4 = i + CAR_WHEEL_VERTEX_GROUP_SIZE;

		for (; i < var_4; i++) {
			shape3d_vertex_read(shape,
				LEGACY_U16_WRAP_ADD(first_vertex, i), &vertex);
			vertex.y = LEGACY_S16_WRAP_SUB(arg_vecarray[i].y, var_8);
			shape3d_vertex_write(shape,
				LEGACY_U16_WRAP_ADD(first_vertex, i), &vertex);
		}
		arg_8[j] = var_8;
	}

	return ;
}

void shape3d_free_car_shapes() {
	if (car2resptr != 0) {
		sub_204AE(&game3dshapes[OPPONENT_CAR_WHEEL_SHAPE],
			CAR_FIRST_WHEEL_VERTEX, 0, unk_3E710,
			word_4448A, oppcarshapevecs, oppcarshapevec);
		mmgr_release(car2resptr);
	}
	sub_204AE(&game3dshapes[PLAYER_CAR_WHEEL_SHAPE],
		CAR_FIRST_WHEEL_VERTEX, 0, unk_3E710,
		word_443E8, carshapevecs, carshapevec);
	mmgr_free(carresptr);
}
