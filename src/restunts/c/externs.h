#ifndef RESTUNTS_EXTERNS_H
#define RESTUNTS_EXTERNS_H

#include "math.h"

#include "replay.h"
#include "gamestate.h"

#pragma pack (push, 1)

struct SIMD {
	legacy_s8 num_gears;
	legacy_s8 simd_unk;
	legacy_s16 car_mass;
	legacy_s16 braking_eff;
	legacy_s16 idle_rpm;
	legacy_s16 downshift_rpm;
	legacy_s16 upshift_rpm;
	legacy_s16 max_rpm;
	legacy_u16 gear_ratios[7];
	struct POINT2D knob_points[7];
	legacy_s16 aero_resistance;
	legacy_s8 idle_torque;
	legacy_s8 torque_curve[104];
	legacy_s8 field_A3;
	legacy_s16 grip;
	legacy_s16 field_A6[7];
	legacy_s16 sliding;
	legacy_s16 surface_grip[4];
	legacy_s8 simd_unk3[10];
	struct POINT2D collide_points[2];
	legacy_s16 car_height;
	struct VECTOR wheel_coords[4];
	legacy_s8 steeringdots[62];
	struct POINT2D spdcenter;
	legacy_s16 spdnumpoints;
	legacy_s8 spdpoints[208];
	struct POINT2D revcenter;
	legacy_s16 revnumpoints;
	legacy_s8 revpoints[256];
	legacy_s16 far* aerorestable;
};

struct TRKOBJINFO {
	legacy_s8  si_noOfBlocks;      // How many shapeInfo pieces compose the element. Arbitrary for the first piece, 0 for the following ones.
	legacy_s8  si_entryPoint;      // Connectivity of the track element regarding tiles.
	legacy_s8  si_exitPoint;
	legacy_s8  si_entryType;        // Connectivity of the track element regarding element types.
	legacy_s8  si_exitType;
	legacy_s8  si_arrowType;        // Type of the element for determining penalty-arrow behaviour.
	legacy_s16 si_arrowOrient;      // Orientation angle for penalty-arrow purposes
	struct VECTOR* si_cameraDataOffset; // offset (0003B770)
	legacy_s8  si_opp1;             //Appears to affect how the opponent AI approaches an element.
	legacy_s8  si_opp2;
	legacy_s8  si_opp3;
	legacy_s8  si_oppSpedCode;
};

struct TRACK_WALL {
	legacy_s16 orientation;
	legacy_s16 x;
	legacy_s16 z;
};

struct TRACKOBJECT {
	struct TRKOBJINFO* ss_trkObjInfoPtr; // offset (0003B770)
	legacy_s16 ss_rotY;           // Horizontal orientation of the element.
	struct SHAPE3D* ss_shapePtr;       // offset (0003B770)
	struct SHAPE3D* ss_loShapePtr;     // offset (0003B770)
	legacy_u8  ss_ssOvelay;       // Renders additional sceneShapes over the current one.
	legacy_s8  ss_surfaceType;    // Paintjob. FF will induce alternating paintjobs.
	legacy_s8  ss_ignoreZBias;    // Appears to be Z-bias override flag, mostly used for roads and corners.
	legacy_s8  ss_multiTileFlag;  // 0 = one-tile, 1 = two-tile vertical, 2 = two-tile horizontal, 3 = four-tile.
	legacy_s8  ss_physicalModel;  // sets the physical model in build_track_object
	legacy_s8  scene_unk5;        // always zero.
};

#pragma pack (pop)

typedef char legacy_track_wall_must_be_6_bytes[
	(sizeof(struct TRACK_WALL) == 6) ? 1 : -1];

/* These records contain 16-bit near/far pointers.  Their DOS ABI layout is
 * meaningful only when the Borland memory model supplies those pointer sizes. */
#if defined(__BORLANDC__)
typedef char legacy_simd_must_be_776_bytes[
	(sizeof(struct SIMD) == 776) ? 1 : -1];
typedef char legacy_trkobjinfo_must_be_14_bytes[
	(sizeof(struct TRKOBJINFO) == 14) ? 1 : -1];
