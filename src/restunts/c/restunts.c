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
#include "replay_record.h"
#include "replay_viewer.h"
#include "replay_viewer_internal.h"
#include "shape2d.h"
#include "shape3d.h"
#include "timing.h"
#include "ui_dialog.h"
#include "ui_input.h"
#include "ui_text.h"

// Entries in the CVX gamestate buffer.
#define RST_CVX_NUM 20


legacy_s16 get_0(void)
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

void do_opponent_op(void)
{
	opponent_op();
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

	for (i = 0; status1 == dos_video_get_status() && i < 12000; ++i);

	if (i == 1024) {
		i = aMisc_1[0];
	}

	while (i--) {
		rand();
		get_kevinrandom();
	}

	i &= 0xFF;

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
	gameconfig.game_playertransmission = 1;
	gameconfig.game_opponenttype       = 0;
	gameconfig.game_opponentmaterial   = 0;
	gameconfig.game_opponentcarid[0]   = 0xFF;
}


extern legacy_u16 select_cliprect_rotate(legacy_s16 angX, legacy_s16 angY, legacy_s16 angZ, struct RECTANGLE* cliprect, legacy_s16 unk);
//extern void transformed_shape_op(struct TRANSFORMSHAPE3D* shape);
extern void set_projection(legacy_s16, legacy_s16, legacy_s16, legacy_s16);

struct RECTANGLE shaperect = { 0, 320, 0, 200 };
struct TRANSFORMEDSHAPE3D transshape;
struct RECTANGLE cliprect = { 0, 0x140, 0, 0x5F };
struct VECTOR carpos = { 0, 0x0FCB8, 0x0B40 }; // from the original
//struct VECTOR carpos = { 0, 0, 320 };

struct SPRITE far* render_window_sprite;
//cliprect_unk    RECTANGLE <270Fh, 0FFFFh, 270Fh, 0FFFFh>

extern legacy_s16 polyinfonumpolys;
extern legacy_u8 far* polyinfoptrs[]; // array size = 0x190
extern legacy_u16 poly_linked_list_40ED6[]; // array size = 0x190


extern legacy_s16 font_op(const legacy_s8* text, legacy_s16 count);


void audio_sequence_timer(void);
extern void audio_map_song_instruments(void far* song,
	void far* instruments);
extern void audio_map_song_tracks(void far* song);
extern void sub_35B76(legacy_s16 x, legacy_s16 y, legacy_s16 width, legacy_s16 height, legacy_s16 color);
void audio_release_channel_range(legacy_s16 first_channel,
	legacy_s16 last_channel);
extern void audio_op_unk3(legacy_s16 channel);
extern void audio_op_unk4(legacy_s16 channel);
extern legacy_s16 audio_check_flag(void far* resource, legacy_s16 channel,
	legacy_u8 priority, legacy_u16 rate);
extern void audio_init_chunk(legacy_s16 first_channel, legacy_s16 last_channel,
	void far* resource, legacy_u16 resource_data_offset,
	legacy_u16 rate, legacy_u8 priority);

extern legacy_s8 aId1[];
extern legacy_s8 aId2[];
extern legacy_s8 aId3[];
extern legacy_s8 aId4[];
extern legacy_s8 aDea[];
extern legacy_s8 aDer[];
extern legacy_s8 aSav[];
extern legacy_s8 aWai[];
extern legacy_s8 aLoa[];
extern legacy_s8 aLsu[];
extern legacy_s8 aLsd[];
extern legacy_s8* findfilenames[];
extern struct TRACKOBJECT trkObjectList[];
extern struct SHAPE2D far* tracksmenushapes1[];
extern struct SHAPE2D far* tracksmenushape2dunk[];
extern struct SHAPE2D far* tracksmenushape2dunk2[];
legacy_s16 call_read_line(legacy_s8* text, legacy_s16 max_characters, legacy_s16 x, legacy_s16 y,
	legacy_u32 timeout);
