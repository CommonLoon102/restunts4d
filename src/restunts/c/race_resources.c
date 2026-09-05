#include "audio_internal.h"
#include "dashboard.h"
#include "fileio.h"
#include "memmgr.h"
#include "menu_internal.h"
#include "race_resources.h"
#include "race_resources_internal.h"
#include "replay_viewer.h"
#include "shape2d.h"
#include "shape3d.h"
#include "ui_dialog.h"

#define SKYBOX_RESOURCE_COUNT 5
#define SKYBOX_RESOURCE_NAME_BYTES 9
#define SKYBOX_IMAGE_COUNT 4
#define SKYBOX_NO_IMAGES_FLAG 8U
#define SKYBOX_GROUND_MATERIAL_INDEX 16
#define SKYBOX_SKY_MATERIAL_INDEX 17
#define SKYBOX_WATER_MATERIAL_INDEX 100
#define SDGAME2_EFFECT_SHAPE_COUNT 3
#define PLAYER_ENGINE_LEGACY_TYPE 33
#define OPPONENT_ENGINE_LEGACY_TYPE 32
#define TRACK_SKYBOX_ELEMENT_INDEX 900U
#define RENDER_WINDOW_WIDTH 320U
#define RENDER_WINDOW_HEIGHT 200U
#define RENDER_WINDOW_LEGACY_ARGUMENT 15U
#define RENDER_WINDOW_PIXEL_BYTES 64000U
#define RENDER_WINDOW_ARENA_PARAGRAPHS 4002U

extern void far* engptr;
extern void far* eng1ptr;
extern void far* fontledresptr;
extern void far* sdgameresptr;
extern legacy_s8 unk_3E7FC[];
extern legacy_s8 unk_3E82C[];

static legacy_s8 skybox_resource_names[SKYBOX_RESOURCE_COUNT]
	[SKYBOX_RESOURCE_NAME_BYTES] = {
	"desert",
	"tropical",
	"alpine",
	"city",
	"country"
};

void unload_skybox(void)
{
	if (byte_3B8F6 != 0)
		mmgr_free(skybox_res_ofs);
	byte_3B8F6 = 0;
}

void free_sdgame2(void)
{
	mmgr_free(sdgame2ptr);
}

void load_sdgame2_shapes(void)
{
	legacy_s16 i;

	sdgame2ptr = file_load_resource(FILE_RESOURCE_SHAPE2D_ALTERNATE,
		"sdgame2");
	locate_many_resources(
		sdgame2ptr,
		"ex01ex02ex03leftrigh",
		sdgame2shapes);
	for (i = 0; i < SDGAME2_EFFECT_SHAPE_COUNT; i++)
		sdgame2_widths[i] = shape2d_get_width(
			(struct SHAPE2D far*)sdgame2shapes[i]);
}

void load_skybox(legacy_s8 skybox_index)
{
	legacy_u16 minimum;
	legacy_u16 maximum;
	legacy_u16 image_index;

	if (((legacy_u8)skybox_index & SKYBOX_NO_IMAGES_FLAG) == 0) {
		if (byte_3B8F6 != 0 &&
			(legacy_u8)skybox_index == (legacy_u8)byte_46167)
			return;

		unload_skybox();
		byte_46167 = skybox_index;
		byte_3B8F6 = 1;
		skybox_res_ofs = file_load_shape2d_fatal(
			skybox_resource_names[(legacy_s8)skybox_index]);
		locate_many_resources(
			skybox_res_ofs,
			"scensce2sce3sce4",
			skyboxes);

		for (image_index = 0; image_index < SKYBOX_IMAGE_COUNT;
			image_index++) {
			skybox.heights[image_index] = shape2d_get_height(
				(struct SHAPE2D far*)skyboxes[image_index]);
		}
		minimum = skybox.heights[0];
		maximum = skybox.heights[0];
		for (image_index = 1; image_index < SKYBOX_IMAGE_COUNT;
			image_index++) {
			if (minimum > skybox.heights[image_index])
				minimum = skybox.heights[image_index];
			if (maximum < skybox.heights[image_index])
				maximum = skybox.heights[image_index];
		}
		skybox.minimum_height = minimum;
		skybox.maximum_height = maximum;
	}

	skybox.sky_color = material_clrlist_ptr[SKYBOX_SKY_MATERIAL_INDEX];
	skybox.ground_color = material_clrlist_ptr[SKYBOX_GROUND_MATERIAL_INDEX];
	skybox.water_color = material_clrlist_ptr[SKYBOX_WATER_MATERIAL_INDEX];
	meter_needle_color = dialog_fnt_colour;
}

