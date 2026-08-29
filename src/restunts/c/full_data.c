#include "externs.h"
#include "platform.h"
#include "shape2d.h"
#include "shape3d.h"

/* Renderer work areas formerly reserved as anonymous spans in dseg.asm. */
struct SHAPE3D game3dshapes[130];
struct TRANSFORMEDSHAPE3D currenttransshape[29];
struct TRANSFORMEDSHAPE3D* curtransshape_ptr;

legacy_s16 poly_linked_list_40ED6[401];
legacy_u8 far* polyinfoptrs[400];

struct RECTANGLE rect_unk[15];
struct RECTANGLE rect_unk3;
struct RECTANGLE rect_unk5 = { 0, 320, 0, 200 };
struct RECTANGLE rect_array_unk[15];
struct RECTANGLE rect_array_unk2[15];
struct RECTANGLE rect_array_unk3[45];
legacy_s8 rect_array_unk_indices[15];
legacy_s16 rect_array_unk3_indices[45];
legacy_s8 rect_array_unk3_length;
struct RECTANGLE* rectptr_unk;
struct RECTANGLE* rectptr_unk2;

/* Camera, menu, and renderer constants. */
legacy_s16 custom_camera_distance = 0x00D2;
legacy_s16 custom_camera_azimuth_angle = 0x01D0;
legacy_s16 custom_camera_elevation_angle = 0x0050;

legacy_s16 menu_buttons_x1[5] = { 105, 66, 5, 190, 255 };
legacy_s16 menu_buttons_x2[5] = { 208, 107, 67, 253, 312 };
legacy_s16 menu_buttons_y1[5] = { 119, 77, 114, 76, 116 };
legacy_s16 menu_buttons_y2[5] = { 197, 120, 170, 122, 166 };
legacy_s16 trackmenu_buttons_x1[3] = { 16, 112, 208 };
legacy_s16 trackmenu_buttons_x2[3] = { 112, 208, 304 };
legacy_s16 trackmenu_buttons_y1[3] = { 171, 171, 171 };
legacy_s16 trackmenu_buttons_y2[3] = { 197, 197, 197 };
legacy_s16 carmenu_buttons_x1[5] = { 107, 125, 143, 161, 179 };
legacy_s16 carmenu_buttons_x2[5] = { 124, 142, 160, 178, 196 };
legacy_s16 carmenu_buttons_y1[5] = { 229, 229, 229, 229, 229 };
legacy_s16 carmenu_buttons_y2[5] = { 316, 316, 316, 316, 316 };
legacy_s16 opponentmenu_buttons_x1[5] = { 20, 76, 132, 188, 244 };
legacy_s16 opponentmenu_buttons_x2[5] = { 76, 132, 188, 244, 300 };
legacy_s16 opponentmenu_buttons_y1[5] = { 177, 177, 177, 177, 177 };
legacy_s16 opponentmenu_buttons_y2[5] = { 197, 197, 197, 197, 197 };
legacy_s16 hiscore_buttons_y1[5] = { 174, 174, 174, 174, 174 };
legacy_s16 hiscore_buttons_y2[5] = { 197, 197, 197, 197, 197 };

struct RECTANGLE carmenu_cliprect = { 0, 320, 0, 95 };
struct RECTANGLE rect_unk16 = { 0, 320, 0, 0 };
struct VECTOR carmenu_carpos = { 0, -840, 2880 };
struct RECTANGLE cliprect_unk = { 9999, -1, 9999, -1 };
struct RECTANGLE trackpreview_cliprect = { 0, 320, 0, 200 };
struct RECTANGLE intro_cliprect = { 0, 320, 0, 200 };
struct RECTANGLE rect_ingame_text2 = { 148, 172, 93, 108 };
struct RECTANGLE rect_ingame_text3 = { 68, 92, 113, 128 };
struct RECTANGLE rect_ingame_text4 = { 228, 252, 113, 128 };