typedef char legacy_trackobject_must_be_14_bytes[
	(sizeof(struct TRACKOBJECT) == 14) ? 1 : -1];
#endif

#define SIMD_RESOURCE_SIZE 772U
legacy_u16 simd_decode(struct SIMD* destination,
	const legacy_u8 far* source);

#define OPPONENT_SPEED_COUNT 16U
#define TRACK_PLAN_RESOURCE_COUNT 536U
#define TRACK_WALL_RESOURCE_COUNT 191U
void track_collision_resources_decode(const legacy_u8 far* plane_source,
	const legacy_u8 far* wall_source);

extern struct GAMEINFO gameconfig;
extern struct GAMEINFO gameconfigcopy;

extern struct GAMESTATE state;
extern struct SIMD simd_player;
extern struct SIMD simd_opponent;

void copy_string(legacy_s8* destination, legacy_s8 far* source);
extern legacy_s8 legacy_penalty_route_enabled;
void parse_penalty_route_mode(legacy_s16 argc, legacy_s8* argv[]);
void setup_legacy_penalty_route_word(void);
void setup_aero_trackdata(void far* carresptr, legacy_s16 is_opponent);
void load_opponent_data(void);

extern legacy_s16 video_flag1_is1;
extern legacy_s16 video_flag2_is1;
extern legacy_s16 video_flag3_isFFFF;
extern legacy_s16 video_flag4_is1;
extern legacy_s16 video_flag5_is0;
extern legacy_s16 video_flag6_is1;

extern legacy_u8 byte_44A8A;
extern legacy_u8 byte_4552F;
extern legacy_u16 elapsed_time1;
extern legacy_u16 elapsed_time2;
extern legacy_u8 byte_449DA;
extern legacy_u8 byte_4393C;
extern legacy_u8 game_replay_mode; // 0 = playing, 1 = paused, 2 = replay
extern legacy_s16 word_44DCA;

extern legacy_s16 word_45A24; // current frame?
extern legacy_s16 word_45A00; // fps * 30
extern legacy_s16 word_4499C; // 100 / fps
extern legacy_s16 track_angle;
extern legacy_s8* steerWhlRespTable_ptr;
extern legacy_s8 steerWhlRespTable_10fps[62];
extern legacy_s8 steerWhlRespTable_20fps[64];
extern legacy_s8 startcol2, startrow2;
extern legacy_s8 hillFlag;
extern legacy_s16 hillHeightConsts[];

extern struct RECTANGLE rect_windshield;
extern legacy_s16 word_449EA;
extern legacy_s16 run_game_random;
extern legacy_s8 replaybar_toggle;
extern legacy_s8 is_in_replay;
extern legacy_s8 cameramode;
extern legacy_s8 byte_449E6;
extern legacy_s8 game_replay_mode_copy;
extern legacy_s8 byte_44346;
extern legacy_s8 byte_46467;
extern legacy_s8 dashb_toggle;
extern legacy_s8 byte_4432A;
extern legacy_s8 show_penalty_counter;
extern legacy_s16 word_45D94;
extern legacy_s16 word_45D3E;
extern legacy_s8 byte_3B8F2;
extern void far* gameresptr;
extern void far* dasmshapeptr;
extern legacy_s8 dashb_toggle_copy;
extern legacy_s8 replaybar_toggle_copy;
extern legacy_s8 is_in_replay_copy;
extern legacy_s8 followOpponentFlag;
extern legacy_s8 followOpponentFlag_copy;
extern legacy_s16 roofbmpheight_copy;
extern legacy_s8 byte_449E2;
extern legacy_s8 replaybar_enabled;
extern legacy_s16 dashbmp_y_copy;
extern legacy_s16 height_above_replaybar;
extern legacy_s8 byte_454A4;
extern legacy_s8 byte_449D8[];
extern legacy_s16 dastseg;
extern legacy_s16 dastbmp_y;
extern legacy_s16 dastbmp_y2;
extern legacy_s16 dashbmp_y;
extern legacy_s16 roofbmpheight;
extern struct RECTANGLE* rectptr_unk;

