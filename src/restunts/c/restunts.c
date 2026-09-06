#include <stddef.h>
#include <stdarg.h>
#include "audio.h"
#include "audio_internal.h"
#include "dashboard.h"
#include "restunts.h"
#include "fileio.h"
#include "fatal.h"
#include "game_input.h"
#include "keyboard.h"
#include "legacy.h"
#include "math.h"
#include "memmgr.h"
#include "menu_internal.h"
#include "platform.h"
#include "race.h"
#include "race_resources.h"
#include "race_resources_internal.h"
#include "replay.h"
#include "replay_record.h"
#include "replay_viewer.h"
#include "replay_viewer_internal.h"
#include "shape2d.h"
#include "shape3d.h"
#include "timing.h"
#include "ui_dialog.h"
#include "ui_input.h"
#include "ui_text.h"

#define GAME_SCREEN_WIDTH 320
#define GAME_SCREEN_HEIGHT 200
#define GAME_TOP_PANEL_HEIGHT 100
#define GAME_INITIAL_CLIP_BOTTOM 95
#define GAME_SCREEN_COLOR 15U

#define RANDOM_WAIT_SPIN_LIMIT 12000
#define RANDOM_WAIT_RESOURCE_INDEX 1024
#define NO_OPPONENT_CAR_ID 255U

#define CALLBACK_GRAPHICS_MENU_KEY 7
#define CALLBACK_JOYSTICK_HELP_KEY 10
#define CALLBACK_KEYBOARD_HELP_KEY 11
#define CALLBACK_MOUSE_HELP_KEY 12800
#define CALLBACK_PAUSE_HELP_KEY 16
#define CALLBACK_DOS_HELP_KEY 17
#define CALLBACK_SOUND_HELP_KEY 19
#define CALLBACK_DOS_HELP_ALT_KEY 24

#define STARTUP_PROJECTION_X 36
#define STARTUP_PROJECTION_Y 17
#define STARTUP_SHAPE_COUNT 116
#define STARTUP_CAR_POSITION_Y_BITS 64696U
#define STARTUP_CAR_POSITION_Z 2880

#define TRACK_PATH_STORAGE_SIZE 81
#define TRACK_PRIMARY_PATH_OFFSET REPLAY_TRACK_SIZE
#define TRACK_SECONDARY_PATH_OFFSET \
	(REPLAY_TRACK_SIZE + TRACK_PATH_STORAGE_SIZE)


legacy_s16 video_backbuffer_copy_required(void)
{
	return 0;
}

static void shutdown_dos_game(void)
{
	mouse_draw_opaque_check();
	dos_timer_shutdown();
	dos_audio_shutdown();
	kb_exit_handler();
	dos_kb_set_numlock();
	dos_video_set_mode7();
}

extern void far frame_callback(void);

legacy_s16 camera_track_height_offset;

void update_opponent(void)
{
	update_opponent_tick();
}

legacy_s16 get_super_random(void)
{
	legacy_s16 val = rand() + get_kevinrandom() + timer_get_counter() + gState_frame;
	return val < 0 ? -val : val;
}

legacy_s16 random_wait(void)
{
	legacy_s16 status1, i;

	status1 = dos_video_get_status();

	for (i = 0; status1 == dos_video_get_status() &&
		i < RANDOM_WAIT_SPIN_LIMIT; ++i);

	if (i == RANDOM_WAIT_RESOURCE_INDEX) {
		i = random_wait_legacy_bytes[0];
	}

	while (i--) {
		rand();
		get_kevinrandom();
	}

	i &= LEGACY_U8_MAX;

	while (i--) {
		get_kevinrandom();
		rand();
	}

	return 0;
}

void set_default_car(void) {
	gameconfig.game_playercarid[0]     = 'C';
	gameconfig.game_playercarid[1]     = 'O';
	gameconfig.game_playercarid[2]     = 'U';
	gameconfig.game_playercarid[3]     = 'N';
	gameconfig.game_playermaterial     = 0;
	gameconfig.game_playertransmission = TRANSMISSION_AUTOMATIC;
	gameconfig.game_opponenttype       = 0;
	gameconfig.game_opponentmaterial   = 0;
	gameconfig.game_opponentcarid[0]   = NO_OPPONENT_CAR_ID;
}