legacy_s8 detail_threshold_by_level[6] = { 2, 2, 1, 0, 0, 0 };
legacy_s16 word_3BE34[8] = { 30, 200, 320, 400, 530, 700, 880, 960 };
legacy_s16 unk_3C0A2[2] = { 0, 0 };
legacy_s16 unk_3C0A6[4] = { 0, 512, 0, -512 };
legacy_s16 unk_3C0AE[4] = { 512, 0, -512, 0 };
legacy_s16 unk_3C0B6[8] = {
	-512, 512, -512, -512, 512, 512, 512, -512
};
legacy_s8 byte_3C0C6[16] = {
	0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3
};
legacy_s16 word_3C0D6[8] = { 0, 0, 256, 256, 512, 512, 768, 768 };
legacy_u8 fence_TrkObjCodes[8] = {
	0xD6, 0xD7, 0xD6, 0xD7, 0xD6, 0xD7, 0xD6, 0xD7
};
legacy_s8 unk_3C0EE[2] = { 0, 0 };
legacy_s8 unk_3C0F0[4] = { 0, 0, 0, 1 };
legacy_s8 unk_3C0F4[4] = { 0, 0, 1, 0 };
legacy_s8 unk_3C0F8[16] = {
	0, 0, 1, 0, 0, 1, 1, 1, -128, 0, -128, 1, -1, -1, 0, 0
};
legacy_s16 word_3C108 = 0x3C00;
legacy_s16 word_3C10A = 0x4EE8;
legacy_s16 word_3C10C = (legacy_s16)0xF510;
legacy_s16 word_3C10E = 0x3C00;
legacy_s16 word_3C110 = 0x0AF0;
legacy_s16 word_3C112 = 0x2AD0;
struct VECTOR unk_3C114 = { 0, (legacy_s16)0xD88C, 0x4178 };

/* Static scene objects refer to portable C shape records, not dseg offsets. */
struct TRACKOBJECT sceneshapes2[19] = {
	{ 0, 0x0000, &game3dshapes[108], &game3dshapes[109], 0, 0, 0, 0, -1, 0 },
	{ 0, 0x0000, &game3dshapes[45], &game3dshapes[45], 0, 0, 1, 0, -1, 0 },
	{ 0, 0x0000, &game3dshapes[44], &game3dshapes[44], 0, 0, 1, 0, -1, 0 },
	{ 0, 0x0300, &game3dshapes[44], &game3dshapes[44], 0, 0, 1, 0, -1, 0 },
	{ 0, 0x0200, &game3dshapes[44], &game3dshapes[44], 0, 0, 1, 0, -1, 0 },
	{ 0, 0x0100, &game3dshapes[44], &game3dshapes[44], 0, 0, 1, 0, -1, 0 },
	{ 0, 0x0000, &game3dshapes[43], &game3dshapes[43], 0, 0, 1, 0, -1, 0 },
	{ 0, 0x0000, &game3dshapes[42], &game3dshapes[42], 0, 0, 1, 0, -1, 0 },
	{ 0, 0x0300, &game3dshapes[42], &game3dshapes[42], 0, 0, 1, 0, -1, 0 },
	{ 0, 0x0200, &game3dshapes[42], &game3dshapes[42], 0, 0, 1, 0, -1, 0 },
	{ 0, 0x0100, &game3dshapes[42], &game3dshapes[42], 0, 0, 1, 0, -1, 0 },
	{ 0, 0x0000, &game3dshapes[41], &game3dshapes[41], 0, 0, 1, 0, -1, 0 },
	{ 0, 0x0300, &game3dshapes[41], &game3dshapes[41], 0, 0, 1, 0, -1, 0 },
	{ 0, 0x0200, &game3dshapes[41], &game3dshapes[41], 0, 0, 1, 0, -1, 0 },
	{ 0, 0x0100, &game3dshapes[41], &game3dshapes[41], 0, 0, 1, 0, -1, 0 },
	{ 0, 0x0000, &game3dshapes[40], &game3dshapes[40], 0, 0, 1, 0, -1, 0 },
	{ 0, 0x0300, &game3dshapes[40], &game3dshapes[40], 0, 0, 1, 0, -1, 0 },
	{ 0, 0x0200, &game3dshapes[40], &game3dshapes[40], 0, 0, 1, 0, -1, 0 },
	{ 0, 0x0100, &game3dshapes[40], &game3dshapes[40], 0, 0, 1, 0, -1, 0 }
};

