#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define plane_signed_distance frame_test_plane_distance
#define subst_hillroad_track frame_test_subst_hillroad_track
#define transform_wheel_travel_to_world frame_test_transform_wheel_travel
#include "../c/frame.c"

#undef printf

struct TRACKOBJECT trkObjectList[215];
legacy_s16 camera_track_height_offset;

static uint64_t trace_hash = UINT64_C(1469598103934665603);
static legacy_s16 fixture_plane_distance;
static legacy_s16 transform_stop_at;
static legacy_s16 transform_count;
static legacy_s16 rejected_shape;
static struct VECTOR flag_vertices[4];
static legacy_u8 terrain_map[900];
static legacy_u8 element_map[900];
static legacy_u8 sign_map[900];

static void trace_word(legacy_u16 value)
{
	trace_hash = (trace_hash ^ (value & 255U)) * UINT64_C(1099511628211);
	trace_hash = (trace_hash ^ (value >> 8)) * UINT64_C(1099511628211);
}

static void trace_vector(const struct VECTOR *vector)
{
	trace_word(vector->x);
	trace_word(vector->y);
	trace_word(vector->z);
}

void build_track_object(struct VECTOR *first, struct VECTOR *second)
{
	assert(first == second);
	trace_word(1);
	trace_vector(first);
}

legacy_s16 plane_signed_distance(legacy_s16 plane, legacy_s16 x, legacy_s16 y, legacy_s16 z)
{
	trace_word(2);
	trace_word(plane);
	trace_word(x);
	trace_word(y);
	trace_word(z);
	return fixture_plane_distance;
}

void transform_wheel_travel_to_world(void)
{
	trace_word(3);
	trace_vector(&wheel_forward_travel);
	trace_word(planindex_copy);
	trace_word(wheel_heading_offset);
	trace_word(car_initial_pitch);
	trace_word(car_initial_roll);
	trace_word(car_initial_yaw);
	wheel_world_travel.x = -5;
	wheel_world_travel.y = wheel_forward_travel.y;
	wheel_world_travel.z = 7;
}

static void trace_shape(const struct TRANSFORMEDSHAPE3D *shape)
{
	trace_word(shape->shapeptr == 0 ? 65535U : (legacy_u16)(shape->shapeptr - game3dshapes));
	trace_vector(&shape->pos);
	trace_vector(&shape->rotvec);
	trace_word(shape->culling_distance);
	trace_word(shape->ts_flags);
	trace_word(shape->material);
	trace_word(shape->rectptr == &frame_unsorted_shapes_rect);
	trace_word(shape->rectptr == &frame_sorted_shapes_rect);
}

legacy_u16 shape3d_transform_and_queue(struct TRANSFORMEDSHAPE3D *shape)
{
	trace_word(4);
	trace_shape(shape);
	trace_word(backlights_paint_override);
	transform_count++;
	if (transform_count == transform_stop_at) {
		return 1;
	}
	return transform_count == rejected_shape ? 65535U : 0;
}

void shape3d_vertex_read(const struct SHAPE3D *shape, legacy_u16 index, struct VECTOR *destination)
{
	assert(shape == &game3dshapes[FRAME_START_FLAG_RESOURCE_OFFSET / sizeof(struct SHAPE3D)]);
	trace_word(5);
	trace_word(index);
	*destination = flag_vertices[index - FRAME_START_FLAG_FIRST_VERTEX];
}

void shape3d_vertex_write(struct SHAPE3D *shape, legacy_u16 index, const struct VECTOR *source)
{
	assert(shape == &game3dshapes[FRAME_START_FLAG_RESOURCE_OFFSET / sizeof(struct SHAPE3D)]);
	trace_word(6);
	trace_word(index);
	trace_vector(source);
	flag_vertices[index - FRAME_START_FLAG_FIRST_VERTEX] = *source;
}

legacy_u8 subst_hillroad_track(legacy_u8 terrain, legacy_u8 element)
{
	trace_word(7);
	trace_word(terrain);
	trace_word(element);
	return element;
}