legacy_s8 do_fileselect_dialog(legacy_s8* directory, legacy_s8* filename,
	legacy_s8* extension, legacy_s8 far* prompt);
legacy_s16 mouse_timer_sprite_unk(legacy_s16 item_index,
	const legacy_s16* x_values, const legacy_s16* width_values,
	const legacy_s16* y_values, const legacy_s16* height_values,
	legacy_s16 second_state, legacy_s16 first_state);
void do_mer_restext(void);
struct RECTANGLE* intro_draw_text(legacy_s8* text, legacy_s16 x, legacy_s16 y, legacy_s16 color,
	legacy_s16 shadow_color);
legacy_u8 subst_hillroad_track(legacy_u8 terrain, legacy_u8 track);

extern void sprite_1_unk4(legacy_s16 x, legacy_s16 y, legacy_s16 width, legacy_s16 height, legacy_s16 color);


extern legacy_s8 gnam_string[];
extern legacy_s8 gsna_string[];
extern legacy_s8 unk_46464[];
extern legacy_s8 byte_459E0[];

extern void far* engptr;
extern void far* eng1ptr;
extern void far* fontledresptr;
extern void far* sdgameresptr;
extern legacy_s8 unk_3E7FC[];
extern legacy_s8 unk_3E82C[];
extern legacy_s16 audio_init_engine(legacy_s16, void far*, void far*, void far*);

extern void update_car_speed(legacy_s8 input, legacy_s16 multiplayer,
	struct CARSTATE* carstate, struct SIMD* simd);

void init_div0(void)
{
	dos_install_divide_error_handler();
}

void copy_material_list_pointers(void* clrlist, void* clrlist2, void* patlist, void* patlist2, legacy_u16 videoConst)
{
	material_clrlist_ptr_cpy = clrlist;
	material_clrlist2_ptr_cpy = clrlist2;
	material_patlist_ptr_cpy = patlist;
	material_patlist2_ptr_cpy = patlist2;
	someZeroVideoConst = videoConst;
}