struct TRACKOBJECT sceneshapes3[13] = {
	{ 0, 0, &game3dshapes[112], &game3dshapes[112], 0, 0, 1, 0, -1, 0 },
	{ 0, 0, &game3dshapes[113], &game3dshapes[113], 0, 0, 1, 0, -1, 0 },
	{ 0, 0, &game3dshapes[114], &game3dshapes[114], 0, 0, 1, 0, -1, 0 },
	{ 0, 0, &game3dshapes[115], &game3dshapes[115], 0, 0, 1, 0, -1, 0 },
	{ 0, 0, &game3dshapes[116], &game3dshapes[116], 0, 0, 1, 0, -1, 0 },
	{ 0, 0, &game3dshapes[117], &game3dshapes[117], 0, 0, 1, 0, -1, 0 },
	{ 0, 0, &game3dshapes[118], &game3dshapes[118], 0, 0, 1, 0, -1, 0 },
	{ 0, 0, &game3dshapes[119], &game3dshapes[119], 0, 0, 1, 0, -1, 0 },
	{ 0, 0, &game3dshapes[120], &game3dshapes[120], 0, 0, 1, 0, -1, 0 },
	{ 0, 0, &game3dshapes[121], &game3dshapes[121], 0, 0, 1, 0, -1, 0 },
	{ 0, 0, &game3dshapes[122], &game3dshapes[122], 0, 0, 1, 0, -1, 0 },
	{ 0, 0, &game3dshapes[123], &game3dshapes[123], 0, 0, 1, 0, -1, 0 },
	{ 0, 0, &game3dshapes[110], &game3dshapes[110], 0, 0, 1, 0, -1, 0 }
};

struct SHAPE3D* off_3BE44[8] = {
	&game3dshapes[48], &game3dshapes[47], &game3dshapes[46],
	&game3dshapes[48], &game3dshapes[47], &game3dshapes[46],
	&game3dshapes[48], &game3dshapes[47]
};

/* Shape transformation and rasterizer state. */
legacy_u16 transshapenumverts;
legacy_u8 far* transshapeprimitives;
legacy_u16 transshapenumpaints;
legacy_u8 transshapeflags;
legacy_u8 transshapematerial;
struct RECTANGLE* transshaperectptr;
struct MATRIX mat_temp;
struct MATRIX mat_y0;
struct MATRIX mat_y100;
struct MATRIX mat_y200;
struct MATRIX mat_y300;
legacy_s32 sin80;
legacy_s32 cos80;
legacy_s32 sin80_2;
legacy_s32 cos80_2;
legacy_u16 projectiondata1;
legacy_u16 projectiondata2;
legacy_u16 projectiondata3 = 160;
legacy_u16 projectiondata4;
legacy_u16 projectiondata5 = 160;
legacy_u16 projectiondata6 = 100;
legacy_u16 projectiondata7;
legacy_u16 projectiondata8 = 100;
legacy_u16 projectiondata9;
legacy_u16 projectiondata10;

legacy_u16 select_rect_param;
struct RECTANGLE select_rect_rc;
legacy_u16 polyinfoptrnext;
legacy_u8 far* polyinfoptr;
legacy_u8 far* transshapepolyinfo;
legacy_u8 far* transshapeprimptr;
legacy_u8 far* transshapeprimindexptr;
legacy_s8 transprimitivepaintjob;
legacy_u16 polyinfonumpolys;
legacy_u16 poly_linklist_40ED6_iter1;
legacy_u16 poly_linklist_40ED6_iter2;
legacy_u16 poly_linklist_40ED6_iter3;
legacy_u16 poly_linklist_40ED6_iter4;
legacy_u16 word_40ECE;
struct POINT2D* polyvertpointptrtab[11];