extern void player_op(legacy_s8);
extern void opponent_op(void);
extern void state_op_unk(legacy_s16, legacy_s16, legacy_s16);
extern void sub_19BA0(void);
extern void init_kevinrandom(const legacy_s8* seed);
extern void get_kevinrandom_seed(legacy_s8* seed);
extern legacy_s16 get_kevinrandom(void);
extern void init_row_tables(void);
extern void init_trackdata(void);
extern void init_unknown(void);
extern void audio_carstate(void);
extern void setup_car_shapes(legacy_s16);
extern void update_frame(legacy_s8, struct RECTANGLE* rc);
extern void loop_game(legacy_s16, legacy_s16, legacy_s16);
extern void set_frame_callback(void);
extern void remove_frame_callback(void);
extern void mouse_minmax_position(legacy_s16);
extern legacy_s16 handle_ingame_kb_shortcuts(legacy_s16 key);
extern void update_crash_state(legacy_s16 state, legacy_s16 multiplayer);

extern legacy_s16 mouse_butstate;
extern legacy_s16 mouse_xpos;
extern legacy_s16 mouse_ypos;
extern legacy_s16 performGraphColor;
#define RESID_BUFFER_SIZE 80
#define RESID_TEXT_OFFSET 6
extern legacy_s8 resID_buffer[RESID_BUFFER_SIZE];
/* Legacy labels for byte 0 and byte 6 of the same scratch buffer. */
#define resID_byte1 resID_buffer[0]
#define unk_463EA (resID_buffer + RESID_TEXT_OFFSET)
extern legacy_s16 waitflag;

extern void far* fontnptr;
extern void far* fontdefptr;
extern void far* mainresptr;
extern struct GAMESTATE far* cvxptr;
extern legacy_s16 trackrows[];
extern legacy_s16 terrainrows[];
extern legacy_s16 trackpos[];
extern legacy_s16 trackcenterpos[];
extern legacy_s16 terrainpos[];
extern legacy_s16 terraincenterpos[];
extern legacy_s16 trackpos2[];
extern legacy_s16 trackcenterpos2[];
extern legacy_s16 far* td01_track_file_cpy; //trackdata1;
extern legacy_s16 far* td02_penalty_related; //trackdata2;
extern legacy_s8 far* trackdata3;
extern legacy_s16 far* td04_aerotable_pl; //trackdata4;
extern legacy_s16 far* td05_aerotable_op; //trackdata5;
extern legacy_s16 far* trackdata6;
extern legacy_s16 far* trackdata7;
extern legacy_s16 far* td08_direction_related; //trackdata8;
extern legacy_s16 far* trackdata9;
extern legacy_s16 far* td10_track_check_rel;// trackdata10;
extern legacy_s8 far* td11_highscores; //trackdata11;
extern legacy_s8 far* trackdata12;
extern legacy_s8 far* td13_rpl_header; //trackdata13;
extern legacy_u8 far* td14_elem_map_main; //trackdata14;
extern legacy_u8 far* td15_terr_map_main; //trackdata15;
extern legacy_s8 far* td16_rpl_buffer; //trackdata16;
extern legacy_s8 far* td17_trk_elem_ordered; //trackdata17;
extern legacy_s8 far* trackdata18;
extern legacy_u8 far* trackdata19;
extern legacy_s8 far* td20_trk_file_appnd; //trackdata20;
extern legacy_s8 far* td21_col_from_path; //trackdata21;
extern legacy_s8 far* td22_row_from_path; //trackdata22;
extern legacy_u8 far* trackdata23; // indexes into trkObjectList
extern legacy_s8 kbormouse;
extern legacy_s8 passed_security;
extern legacy_s8 g_is_busy;
extern legacy_s8 g_path_buf[];
extern legacy_s8 byte_3B80C[];
extern legacy_s8 idle_expired;
extern legacy_u16 dialogarg2;
extern legacy_s8 byte_3B85E[];
extern legacy_s8 byte_43966;
extern legacy_s8 aMain[];
extern legacy_s8 aMisc_1[];
extern legacy_s8 aFontdef_fnt[];
extern legacy_s8 aFontn_fnt[];
extern legacy_s8 aTrakdata[];
extern legacy_s8 aDefault_0[];
extern legacy_s8 aCvx[];
extern legacy_s8 aTedit__0[];
extern legacy_s8 aSlct[];
extern legacy_s8 aSkidms_0[];
extern legacy_s8 aSkidslct[];
extern legacy_s8 aDos[];