extern legacy_u16 select_cliprect_rotate(legacy_s16 angX, legacy_s16 angY, legacy_s16 angZ, struct RECTANGLE* cliprect, legacy_s16 half_scale);
//extern void shape3d_transform_and_queue(struct TRANSFORMSHAPE3D* shape);
extern void set_projection(legacy_s16, legacy_s16, legacy_s16, legacy_s16);

struct RECTANGLE shaperect = {
	0, GAME_SCREEN_WIDTH, 0, GAME_SCREEN_HEIGHT
};
struct TRANSFORMEDSHAPE3D transshape;
struct RECTANGLE cliprect = { 0, GAME_SCREEN_WIDTH,
	0, GAME_INITIAL_CLIP_BOTTOM };
struct VECTOR carpos = { 0, LEGACY_S16_FROM_BITS(
	STARTUP_CAR_POSITION_Y_BITS), STARTUP_CAR_POSITION_Z }; // from the original
//struct VECTOR carpos = { 0, 0, GAME_SCREEN_WIDTH };

struct SPRITE far* render_window_sprite;
// empty_rect uses the legacy sentinel rectangle <9999, 65535, 9999, 65535>.

extern legacy_s16 polyinfonumpolys;
extern legacy_u8 far* polyinfoptrs[]; // array size = 400
extern legacy_u16 polygon_next_index[]; // array size = 400


extern legacy_s16 font_prefix_width(const legacy_s8* text, legacy_s16 count);


void audio_sequence_timer(void);
extern void audio_map_song_instruments(void far* song,
	void far* instruments);
extern void audio_map_song_tracks(void far* song);
extern void sprite_xor_rect_clipped(legacy_s16 x, legacy_s16 y, legacy_s16 width, legacy_s16 height, legacy_s16 color);
void audio_release_channel_range(legacy_s16 first_channel,
	legacy_s16 last_channel);
extern void audio_play_bump(legacy_s16 channel);
extern void audio_play_scrape(legacy_s16 channel);
extern legacy_s16 audio_play_effect_at_rate(void far* resource, legacy_s16 channel,
	legacy_u8 priority, legacy_u16 rate);
extern void audio_init_channel_range(legacy_s16 first_channel, legacy_s16 last_channel,
	void far* resource, legacy_u16 resource_data_offset,
	legacy_u16 rate, legacy_u8 priority);

extern legacy_s8 missing_disk1_message_id[];
extern legacy_s8 missing_disk2_message_id[];
extern legacy_s8 missing_disk3_message_id[];
extern legacy_s8 missing_disk4_message_id[];
extern legacy_s8 disk_retry_dialog_id[];
extern legacy_s8 disk_error_dialog_id[];
extern legacy_s8 file_save_dialog_id[];
extern legacy_s8 waiting_message_id[];
extern legacy_s8 file_load_dialog_id[];
extern legacy_s8 file_scroll_up_label_id[];
extern legacy_s8 file_scroll_down_label_id[];
extern legacy_s8* findfilenames[];
extern struct TRACKOBJECT trkObjectList[];
extern struct SHAPE2D far* track_editor_terrain_shapes[];
extern struct SHAPE2D far* track_editor_tile_shapes[];
extern struct SHAPE2D far* track_editor_tile_masks[];
legacy_s16 call_read_line(legacy_s8* text, legacy_s16 max_characters, legacy_s16 x, legacy_s16 y,
	legacy_u32 timeout);
legacy_s8 do_fileselect_dialog(legacy_s8* directory, legacy_s8* filename,
	legacy_s8* extension, legacy_s8 far* prompt);
void show_insufficient_memory_dialog(void);
struct RECTANGLE* intro_draw_text(legacy_s8* text, legacy_s16 x, legacy_s16 y, legacy_s16 color,
	legacy_s16 shadow_color);
legacy_u8 subst_hillroad_track(legacy_u8 terrain, legacy_u8 track);