static void test_camera_modes(void)
{
	memset(&state, 0, sizeof(state));
	state.playerstate.car_position.lx = -80000;
	state.playerstate.car_position.ly = 1280;
	state.playerstate.car_position.lz = 96000;
	state.opponentstate.car_position.lx = 120000;
	state.opponentstate.car_position.ly = -640;
	state.opponentstate.car_position.lz = -40000;
	static struct VECTOR track_cameras[] = {{200, -20, 700}, {-400, 40, -300}};
	state.game_follow_camera_position[0] = track_cameras[1];
	state.game_follow_camera_position[1] = track_cameras[0];
	state.game_trackside_camera_index[0] = 0;
	state.game_trackside_camera_index[1] = 1;
	trackside_camera_positions = track_cameras;
	simd_player.car_height = 60;
	simd_opponent.car_height = 90;
	camera_track_height_offset = 25;
	planindex = 2;
	struct FRAME_CAMERA camera;
	for (unsigned int scenario = 0; scenario < 64U; scenario++) {
		memset(&camera, 0, sizeof(camera));
		followOpponentFlag = (scenario / 4U) % 2U;
		cameramode = scenario % 4U;
		state.playerstate.car_rotate.x = (legacy_s16)(scenario * 73U);
		state.playerstate.car_rotate.y = (legacy_s16)(scenario * 7U);
		state.playerstate.car_rotate.z = (legacy_s16)(scenario % 4U);
		state.opponentstate.car_rotate.x = (legacy_s16)(-300 + scenario * 37U);
		state.opponentstate.car_rotate.y = -20;
		state.opponentstate.car_rotate.z = (legacy_s16)(1023 - scenario % 4U);
		custom_camera.distance = 900 + scenario;
		custom_camera.elevation_angle = 50 + scenario;
		custom_camera.azimuth_angle = -20 + scenario;
		terrainHeight = scenario % 2U == 0 ? -100 : 600;
		track_wall_collision_enabled = (scenario / 8U) % 2U;
		fixture_plane_distance = (scenario / 16U) * 10;
		frame_setup_camera(&camera);
		trace_vector(&camera.position);
		trace_word(camera.pitch);
		trace_word(camera.yaw);
		trace_word(camera.roll);
	}
}

static void test_covered_tiles(void)
{
	struct FRAME_TILE tile;
	memset(&tile, 0, sizeof(tile));
	static const legacy_s8 offsets[] = {-128, -1, 0, 126, 127};
	struct FRAME_LOOKAHEAD_TILE lookahead[24];
	struct FRAME_TILE_SELECTION tiles;
	for (unsigned int flag = 0; flag < 5U; flag++) {
		trkObjectList[1].ss_multiTileFlag = flag;
		for (unsigned int east = 0; east < 5U; east++) {
			for (unsigned int south = 0; south < 5U; south++) {
				memset(&tiles, 0, sizeof(tiles));
				tiles.lookahead = lookahead;
				tile.element = 1;
				tile.east = offsets[east];
				tile.south = offsets[south];
				for (unsigned int index = 0; index < 24U; index++) {
					lookahead[index].east = LEGACY_S8_WRAP_ADD(offsets[east], (index % 3U) - 1U);
					lookahead[index].south =
						LEGACY_S8_WRAP_ADD(offsets[south], (index / 3U) % 3U - 1U);
					tiles.markers[index] = index % 3U;
				}
				frame_mark_covered_tiles(&tiles, &tile, 22);
				for (unsigned int index = 0; index < 24U; index++) {
					trace_word(tiles.markers[index]);
				}
			}
		}
	}
}

static void reset_shapes(void)
{
	memset(currenttransshape, 0, sizeof(currenttransshape));
	curtransshape_ptr = currenttransshape;
	transformedshape_counter = 0;
	transform_count = 0;
	transform_stop_at = 0;
	rejected_shape = 0;
	backlights_paint_override = BACKLIGHT_PAINT_DEFAULT;
	memset(&mat_temp, 0, sizeof(mat_temp));
	mat_temp.m._11 = mat_temp.m._22 = mat_temp.m._33 = 16384;
}