legacy_u8 primidxcounttab[16] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 2, 6, 3, 0, 0
};
legacy_u8 primtypetab[16] = {
	0, 5, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 3, 4, 0, 0
};
legacy_s32 invpow2tbl[32] = {
	(legacy_s32)0x80000000UL, 0x40000000L, 0x20000000L, 0x10000000L,
	0x08000000L, 0x04000000L, 0x02000000L, 0x01000000L,
	0x00800000L, 0x00400000L, 0x00200000L, 0x00100000L,
	0x00080000L, 0x00040000L, 0x00020000L, 0x00010000L,
	0x00008000L, 0x00004000L, 0x00002000L, 0x00001000L,
	0x00000800L, 0x00000400L, 0x00000200L, 0x00000100L,
	0x00000080L, 0x00000040L, 0x00000020L, 0x00000010L,
	0x00000008L, 0x00000004L, 0x00000002L, 0x00000001L
};

legacy_s16 transformedshape_zarray[29];
legacy_s16 transformedshape_indices[29];
legacy_s8 transformedshape_arg2array[29];
legacy_s8 transformedshape_counter;
legacy_s8 transshapenumvertscopy;
legacy_s8 backlights_paint_override;
legacy_s16 word_443E8[5];
legacy_s16 word_4448A[5];
legacy_s16 word_449FC;
legacy_s16 word_449FE;
legacy_s16 word_44DCC;
legacy_s16 word_463D6;

struct VECTOR carshapevec[2];
struct VECTOR carshapevecs[24];
struct VECTOR oppcarshapevec[2];
struct VECTOR oppcarshapevecs[24];
legacy_s16 unk_3E710[4];

struct SHAPE3D logoshape;
struct SHAPE3D logo2shape;
struct SHAPE3D bravshape;

/* Video and sprite state formerly split between dseg and seg012. */
struct SPRITE far sprite1;
struct SPRITE far sprite2;
legacy_u16 far full_screen_line_offsets[200];
legacy_u8 far wnd_defs[0xE10];
legacy_s8* far next_wnd_def = (legacy_s8*)&wnd_defs[0];
struct SPRITE far* sprite_ptrs[4];
struct SPRITE far* mcgawndsprite;
legacy_u8 byte_3B8FC;
legacy_u16 fontdefseg;
legacy_u16 word_4031E;
legacy_u16 word_40320;
legacy_s16 word_4646A[4];
legacy_s16 word_46486[4];

legacy_s16 skybox_sky_color;
legacy_s16 skybox_grd_color;
legacy_s16 skybox_wat_color;
legacy_u16 skybox_ptr1;
legacy_u16 skybox_ptr2;
legacy_u16 skybox_ptr3;
legacy_u16 skybox_ptr4;
legacy_u16 skybox_current;
legacy_u16 word_454CE;
struct RECTANGLE rect_ingame_text;
legacy_s16 intro_colorvalue = 1;
legacy_s16 word_407CC = 16;
legacy_s16 dialog_fnt_colour = 15;
legacy_s16 fontdef_unk_0E;
legacy_s16 sdgame2_widths[5];
void far* sdgame2shapes[5];
struct SHAPE2D far* skyboxes[4];
void far* fontledresptr;

/* Replay summary and input state. */
legacy_s32 gState_travDist;
legacy_u16 gState_frame;
legacy_s16 gState_total_finish_time;
legacy_s16 gState_144;
legacy_s16 gState_pEndFrame;
legacy_s16 gState_oEndFrame;
legacy_s16 gState_penalty;
legacy_s16 gState_impactSpeed;
legacy_s16 gState_topSpeed;
legacy_s16 gState_jumpCount;
legacy_s8 byte_43966;
legacy_s16 mouse_butstate;
legacy_s16 mouse_xpos;
legacy_s16 mouse_ypos;