void init_main(legacy_s16 argc, legacy_s8* argv[])
{
	legacy_u16 i, j;
	legacy_u8 argmode4, argnosound, argnounknown;
	legacy_u32 timerdelta1, timerdelta2, timerdelta3;
	struct POINT2D tmppoint;
	struct RECTANGLE tmprect;

	// Keyboard
	kb_init_interrupt();
	dos_kb_clear_numlock();
	kb_call_readchar_callback();

	kb_reg_callback(0x0007, &show_graphic_levels_menu);
	kb_reg_callback(0x000A, &do_joy_restext);
	kb_reg_callback(0x000B, &do_key_restext);
	kb_reg_callback(0x3200, &do_mof_restext);
	kb_reg_callback(0x0010, &do_pau_restext);
	kb_reg_callback('p', &do_pau_restext);
	kb_reg_callback(0x0011, &do_dos_restext);
	kb_reg_callback(0x0013, &do_sonsof_restext);
	kb_reg_callback(0x0018, &do_dos_restext);

	// Video
	init_video_geometry_flags();

	mmgr_alloc_a000();
	himem_init();
	audio_allocate_car_state_records();

	video_flag5_is0 = 0;
	video_flag6_is1 = 1;

	textresprefix = 'e';

	// Parse arguments.
	argmode4 = 0;
	argnosound = 0;
	argnounknown = 0;

	for (i = 1; argc > i; ++i) {
		if (argv[i][0] == '/') {
			switch (argv[i][1]) {
				case 'h':
					argmode4 = 4;
					break;

				case 'n':
					if (argv[i][2] == 's') {
						argnosound = 1;
					}
					else if (argv[i][2] == 'd') {
						argnounknown = 1;
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
	(void)argnounknown;

	// Video mode.
	dos_video_set_mode_13h();
	if (argmode4) {
		dos_video_set_mode4();
	}

	dos_timer_setup_interrupt();

	sprite_copy_2_to_1_clear();

	dos_mouse_init(0x0140, 0x00C8);

	// Audio driver.
	if (audio_load_dos_driver(audiodriverstring, 0, 0)) {
		dos_timer_shutdown();
		dos_process_exit(1);
	}

	if (argnosound) {
		audio_toggle_flag2();
		audio_toggle_flag6();
	}

	dos_set_critical_error_handler(&do_dea_textres);

	load_palandcursor();

	// Timing measures.
	sprite_copy_2_to_1();
	sprite_set_1_size(0, 320, 0, 120);

	timer_get_delta_alt();
	for (i = 0; i < 15; ++i) {
		sprite_clear_1_color(0);
	}
	timerdelta1 = timer_get_delta_alt();

	sprite_set_1_size(0, 320, 0, 60);

	for (i = 0; i < 15; ++i) {
		tmprect.left = tmprect.right = tmprect.top = tmprect.bottom = 0;

		for (j = 0; j < 400; ++j) {
			tmppoint.px = tmppoint.py = j;
			rect_adjust_from_point(&tmppoint, &tmprect);
		}

		sprite_clear_1_color(0);
	}

	timerdelta2 = timer_get_delta_alt();

	for (i = 0; i < 146; ++i) {
		for (j = 0; j < 255; ++j) {
			rect_adjust_from_point(&tmppoint, &tmprect);
		}
	}

	timerdelta3 = timer_get_delta_alt();

	slow_video_mgmt = (timerdelta2 <= timerdelta1);
	framespersec2 = (timerdelta3 >= 75) ? 10 : 20;

	if (timerdelta3 < 35) {
		detail_level = 0;
	}
	else if (timerdelta3 < 55) {
		detail_level = 1;
	}
	else if (timerdelta3 < 75) {
		detail_level = 2;
	}
	else if (timerdelta3 < 100) {
		detail_level = 3;
	}
	else if (slow_video_mgmt) {
		detail_level = 4;
	}
	else {
		detail_level = 3;
	}

	framespersec = framespersec2;
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
	fontdefptr = file_load_resource(0, "fontdef.fnt");
	fontnptr = file_load_resource(0, "fontn.fnt");

	font_set_fontdef();
	init_polyinfo();
	init_trackdata();

	init_unknown();

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

legacy_s16 stuntsmain2(legacy_s16 argc, legacy_s8* argv[]) {
	legacy_s16 result;
	legacy_s8 far* textresptr;
	legacy_s16 carposangle;
	struct SPRITE far* var42wnd;
	legacy_s16 counter;
	legacy_s16 inch;
	legacy_s16 shapeindex;

	// initialization
	init_full_game(argc, argv);
	init_main_input_state();
	//set_default_car();

	// try do something
	sub_29772();
	set_projection(0x24, 0x11, 0x140, 0x64);	// would at best draw just a pixel without this - camera projection??

	render_window_sprite = sprite_make_wnd(320, 100, 0x0F);

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

	transshape.unk = 0;//0x7530;
	transshape.ts_flags = 0;
	transshape.rectptr = &shaperect;

	counter = 0;
	shapeindex = 24;
	for (; ; counter++) {

		transshape.rotvec.z = 0; //counter + 0x230;

		// seg000:1C58                 mov     [bp+var_transshape.ts_shapeptr], (offset game3dshapes.shape3d_numverts+0AA8h)
		// 0xAA8 / sizeof(SHAPE3D) = 0xAA8 / 0x16 = 124, points at where car0 is loaded during shape3d_load_car_shapes();

		transshape.shapeptr = &game3dshapes[shapeindex];

		//transshape.shapeptr = &game3dshapes[124];
		//transshape.shapeptr = &game3dshapes[124];

		transformed_shape_op(&transshape);

		sprite_copy_wnd_to_1();
		sprite_clear_1_color(3);

		//sprite_set_1_size(50, 200, 50, 100);
		get_a_poly_info(); // renders to sprite1

		//sprite_copy_2_to_1_2();
		sprite_blit_to_video(render_window_sprite, 0);

		inch = get_kb_or_joy_flags();//kb_get_char();
		if (inch == 4) { // right
			shapeindex++;
			shapeindex = (shapeindex + 0x74) % 0x74;
		} else
		if (inch == 8) { // left
			shapeindex--;
			shapeindex = (shapeindex + 0x74) % 0x74;
		} else
		if (inch != 0) {
			textresptr = locate_text_res(mainresptr, "dos");
			//result = show_dialog(2, 1, textresptr, 0xFFFF, 0xFFFF, dialogarg2, 0, 0); // center
			result = show_dialog(2, 1, textresptr, 0, 170, dialogarg2, 0, 0);
			if (result >= 1)
				break;
		}
	}

	//var42wnd = sprite_make_wnd(320, 200);
	//setup_mcgawnd2();
	//sprite_set_1_size(0, 320, 0, 200);
	//sprite_copy_2_to_1_2();
	//sprite_clear_1_color(2);
		//sprite_copy_wnd_to_1();
		//sprite_copy_2_to_1_2();

		//sprite_putimage(render_window_sprite->sprite_bitmapptr);
		//sprite_putimage(var42wnd->sprite_bitmapptr);

	//fatal_error("happy yet?");


	// shutdown
	shutdown_dos_game();

	fatal_error("err %i", inch);

	return 0;
}

legacy_s16 stuntsmainimpl(legacy_s16 argc, legacy_s8* argv[]) {

	legacy_s16 i, result;
	legacy_s16 regax, regsi;
	legacy_s8 var_A;
	legacy_s8 far* trkptr;
	legacy_s8 far* textresptr;

	init_full_game(argc, argv);

	//fatal_error("ai");
	init_main_input_state();
	set_default_car();

	regsi = 1;

	while (1) {

		ensure_file_exists(2);

		if (regsi != 0) {
			file_build_path(byte_3B80C, gameconfig.game_trackname, ".trk", g_path_buf);
			file_read_fatal(g_path_buf, td14_elem_map_main);
		}

		idle_expired = 0;
		result = run_intro_looped();
		if (result == 27) {
			textresptr = locate_text_res(mainresptr, "dos");
			result = show_dialog(2, 1, textresptr, 0xFFFF, 0xFFFF, dialogarg2, 0, 0);
			if (result >= 1) {
				shutdown_dos_game();
				return result;
			}
			regsi = 0;
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
				regsi = 0;
				break;
			} else if (result == 0) {
				var_A = 0;
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
					var_A = 1;
				}
			} else {
				continue;
			}

			_memcpy(&gameconfigcopy, &gameconfig, sizeof(struct GAMEINFO));
			for (i = 0; i < 0x70A; i++) {
				td20_trk_file_appnd[i] = td14_elem_map_main[i];
			}
			for (i = 0; i < 0x51; i++) {
				td20_trk_file_appnd[i + 0x70A] = byte_3B80C[i];
				td20_trk_file_appnd[i + 0x75B] = byte_3B85E[i];
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
				regsi = 0;
				break;
			}

			audio_unload();

			cvxptr = mmgr_alloc_resbytes("cvx", sizeof(struct GAMESTATE) * RST_CVX_NUM);
			init_game_state(-1);

			if (var_A != 0) {
				byte_43966 = 0;
 			} else {

				gameconfig.game_recordedframes = 0;
			}

			while (1) {
				show_waiting();
				run_game();
				if (idle_expired == 0 && byte_43966 != 0) {
					result = end_hiscore();
					if (result == 0) {
						// view replay
						byte_43966 = 4;
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
			for (i = 0; i < 0x70A; i++) {
				td14_elem_map_main[i] = td20_trk_file_appnd[i];
			}
			for (i = 0; i < 0x51; i++) {
				byte_3B80C[i] = td20_trk_file_appnd[i + 0x70A];
				byte_3B85E[i] = td20_trk_file_appnd[i + 0x75B];
			}
			mmgr_release(cvxptr);

			if (idle_expired != 0) {
				regsi = 0;
				break;
			}
		}

	}
}