static void configure_track(void)
{
	memset(terrain_map, 0, sizeof(terrain_map));
	memset(element_map, 0, sizeof(element_map));
	memset(sign_map, 255, sizeof(sign_map));
	track_terrain_map = terrain_map;
	track_element_map = element_map;
	roadside_sign_indices_by_tile = sign_map;
	for (unsigned int index = 0; index < 30U; index++) {
		terrainrows[index] = trackrows[index] = index * 30U;
		track_column_centers[index] = index * 1024U + 512U;
		track_row_centers[index] = 30000 - index * 1024U;
		track_column_positions[index] = index * 1024U;
		track_row_positions[index] = 30512 - index * 1024U;
	}
}

static void test_tile_selection(void)
{
	struct FRAME_LOOKAHEAD_TILE lookahead[24];
	struct FRAME_CAMERA camera;
	struct FRAME_TILE_SELECTION tiles;
	for (unsigned int scenario = 0; scenario < 30U; scenario++) {
		memset(&camera, 0, sizeof(camera));
		memset(&tiles, 0x35, sizeof(tiles));
		configure_track();
		detail_level = scenario % 5U;
		camera.position.x = scenario % 2U == 0 ? 10240 : -512;
		camera.position.z = 19456;
		state.playerstate.car_position.lx = 10L * 65536L;
		state.playerstate.car_position.lz = 19L * 65536L;
		tiles.lookahead = lookahead;
		for (unsigned int index = 0; index < 24U; index++) {
			lookahead[index].east = (legacy_s8)(index % 5U - 2U);
			lookahead[index].south = (legacy_s8)(index / 5U - 2U);
			lookahead[index].detail = index % 3U;
		}
		trkObjectList[1].ss_multiTileFlag = scenario % 4U;
		trkObjectList[1].ss_physicalModel = scenario % 2U == 0 ? 64 : 63;
		element_map[10 + 10 * 30] = 1;
		element_map[11 + 10 * 30] = TRACK_TILE_CONTINUATION_EAST;
		element_map[10 + 11 * 30] = TRACK_TILE_CONTINUATION_SOUTH;
		element_map[11 + 11 * 30] = TRACK_TILE_CONTINUATION_SOUTHEAST;
		terrain_map[10 + 10 * 30] = scenario % 3U == 0 ? 7 : 0;
		frame_select_tiles(&tiles, &camera);
		for (unsigned int index = 0; index < 24U; index++) {
			trace_word(tiles.markers[index]);
			trace_word(tiles.east[index]);
			trace_word(tiles.south[index]);
			trace_word(tiles.elements[index]);
			trace_word(tiles.terrain[index]);
			trace_word(tiles.detail[index]);
		}
	}
}

static void test_terrain_exhaustion(void)
{
	struct FRAME_CAMERA camera;
	memset(&camera, 0, sizeof(camera));
	camera.position.x = -100;
	camera.position.y = 250;
	camera.position.z = 400;
	struct FRAME_TILE tile;
	for (unsigned int scenario = 0; scenario < 24U; scenario++) {
		configure_track();
		reset_shapes();
		memset(&tile, 0, sizeof(tile));
		tile.east = 4;
		tile.south = 5;
		tile.element = scenario % 3U == 0 ? 105 : 0;
		tile.terrain = scenario % 3U == 1 ? TERRAIN_RAISED_TILE : 1;
		terrain_map[4 + 5 * 30] = 1;
		terrain_map[5 + 5 * 30] = 2;
		terrain_map[4 + 6 * 30] = 0;
		terrain_map[5 + 6 * 30] = 3;
		transform_stop_at = scenario / 3U;
		trace_word(frame_draw_terrain(&tile, &camera, 8));
		trace_word(tile.terrain);
		trace_word(tile.height);
		trace_word(tile.last_east);
		trace_word(tile.last_south);
	}
}