/* Resource ownership. */
void far* mainresptr;
void far* fontnptr;
void far* fontdefptr;
void far* songfileptr;
void far* voicefileptr;
void far* engptr;
void far* eng1ptr;
void far* sdgameresptr;
legacy_s8 far* sdgame2ptr;
legacy_s8 far* skybox_res_ofs;
legacy_s8 far* carresptr;
legacy_s8 far* car2resptr;
legacy_s8 far* game1ptr;
legacy_s8 far* game2ptr;
legacy_s8 far* curshapeptr;
legacy_s8 far* opp_res;
legacy_s8 far* oppresources[7];
legacy_s8 is_audioloaded;

void (far* exitlistfuncs[11])(void);
legacy_s16 waitflag;

/* Menu and overlay resources. */
struct SHAPE2D far* whlshapes[9];
struct SHAPE2D far* gnobshapes[6];
struct SHAPE2D far* digshapes[10];
struct SHAPE2D far* rplyshapes[23];
struct SHAPE2D far* tracksmenushapes1[19];
struct SHAPE2D far* tracksmenushapes2[4];
struct SHAPE2D far* tracksmenushapes3[4];
struct SHAPE2D far* tracksmenushape2dunk[186];
struct SHAPE2D far* tracksmenushape2dunk2[186];
struct SPRITE far* whlsprite1;
struct SPRITE far* whlsprite2;
struct SPRITE far* whlsprite3;
legacy_s8 far* stdaresptr;
legacy_s8 far* stdbresptr;
void far* miscptr;

legacy_s8 byte_3B8F2;
legacy_s8 byte_3B8F6;
legacy_u8 detail_level;
legacy_s8 byte_4393D;
legacy_s8 byte_449D8[2];
legacy_s8 byte_449E2;
legacy_s8 byte_454A4;
legacy_s8 byte_459E0[17];
legacy_s8 byte_46167;
legacy_s8 unk_463EA[74];
legacy_s8 dashb_toggle_copy;
legacy_s8 replaybar_toggle_copy;
legacy_s8 followOpponentFlag_copy;
legacy_s8 replaybar_enabled;
legacy_s8 passed_security;
legacy_s8 resID_byte1;
legacy_s16 dashbmp_y;
legacy_s16 dashbmp_y_copy;
legacy_s16 dastbmp_y;
legacy_s16 dastbmp_y2;
legacy_s16 dastseg;
void far* dasmshapeptr;
legacy_s16 roofbmpheight;
legacy_s16 roofbmpheight_copy;
legacy_s16 height_above_replaybar;
legacy_s16 meter_needle_color;
legacy_u16 framespersec2;
legacy_u16 slow_video_mgmt;
legacy_u16 slow_video_mgmt_copy;
legacy_u16 someZeroVideoConst;
legacy_s16 word_45D94;
struct RECTANGLE rect_windshield;

legacy_u8 byte_3E9DB = 6;
legacy_u8 byte_3E9DC[10] = { 1, 7, 3, 4, 5, 6, 7, 8, 8, 0 };
legacy_u8 byte_3E9E6[10] = { 0, 0, 2, 2, 3, 4, 5, 1, 7, 0 };
legacy_u8 byte_3E9F0[10] = { 2, 6, 2, 3, 4, 5, 6, 7, 8, 0 };
legacy_u8 byte_3E9FA[10] = { 0, 1, 0, 0, 1, 1, 1, 7, 8, 0 };
legacy_u8 game_camera_buttons_count[4] = { 6, 6, 8, 7 };
legacy_s16 game_camera_buttons_x1[9] = {
	272, 109, 274, 232, 190, 151, 108, 66, 10
};
legacy_s16 game_camera_buttons_x2[9] = {
	314, 151, 314, 274, 232, 190, 151, 91, 47
};
legacy_s16 game_camera_buttons_y1[9] = {
	176, 176, 156, 156, 156, 156, 156, 156, 156
};
legacy_s16 game_camera_buttons_y2[9] = {
	193, 193, 173, 173, 173, 173, 173, 193, 193
};
legacy_s16 word_3EA18 = 10;
legacy_s16 word_3EA2A = 47;
legacy_s16 word_3EA3A = 156;
legacy_s16 word_3EA3C = 156;
legacy_s16 word_3EA4C = 193;
legacy_s16 word_3EA4E = 193;
legacy_s16 gameunk_button_x1;
legacy_s16 gameunk_button_x2 = 104;
legacy_s16 gameunk_button_y1 = 151;
legacy_s16 gameunk_button_y2 = 200;