extern void sprite_draw_rect_outline(legacy_s16 x, legacy_s16 y, legacy_s16 width, legacy_s16 height, legacy_s16 color);


extern legacy_s8 gnam_string[];
extern legacy_s8 gsna_string[];
extern legacy_s8 opponent_highscore_name[];
extern legacy_s8 highscore_player_name_input[];

extern void far* engptr;
extern void far* eng1ptr;
extern void far* fontledresptr;
extern void far* sdgameresptr;
extern legacy_s8 player_engine_definition[];
extern legacy_s8 opponent_engine_definition[];
extern legacy_s16 audio_init_engine(legacy_s16, void far*, void far*, void far*);

extern void update_car_speed(legacy_s8 input, legacy_s16 car_index,
	struct CARSTATE* carstate, struct SIMD* simd);

void init_div0(void)
{
	dos_install_divide_error_handler();
}

void copy_material_list_pointers(void* clrlist, void* clrlist2, void* patlist, void* patlist2, legacy_u16 reserved_video_word)
{
	material_clrlist_ptr_cpy = clrlist;
	material_clrlist2_ptr_cpy = clrlist2;
	material_patlist_ptr_cpy = patlist;
	material_patlist2_ptr_cpy = patlist2;
	reserved_material_video_word = reserved_video_word;
}

void init_main(legacy_s16 argc, legacy_s8* argv[])
{
	legacy_u16 i, j;
	legacy_u8 mode4_requested, sound_disabled, unused_nd_option;
	legacy_u32 full_clear_ticks, partial_redraw_ticks, geometry_benchmark_ticks;
	struct POINT2D benchmark_point;
	struct RECTANGLE benchmark_bounds;

	// Keyboard
	kb_init_interrupt();
	dos_kb_clear_numlock();
	kb_call_readchar_callback();

	kb_reg_callback(CALLBACK_GRAPHICS_MENU_KEY, &show_graphic_levels_menu);
	kb_reg_callback(CALLBACK_JOYSTICK_HELP_KEY, &calibrate_joystick_driving);
	kb_reg_callback(CALLBACK_KEYBOARD_HELP_KEY, &select_keyboard_driving);
	kb_reg_callback(CALLBACK_MOUSE_HELP_KEY, &toggle_music_with_dialog);
	kb_reg_callback(CALLBACK_PAUSE_HELP_KEY, &show_pause_dialog);
	kb_reg_callback('p', &show_pause_dialog);
	kb_reg_callback(CALLBACK_DOS_HELP_KEY, &show_exit_to_dos_dialog);
	kb_reg_callback(CALLBACK_SOUND_HELP_KEY, &toggle_effects_with_dialog);
	kb_reg_callback(CALLBACK_DOS_HELP_ALT_KEY, &show_exit_to_dos_dialog);

	// Video
	init_video_geometry_flags();

	mmgr_init_conventional_arena();
	himem_init();
	audio_allocate_car_state_records();

	video_uses_page_flipping = 0;
	video_page_count = 1;

	textresprefix = 'e';

	// Parse arguments.
	mode4_requested = 0;
	sound_disabled = 0;
	unused_nd_option = 0;

	for (i = 1; argc > i; ++i) {
		if (argv[i][0] == '/') {
			switch (argv[i][1]) {
				case 'h':
					mode4_requested = 4;
					break;

				case 'n':
					if (argv[i][2] == 's') {
						sound_disabled = 1;
					}
					else if (argv[i][2] == 'd') {
						unused_nd_option = 1;
					}
					break;

				case 's':
				if (strlen(argv[i]) >= 4) {
					if (
						   (argv[i][2] == 'S' || argv[i][2] == 's')
						&& (argv[i][3] == 'B' || argv[i][3] == 'b'))
					{
						// We do not have Sound Blaster drivers.
						// Replace them with Adlib
						audiodriverstring[0] = 'a';
						audiodriverstring[1] = 'd';
					}
					else {
						audiodriverstring[0] = argv[i][2];
						audiodriverstring[1] = argv[i][3];
					}
					break;
				}
			}
		}
	}

	// Unused "/nd" switch. Maybe used when loading other video drivers?
	(void)unused_nd_option;

	// Video mode.
	dos_video_set_mode_13h();
	if (mode4_requested) {
		dos_video_set_mode4();
	}

	dos_timer_setup_interrupt();

	sprite_select_screen_and_clear();

	dos_mouse_init(GAME_SCREEN_WIDTH, GAME_SCREEN_HEIGHT);

	// Audio driver.
	if (audio_load_dos_driver(audiodriverstring, 0, 0)) {
		dos_timer_shutdown();
		dos_process_exit(1);
	}

	if (sound_disabled) {
		audio_toggle_music();
		audio_toggle_effects();
	}

	dos_set_critical_error_handler(&show_disk_error_dialog);

	load_palandcursor();

	// Timing measures.
	sprite_select_screen();
	sprite_set_target_clip_bounds(0, 320, 0, 120);

	timer_get_delta_alt();
	for (i = 0; i < 15; ++i) {
		sprite_clear_target(0);
	}
	full_clear_ticks = timer_get_delta_alt();

	sprite_set_target_clip_bounds(0, 320, 0, 60);

	for (i = 0; i < 15; ++i) {
		benchmark_bounds.left = benchmark_bounds.right = benchmark_bounds.top = benchmark_bounds.bottom = 0;

		for (j = 0; j < 400; ++j) {
			benchmark_point.px = benchmark_point.py = j;
			rect_adjust_from_point(&benchmark_point, &benchmark_bounds);
		}

		sprite_clear_target(0);
	}

	partial_redraw_ticks = timer_get_delta_alt();

	for (i = 0; i < 146; ++i) {
		for (j = 0; j < 255; ++j) {
			rect_adjust_from_point(&benchmark_point, &benchmark_bounds);
		}
	}

	geometry_benchmark_ticks = timer_get_delta_alt();

	slow_video_mgmt = (partial_redraw_ticks <= full_clear_ticks);
	configured_frame_rate = (geometry_benchmark_ticks >= 75) ?
		GAME_FRAME_RATE_LOW : GAME_FRAME_RATE_NORMAL;

	if (geometry_benchmark_ticks < 35) {
		detail_level = 0;
	}
	else if (geometry_benchmark_ticks < 55) {
		detail_level = 1;
	}
	else if (geometry_benchmark_ticks < 75) {
		detail_level = 2;
	}
	else if (geometry_benchmark_ticks < 100) {
		detail_level = 3;
	}
	else if (slow_video_mgmt) {
		detail_level = 4;
	}
	else {
		detail_level = 3;
	}

	framespersec = configured_frame_rate;
	slow_video_mgmt_copy = slow_video_mgmt;

	random_wait();

	copy_material_list_pointers(material_clrlist_ptr, material_clrlist2_ptr, material_patlist_ptr, material_patlist2_ptr, 0);
}