static void test_sorted_shapes(void)
{
	struct FRAME_CAR_RENDER cars[2];
	for (unsigned int scenario = 0; scenario < 36U; scenario++) {
		reset_shapes();
		memset(cars, 0, sizeof(cars));
		state.playerstate.car_is_braking = scenario % 2U;
		state.opponentstate.car_is_braking = (scenario / 2U) % 2U;
		state.playerstate.car_crashBmpFlag = CRASH_EVENT_COLLISION;
		state.opponentstate.car_crashBmpFlag = scenario % 2U == 0 ? CRASH_EVENT_COLLISION : 0;
		transformedshape_counter = scenario % 5U;
		for (unsigned int index = 0; index < 5U; index++) {
			currenttransshape[index].shapeptr = &game3dshapes[index];
			transformed_shape_sort_types[index] = index % 4U;
			transformedshape_indices[index] = index;
			transformedshape_zarray[index] = (legacy_s16)((index * 7U + scenario) % 4U);
		}
		transform_stop_at = (scenario / 5U) % 4U;
		rejected_shape = scenario % 4U;
		frame_draw_sorted_shapes(cars);
		trace_word(cars[0].explosion_visible);
		trace_word(cars[1].explosion_visible);
	}
}

static void test_track_elements_and_flags(void)
{
	struct FRAME_CAMERA camera;
	memset(&camera, 0, sizeof(camera));
	camera.position.x = 100;
	camera.position.y = 200;
	camera.position.z = -100;
	struct FRAME_TILE tile;
	struct FRAME_CAR_RENDER cars[2];
	legacy_s8 overlay;
	for (unsigned int scenario = 0; scenario < 32U; scenario++) {
		configure_track();
		reset_shapes();
		memset(&tile, 0, sizeof(tile));
		memset(cars, 0, sizeof(cars));
		tile.east = 4;
		tile.south = 5;
		tile.element = scenario % 8U == 0 ? 0 : 1;
		tile.detail = scenario % 2U;
		tile.height = scenario % 3U == 0 ? 450 : 0;
		trkObjectList[1].ss_shapePtr = &game3dshapes[1];
		trkObjectList[1].ss_loShapePtr = &game3dshapes[2];
		trkObjectList[1].ss_multiTileFlag = scenario % 4U;
		trkObjectList[1].ss_ignoreZBias = (scenario / 4U) % 2U;
		trkObjectList[1].ss_surfaceType = scenario % 2U == 0 ? -1 : 2;
		trkObjectList[1].ss_ssOvelay = 0;
		trkObjectList[1].ss_rotY = scenario * 31U;
		start_finish_column = 4;
		start_finish_row = scenario % 2U == 0 ? 5 : 6;
		overlay = scenario % 2U;
		cars[0].depth_adjustment = cars[1].depth_adjustment = 2048;
		transform_stop_at = (scenario / 8U) % 3U;
		trace_word(frame_add_track_element(&tile, &camera, cars, 8, 3, &overlay));
		trace_vector(&tile.position);
		trace_word(tile.last_east);
		trace_word(tile.last_south);
		trace_word(tile.depth_mask);
		trace_word(overlay);
		trace_word(cars[0].depth_adjustment);
		trace_word(cars[1].depth_adjustment);
		state.game_inputmode = GAME_INPUT_MODE_WAITING;
		start_flag_animation = scenario * 33U;
		track_angle = scenario * 17U;
		hillFlag = scenario % 2U;
		for (unsigned int index = 0; index < 4U; index++) {
			flag_vertices[index].x = index;
			flag_vertices[index].y = index * 30U;
			flag_vertices[index].z = -10;
		}
		frame_add_start_flag(&tile, &camera, 8);
		trace_word(transformedshape_counter);
		for (unsigned int index = 0; index < (unsigned int)transformedshape_counter; index++) {
			trace_shape(&currenttransshape[index]);
			trace_word(transformedshape_zarray[index]);
		}
	}
}

int main(void)
{
	test_camera_modes();
	test_covered_tiles();
	test_tile_selection();
	test_terrain_exhaustion();
	test_sorted_shapes();
	test_track_elements_and_flags();
	/* Captured before extraction: camera modes, tile selection, queue exhaustion,
	 * sorted brake paint, component geometry and animated start-flag vertices. */
	assert(trace_hash == UINT64_C(0xcf35ecd7319fcd5c));
	puts("Frame rendering snapshots passed (311 scenarios).");
	return 0;
}