legacy_s16 word_3BCDE[3] = { 2, 0, 1 };
legacy_s16 word_3BCE4[4] = { 1, 0, 3, 2 };
legacy_s16 word_3BCEC[4] = { 4, 84, 164, 244 };
legacy_s16 word_3BCF6[5] = { 75, 155, 235, 315, 199 };

legacy_s16 word_407CE = 5;
legacy_s16 word_407D0 = 14;
legacy_s16 word_407D2 = 8;
legacy_s16 word_407D4 = 15;
legacy_s16 word_407D6 = 8;
legacy_s16 word_407D8 = 11;
legacy_s16 word_407DA = 3;
legacy_s16 word_407DC = 12;
legacy_s16 word_407DE = 4;
legacy_s16 word_407E0 = 9;
legacy_s16 word_407E2 = 1;
legacy_s16 word_407E4 = 10;
legacy_s16 word_407E6 = 2;
legacy_s16 word_407E8 = 13;
legacy_s16 word_407EA = 5;
legacy_s16 word_407F2 = 12;
legacy_s16 word_407F4 = 15;
legacy_s16 word_407F6 = 8;
legacy_s16 word_407F8 = 7;
legacy_s16 word_407FA = 9;
legacy_s16 word_407FC = 1;
legacy_s16 word_407FE = 4;
legacy_s16 performGraphColor = 1;
legacy_u16 dialogarg2 = 4;

legacy_s16 word_40D3A;
legacy_s16 word_40D3C;
legacy_s16 word_40D3E;
legacy_s16 word_40D40;
legacy_s16 end_hiscore_random;
legacy_s16 word_40D44;
legacy_s16 word_40D6C[2];
legacy_s16 word_40D70[2];
legacy_s16 word_40D74[2];
legacy_s16 word_40D78[2];
legacy_u8 byte_40DF0[2];
legacy_s16 word_40DF2[2];
legacy_s16 word_40DF6[2];
legacy_u8 byte_40DFA[2];
legacy_s16 word_40E00[2];
legacy_s16 word_40E04[2];
legacy_u8 byte_40E08[2];
legacy_s16 word_40E0A[2];
legacy_u8 byte_40E6A[9];
legacy_u8 byte_40E6C;
legacy_u8 byte_40E6D;
legacy_u8 byte_40E74[2];
legacy_s16 word_40E76[2];
legacy_u8 byte_40E7A[18];
legacy_s16 word_3EB90;
legacy_u16 word_3F1C2;
legacy_u16 word_3F1C4;
struct RECTANGLE word_42248;
struct RECTANGLE word_42250;

legacy_u8 palmap[16] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};

/* Current material maps; the copies are switched independently at runtime. */
legacy_s16 material_color_list[129] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
	108, 116, 15, 28, 29, 14, 28, 31, 14, 200, 198, 196, 112, 114,
	116, 194, 197, 200, 146, 37, 35, 181, 29, 31, 19, 3, 11, 27, 0,
	4, 4, 12, 156, 154, 152, 150, 42, 40, 38, 37, 27, 26, 25, 24,
	72, 70, 68, 66, 123, 121, 120, 117, 92, 90, 88, 87, 173, 171,
	169, 167, 20, 19, 18, 17, 77, 76, 74, 73, 45, 44, 42, 41, 159,
	175, 174, 172, 29, 28, 18, 90, 15, 7, 200, 219, 136, 99, 101,
	103, 104, 106, 17, 20, 60, 77, 46, 61, 45, 202, 190, 186, 183,
	180, 0, 28, 30, 16, 20, 68, 54, 39, 43, 12, 17
};
legacy_s16 material_pattern_list[129] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
	0
};
legacy_s16 material_pattern2_list[129] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, 0, -1, -1, -1, 30685, -8841, 30685, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, 30685, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 30685, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -13261, 13260, -18835, 18834,
	-18835, 18834, -18835, 18834, 0, 0
};
legacy_s16* material_clrlist_ptr = material_color_list;
legacy_s16* material_clrlist2_ptr = material_color_list;
legacy_s16* material_patlist_ptr = material_pattern_list;
legacy_s16* material_patlist2_ptr = material_pattern2_list;
legacy_s16* material_clrlist_ptr_cpy;
legacy_s16* material_clrlist2_ptr_cpy;
legacy_s16* material_patlist_ptr_cpy;
legacy_s16* material_patlist2_ptr_cpy;