extern legacy_u16 framespersec;
extern legacy_u16 framespersec2;
extern legacy_u16 slow_video_mgmt;
extern legacy_u16 slow_video_mgmt_copy;
extern legacy_u8 detail_level;

extern legacy_u16 pspofs;
extern legacy_u16 pspseg;
extern legacy_u16 word_3FF82;
extern legacy_u16 word_3FF84;

extern struct MEMCHUNK* resptr1;
extern struct MEMCHUNK* resptr2;
extern struct MEMCHUNK* resendptr1;
extern struct MEMCHUNK* resendptr2;
extern legacy_u16 resmaxsize;

extern legacy_u16 word_3F1C2;
extern legacy_u16 word_3F1C4;
extern void (far* exitlistfuncs[])(void);
extern const legacy_s8 aExitListOverflow[];

extern const legacy_s8 aReservememoryO[];
extern const legacy_s8 aReservememoryOutOfMemory[];
extern const legacy_s8 aMemoryManagerB[];
extern const legacy_s8 aResizememoryNo[];
extern const legacy_s8 aResizememoryCa[];
extern const legacy_s8 aSFileError[];
extern const legacy_s8 aSFileError_0[];
extern const legacy_s8 aSFileError_1[];
extern const legacy_s8 aSInvalidPackTy[];
extern const legacy_s8 aLocateshape4_4sShapeNotF[];
extern const legacy_s8 aLocatesound4_4sSoundNotF[];
extern legacy_s8 audiodriverstring[];

extern legacy_u16 gState_frame;
extern legacy_s8 is_audioloaded;
extern void far* songfileptr;
extern void far* voicefileptr;
extern legacy_s8 textresprefix; // = 'e'
extern legacy_s8* shapeexts[];
extern legacy_u8 palmap[];

extern legacy_s16* material_clrlist_ptr;
extern legacy_s16* material_clrlist_ptr_cpy;
extern legacy_s16* material_clrlist2_ptr;
extern legacy_s16* material_clrlist2_ptr_cpy;
extern legacy_s16* material_patlist_ptr;
extern legacy_s16* material_patlist_ptr_cpy;
extern legacy_s16* material_patlist2_ptr;
extern legacy_s16* material_patlist2_ptr_cpy;
extern legacy_u16 someZeroVideoConst;

extern legacy_s16 sub_18D60(legacy_s16 car_trackdata3_index, struct VECTOR* car_vec_unk3, legacy_s16 field_CE, legacy_s8* optional_speed);
extern void init_carstate_from_simd(struct CARSTATE* carstate, struct SIMD* simd, legacy_s8 transmission, legacy_s32 posX, legacy_s32 posY, legacy_s32 posZ, legacy_s16 track_angle);
extern void init_game_state(legacy_s16 arg);
extern void restore_gamestate(legacy_u16 frame);
extern void update_gamestate(void);
extern void init_rect_arrays(void);
extern void sub_19F14(struct RECTANGLE* rect);
extern void font_set_fontdef(void);
extern void init_polyinfo(void);
extern legacy_s16 run_intro_looped(void);
extern legacy_s8 setup_intro(void);
extern legacy_s8 load_intro_resources(void);
extern legacy_u16 show_dialog(legacy_s16 unk1, legacy_s16 unk2, void far* textresptr, legacy_u16 unk3, legacy_u16 unk4, legacy_s16 arg, legacy_s16* disabled_choices, legacy_s16 unk6);
extern legacy_s8 run_menu(void);
extern legacy_s8 setup_track(void);
extern void run_tracks_menu(legacy_s16 unk);
extern legacy_s16 track_setup(void);
extern void run_opponent_menu(void);
extern void show_waiting(void);
extern void run_car_menu(legacy_s8* carid, legacy_s8* material, legacy_s8* transmission,
	legacy_u16 opponent_type);