static legacy_s16 setup_player_cars_impl(legacy_s16 load_dashboard_shapes) {
	void far* carresptr;
	legacy_u32 var_8;

	setup_legacy_penalty_route_word();
	render_window_sprite = 0;
	ensure_file_exists(2);
	shape3d_load_car_shapes(gameconfig.game_playercarid, gameconfig.game_opponentcarid);
	aCarcoun[3] = gameconfig.game_playercarid[0];
	aCarcoun[4] = gameconfig.game_playercarid[1];
	aCarcoun[5] = gameconfig.game_playercarid[2];
	aCarcoun[6] = gameconfig.game_playercarid[3];
	carresptr = file_load_resfile(aCarcoun);
	setup_aero_trackdata(carresptr, 0);
	unload_resource(carresptr);

	if (gameconfig.game_opponenttype != 0) {
		aCarcoun[3] = gameconfig.game_opponentcarid[0];
		aCarcoun[4] = gameconfig.game_opponentcarid[1];
		aCarcoun[5] = gameconfig.game_opponentcarid[2];
		aCarcoun[6] = gameconfig.game_opponentcarid[3];
		carresptr = file_load_resfile(aCarcoun);
		setup_aero_trackdata(carresptr, 1);
		unload_resource(carresptr);

		ensure_file_exists(4);
		load_opponent_data();
	}

	ensure_file_exists(3);
	eng1ptr = file_load_resource(FILE_RESOURCE_VOICE, "eng1");
	engptr = file_load_resource(FILE_RESOURCE_SOUND_EFFECTS, "eng");
	audio_add_driver_timer();
	audio_player_engine_channel = audio_init_engine(
		PLAYER_ENGINE_LEGACY_TYPE, &unk_3E7FC, eng1ptr, engptr);

	audio_car_state_ready = 0;
	audio_player_car_flags = 0;
	audio_opponent_car_flags = 0;
	if (gameconfig.game_opponenttype != 0) {
		audio_opponent_engine_channel = audio_init_engine(
			OPPONENT_ENGINE_LEGACY_TYPE, &unk_3E82C, eng1ptr, engptr);
	}

	audio_car_state_read_index = 0;
	audio_car_state_write_index = 0;
	audio_car_state_interval = 0;
	fontledresptr = file_load_resource(FILE_RESOURCE_BINARY_FATAL,
		"fontled.fnt");
	slow_video_mgmt_copy = slow_video_mgmt;
	init_rect_arrays();
	/* REPLDUMP advances simulation without rendering the dashboard.  Keep the
	 * car 3D container in its original arena position because later legacy
	 * state still observes that memory layout, but avoid the much larger 2D
	 * dashboard allocation that memory-heavy custom cars cannot afford. */
	if (idle_expired == 0 && load_dashboard_shapes) {
		setup_car_shapes(0);
	}

	if (idle_expired == 0) {
		sdgameresptr = file_load_resource(FILE_RESOURCE_SHAPE2D_COLLECTION,
			"sdgame");
		loop_game(0, 0, 0);
	}

	load_track_collision_resources();
	load_sdgame2_shapes();
	load_skybox(td14_elem_map_main[TRACK_SKYBOX_ELEMENT_INDEX]);
	if (shape3d_load_all() != 0) {
		return 1;
	}

	if (video_flag5_is0 == 0) {
		// The free-arena check only applies when the window has to come from
		// the arena; the paragraph count covers all pixels and its header.
		if (!highpool_can_fit(RENDER_WINDOW_ARENA_PARAGRAPHS)) {
			var_8 = LEGACY_U16_DIV_OR_ZERO(RENDER_WINDOW_PIXEL_BYTES,
				LEGACY_U16_WRAP_MUL(
					video_flag1_is1, video_flag4_is1));
			if (mmgr_get_res_ofs_diff_scaled() <= var_8) {
				return 1;
			}
		}
		render_window_sprite = sprite_make_wnd(
			RENDER_WINDOW_WIDTH, RENDER_WINDOW_HEIGHT,
			RENDER_WINDOW_LEGACY_ARGUMENT);
	}

	followOpponentFlag = 0;
	is_in_replay_copy = -1;
	return 0;
}

legacy_s16 setup_player_cars(void) {
	return setup_player_cars_impl(1);
}

legacy_s16 setup_player_cars_repldump(void) {
	return setup_player_cars_impl(0);
}

void free_player_cars(void) {
	if (video_flag5_is0 == 0) {
		if (render_window_sprite != 0) {
			sprite_free_wnd(render_window_sprite);
		}
	}
	shape3d_free_all();
	unload_skybox();
	free_sdgame2();
	unload_resource(gameresptr);
	if (idle_expired == 0) {
		mmgr_free(sdgameresptr);
		setup_car_shapes(3);
	}

	mmgr_free(fontledresptr);
	audio_remove_driver_timer();
	mmgr_free(engptr);
	mmgr_free(eng1ptr);
	shape3d_free_car_shapes();
}