legacy_s8* findfilenames[5] = {
	"id4", "setup.exe", "sdtitl.*", "tedit.*", "opp1.*"
};
legacy_s8 full_empty_extension[] = "";
legacy_s8* shapeexts[6] = {
	".PVS", ".XVS", ".VSH", ".PES", ".ESH", full_empty_extension
};

legacy_u8 far incnums[256] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
	16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
	32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
	48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
	64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
	80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
	96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109,
	110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122,
	123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135,
	136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148,
	149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161,
	162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174,
	175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187,
	188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200,
	201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213,
	214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226,
	227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
	240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252,
	253, 254, 255
};

void (*spritefunc)(legacy_s16*, legacy_s16*, legacy_u16, legacy_u16,
	legacy_u16);
void (*imagefunc)(legacy_u16, legacy_u16, legacy_u16, legacy_u16,
	legacy_u16);

#pragma pack (push, 1)
struct FULL_AUDIO_ENGINE_DEFINITION {
	legacy_u8 header[8];
	const legacy_s8 far* resource_ids[10];
};
#pragma pack (pop)

/* The resource-id fields are 16-bit far pointers in the DOS ABI. */
#if defined(__BORLANDC__)
typedef char full_audio_engine_definition_must_be_48_bytes[
	(sizeof(struct FULL_AUDIO_ENGINE_DEFINITION) == 48) ? 1 : -1];
#endif

struct FULL_AUDIO_ENGINE_DEFINITION unk_3E7FC = {
	{ 0xF4, 0x01, 0x10, 0x27, 0x28, 0x23, 0, 0 },
	{ "ENGI", "ENGI", "STAR", "STOP", "BLOW", "CRAS", "SKID",
		"SKI2", "BUMP", "SCRA" }
};
struct FULL_AUDIO_ENGINE_DEFINITION unk_3E82C = {
	{ 0xF4, 0x01, 0x10, 0x27, 0x28, 0x23, 0, 0 },
	{ "ENGI", "ENGI", "STAR", "STOP", "BLOW", "CRAS", "SKID",
		"SKI2", "BUMP", "SCRA" }
};

static void full_initialize_screen_sprite(struct SPRITE far* sprite)
{
	sprite->sprite_bitmapptr = (struct SHAPE2D far*)
		dos_memory_make_pointer(0xA000, 0);
	sprite->sprite_unk1 = 0;
	sprite->sprite_unk2 = 0;
	sprite->sprite_unk3 = 0;
	sprite->sprite_lineofs = (legacy_u8*)dos_memory_make_near_pointer(
		dos_memory_pointer_offset(full_screen_line_offsets));
	sprite->sprite_left = 0;
	sprite->sprite_right = 320;
	sprite->sprite_top = 0;
	sprite->sprite_height = 200;
	sprite->sprite_pitch = 320;
	sprite->sprite_unk4 = 0;
	sprite->sprite_width2 = 320;
	sprite->sprite_left2 = 0;
	sprite->sprite_widthsum = 320;
}

void full_data_initialize(void)
{
	legacy_u16 index;

	for (index = 0; index < 200U; index++)
		full_screen_line_offsets[index] = (legacy_u16)(index * 320U);
	for (index = 0; index < 0xE10U; index++)
		wnd_defs[index] = 0;
	next_wnd_def = (legacy_s8*)dos_memory_make_near_pointer(
		dos_memory_pointer_offset(wnd_defs));
	full_initialize_screen_sprite(&sprite1);
	full_initialize_screen_sprite(&sprite2);
}