static void init_full_game(legacy_s16 argc, legacy_s8* argv[])
{
	init_main(argc, argv);
	init_div0();
	init_row_tables();

	mainresptr = file_load_resfile("main");
	fontdefptr = file_load_resource(FILE_RESOURCE_BINARY_FATAL,
		"fontdef.fnt");
	fontnptr = file_load_resource(FILE_RESOURCE_BINARY_FATAL,
		"fontn.fnt");

	font_set_fontdef();
	init_polyinfo();
	init_trackdata();

	reset_race_loop_state();

	init_kevinrandom("kevin");

	strcpy(gameconfig.game_trackname, "DEFAULT");
}

static void init_main_input_state(void)
{
	input_do_checking(1);
	input_do_checking(1);
	mouse_draw_opaque_check();
	kbormouse = 0;
	passed_security = 1;  // set to 0 for the original copy protection
}

legacy_s16 run_shape_preview(legacy_s16 argc, legacy_s8* argv[]) {
	legacy_s16 result;
	legacy_s8 far* textresptr;
	legacy_s16 carposangle;
	struct SPRITE far* unused_preview_window;
	legacy_s16 counter;
	legacy_s16 input_flags;
	legacy_s16 shapeindex;

	// initialization
	init_full_game(argc, argv);
	init_main_input_state();
	//set_default_car();

	// try do something
	menu_reset_animation_timers();
	set_projection(STARTUP_PROJECTION_X, STARTUP_PROJECTION_Y,
		GAME_SCREEN_WIDTH, GAME_TOP_PANEL_HEIGHT);
	// The camera projection would at best draw a pixel without this setup.

	render_window_sprite = sprite_make_wnd(
		GAME_SCREEN_WIDTH, GAME_TOP_PANEL_HEIGHT, GAME_SCREEN_COLOR);

	//run_intro_looped();

	carposangle = polarAngle(carpos.y, carpos.z);

	shape3d_load_all();
	shape3d_load_car_shapes("coun", "coun");
	select_cliprect_rotate(0, carposangle, 0, &cliprect, 0);

	//shaperect = cliprect;
	transshape.material = 0;
	transshape.rotvec.x = 0;
	transshape.rotvec.y = 0;
	transshape.pos = carpos;

	transshape.culling_distance = 0; // The abandoned experiment used 30000.
	transshape.ts_flags = 0;
	transshape.rectptr = &shaperect;

	counter = 0;
	shapeindex = 24;
	for (; ; counter++) {

		transshape.rotvec.z = 0; // An abandoned experiment added 560.

		// The original adds 2728 bytes, or 124 22-byte SHAPE3D records,
		// to reach the first car shape loaded by shape3d_load_car_shapes().

		transshape.shapeptr = &game3dshapes[shapeindex];

		//transshape.shapeptr = &game3dshapes[124];
		//transshape.shapeptr = &game3dshapes[124];

		shape3d_transform_and_queue(&transshape);

		sprite_select_render_window();
		sprite_clear_target(3);

		//sprite_set_target_clip_bounds(50, 200, 50, 100);
		shape3d_render_queued_primitives(); // renders to drawing_sprite

		//sprite_select_screen_compat();
		sprite_blit_to_video(render_window_sprite, 0);

		input_flags = get_kb_or_joy_flags();//kb_get_char();
		if (input_flags == 4) { // right
			shapeindex++;
			shapeindex = (shapeindex + STARTUP_SHAPE_COUNT) %
				STARTUP_SHAPE_COUNT;
		} else
		if (input_flags == 8) { // left
			shapeindex--;
			shapeindex = (shapeindex + STARTUP_SHAPE_COUNT) %
				STARTUP_SHAPE_COUNT;
		} else
		if (input_flags != 0) {
			textresptr = locate_text_res(mainresptr, "dos");
			// DIALOG_AUTO_POSITION centers both dialog coordinates.
			result = show_dialog(DIALOG_TYPE_MENU, DIALOG_SAVE_BACKGROUND,
				textresptr, 0, 170, dialog_border_color, 0, 0);
			if (result >= 1)
				break;
		}
	}

	//unused_preview_window = sprite_make_wnd(320, 200);
	//sprite_select_mcga_backbuffer();
	//sprite_set_target_clip_bounds(0, 320, 0, 200);
	//sprite_select_screen_compat();
	//sprite_clear_target(2);
		//sprite_select_render_window();
		//sprite_select_screen_compat();

		//sprite_putimage(render_window_sprite->sprite_bitmapptr);
		//sprite_putimage(unused_preview_window->sprite_bitmapptr);

	//fatal_error("happy yet?");


	// shutdown
	shutdown_dos_game();

	fatal_error("err %i", input_flags);

	return 0;
}