extern void run_game(void);
extern legacy_u16 end_hiscore(void);
extern legacy_u16 run_option_menu(void);
extern void security_check(legacy_s16 question_index);

extern void ensure_file_exists(legacy_s16 unk);

extern void far* load_song_file(const legacy_s8* filename);
extern void far* load_voice_file(const legacy_s8* filename);
extern void far* load_sfx_file(const legacy_s8* filename);
extern void far* file_load_shape2d_nofatal(const legacy_s8* shapename);
extern void far* file_load_shape2d_res_nofatal(const legacy_s8* resname);
extern void far* file_load_shape2d_nofatal2(const legacy_s8* shapename);
extern void far* init_audio_resources(void far* songptr, void far* voiceptr, const legacy_s8* name);
extern void load_audio_finalize(void far* audiores);
extern legacy_s16 audio_load_driver(legacy_s8* driver, legacy_s16 a2, legacy_s16 a3);
extern void audio_unload(void);
extern legacy_s16 audio_toggle_flag2(void);
extern legacy_s16 audio_toggle_flag6(void);
extern void audio_stop_unk(void);
extern void audiodrv_atexit(void);
extern void audio_function2_wrap(legacy_s16 index);
extern void audio_add_driver_timer(void);
extern void audio_remove_driver_timer(void);

extern void check_input(void);
extern legacy_s16 input_do_checking(legacy_s16 unk);
extern void kb_exit_handler(void);
extern void kb_reg_callback(legacy_s16 code, void (far* callback)(void));
extern void show_graphic_levels_menu(void);
extern void do_joy_restext(void);
extern void do_key_restext(void);
extern void do_mou_restext(void);
extern void do_mof_restext(void);
extern void do_pau_restext(void);
extern void do_dos_restext(void);
extern void do_sonsof_restext(void);
extern legacy_s16 get_kb_or_joy_flags(void);

extern void mouse_draw_opaque(void);
extern void mouse_draw_transparent(void);
extern void mouse_draw_opaque_check(void);
extern void mouse_draw_transparent_check(void);

extern void video_set_mode4(void);
extern void video_set_mode7(void);
extern void video_set_mode_13h(void);

extern void shape3d_load_car_shapes(legacy_s8* carid, legacy_s8* oppcarid);

extern void load_palandcursor(void);
extern void sprite_set_1_size(legacy_u16 left, legacy_u16 right, legacy_u16 top, legacy_u16 height);
extern void sprite_clear_1_color(legacy_u8);
struct SPRITE;
extern legacy_s16 sprite_blit_to_video(struct SPRITE far* sprite, legacy_s16 mode);

extern void timer_setup_interrupt(void);
extern legacy_u32 timer_get_delta_alt(void);

extern void fatal_error(const legacy_s8*, ...);
extern legacy_s16 do_dea_textres(void);

extern void* _memcpy(void*, const void*, legacy_u16);
extern legacy_s8* _strcpy(legacy_s8* dest, const legacy_s8* src);
extern legacy_s8* _strcat(legacy_s8* dest, const legacy_s8* src);
extern legacy_s16 _strcmp(const legacy_s8* dest, const legacy_s8* src);
extern legacy_s16 _stricmp(const legacy_s8* dest, const legacy_s8* src);
extern legacy_u16 _strlen(const legacy_s8* str);
extern void far* __fmemcpy(void far*, const void far*, legacy_u16);
extern legacy_u16 _abs(legacy_u16);
extern legacy_s16 _rand(void);
extern void _srand(legacy_u16);

#define memcpy _memcpy
#define strcpy _strcpy
#define strcat _strcat
#define strlen _strlen
#define fmemcpy __fmemcpy
#define strcmp _strcmp
#define stricmp _stricmp
#define abs _abs
#define printf _printf
#define rand _rand
#define srand _srand

#endif