legacy_s16 run_main_menu_loop(legacy_s16 argc, legacy_s8* argv[]) {

	legacy_s16 i, result;
	legacy_s16 unused_result, reload_track;
	legacy_s8 start_in_replay;
	legacy_s8 far* trkptr;
	legacy_s8 far* textresptr;

	init_full_game(argc, argv);

	//fatal_error("ai");
	init_main_input_state();
	set_default_car();

	reload_track = 1;

	while (1) {

		ensure_file_exists(2);

		if (reload_track != 0) {
			file_build_path(track_directory, gameconfig.game_trackname, ".trk", g_path_buf);
			file_read_fatal(g_path_buf, track_element_map);
		}

		idle_expired = 0;
		result = run_intro_looped();
		if (result == 27) {
			textresptr = locate_text_res(mainresptr, "dos");
			result = show_dialog(DIALOG_TYPE_MENU, DIALOG_SAVE_BACKGROUND,
				textresptr,
				DIALOG_AUTO_POSITION, DIALOG_AUTO_POSITION,
				dialog_border_color, 0, 0);
			if (result >= 1) {
				shutdown_dos_game();
				return result;
			}
			reload_track = 0;
			continue;
		}

		while (1) {
			ensure_file_exists(2);
			if (is_audioloaded == 0) {
				file_load_audiores("skidslct", "skidms", "SLCT");
			}
			result = run_menu();
			if (result == -1)  {
				audio_unload();
				reload_track = 0;
				break;
			} else if (result == 0) {
				start_in_replay = 0;
			} else if (result == 1) {
				check_input();
				show_waiting();
				run_car_menu(&gameconfig.game_playercarid[0],
					&gameconfig.game_playermaterial,
					&gameconfig.game_playertransmission, 0);
				continue;
			} else if (result == 2) {
				check_input();
				show_waiting();
				run_opponent_menu();
				continue;
			} else if (result == 3) {
				run_tracks_menu(0);
				continue;
			} else if (result == 4) {
				check_input();
				show_waiting();
				result = run_option_menu();
				if (result == 0) {
					continue;
				} else {
					// Enter replay mode if the option-menu result is nonzero.
					start_in_replay = 1;
				}
			} else {
				continue;
			}

			_memcpy(&gameconfigcopy, &gameconfig, sizeof(struct GAMEINFO));
			for (i = 0; i < REPLAY_TRACK_SIZE; i++) {
				track_and_directory_backup[i] = track_element_map[i];
			}
			for (i = 0; i < TRACK_PATH_STORAGE_SIZE; i++) {
				track_and_directory_backup[i + TRACK_PRIMARY_PATH_OFFSET] =
					track_directory[i];
				track_and_directory_backup[i + TRACK_SECONDARY_PATH_OFFSET] =
					replay_directory[i];
			}

			if (idle_expired == 0) {
				result = track_setup();
				//result = setup_track();
				if (result != 0) {
					run_tracks_menu(1);
					continue;
				}
				random_wait();
				if (passed_security == 0) {
					fatal_error("security check");
					//get_super_random();
					//security_check();
				}
			} else if (file_find("tedit.*") == 0) {
				audio_unload();
				reload_track = 0;
				break;
			}

			audio_unload();

			cvxptr = mmgr_alloc_resbytes("cvx",
				sizeof(struct GAMESTATE) * GAMESTATE_CHECKPOINT_COUNT);
			init_game_state(GAMESTATE_INIT_RESET_CHECKPOINTS);

			if (start_in_replay != 0) {
				replay_recording_flags = 0;
 			} else {

				gameconfig.game_recordedframes = 0;
			}

			while (1) {
				show_waiting();
				run_game();
				if (idle_expired == 0 && replay_recording_flags != 0) {
					result = end_hiscore();
					if (result == 0) {
						// view replay
						replay_recording_flags = REPLAY_RECORDING_RESTARTABLE_FLAG;
						continue;
					} else if (result == 1) {
						// drive
						gameconfig.game_recordedframes = 0;
						continue;
					}
				}
				// main menu
				break;
			}

			_memcpy(&gameconfigcopy, &gameconfig, sizeof(struct GAMEINFO));
			for (i = 0; i < REPLAY_TRACK_SIZE; i++) {
				track_element_map[i] = track_and_directory_backup[i];
			}
			for (i = 0; i < TRACK_PATH_STORAGE_SIZE; i++) {
				track_directory[i] =
					track_and_directory_backup[i + TRACK_PRIMARY_PATH_OFFSET];
				replay_directory[i] =
					track_and_directory_backup[i + TRACK_SECONDARY_PATH_OFFSET];
			}
			mmgr_release(cvxptr);

			if (idle_expired != 0) {
				reload_track = 0;
				break;
			}
		}

	}
}
