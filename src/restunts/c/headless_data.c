#include "externs.h"

#if defined(RESTUNTS_HEADLESS) || defined(RESTUNTS_FULL)

/* Mutable engine state formerly allocated by dseg.asm. */
struct GAMEINFO gameconfig;
struct GAMEINFO gameconfigcopy;
struct GAMESTATE state;
struct SIMD simd_player;
struct SIMD simd_opponent;

legacy_s16 video_flag1_is1;
legacy_s16 video_flag2_is1;
legacy_s16 video_flag3_isFFFF;
legacy_s16 video_flag4_is1;
legacy_s16 video_flag5_is0;
legacy_s16 video_flag6_is1;

legacy_u8 byte_44A8A;
legacy_u8 byte_4552F;
legacy_u16 elapsed_time1;
legacy_u16 elapsed_time2;
legacy_u8 byte_449DA;
legacy_u8 byte_4393C;
legacy_u8 game_replay_mode;
legacy_s16 word_44DCA;
legacy_s16 word_45A00;
legacy_s16 word_4499C;
legacy_s16 track_angle;
legacy_s8 startcol2;
legacy_s8 startrow2;
legacy_s8 hillFlag;
legacy_s16 hillHeightConsts[2] = { 0, 450 };

legacy_s16 word_449EA;
legacy_s16 run_game_random;
legacy_s8 replaybar_toggle;
legacy_s8 is_in_replay;
legacy_s8 cameramode;
legacy_s8 byte_449E6;
legacy_s8 game_replay_mode_copy;
legacy_s8 byte_44346;
legacy_s8 byte_46467;
legacy_s8 dashb_toggle;
legacy_s8 byte_4432A;
legacy_s8 show_penalty_counter;
legacy_s16 word_45D3E;
legacy_s8 is_in_replay_copy;
legacy_s8 followOpponentFlag;
legacy_s8 idle_expired;
legacy_s8 kbormouse;
legacy_s8 g_is_busy;
legacy_u16 framespersec;

void far* gameresptr;
struct PLANE far* planptr;
struct PLANE far* current_planptr;
struct TRACK_WALL far* wallptr;
struct GAMESTATE far* cvxptr;

legacy_s16 trackrows[30];
legacy_s16 terrainrows[30];
legacy_s16 trackpos[30];
legacy_s16 trackcenterpos[30];
legacy_s16 terrainpos[30];
legacy_s16 terraincenterpos[30];
legacy_s16 trackpos2[30];
legacy_s16 trackcenterpos2[30];

legacy_s16 far* td01_track_file_cpy;
legacy_s16 far* td02_penalty_related;
legacy_s8 far* trackdata3;
legacy_s16 far* td04_aerotable_pl;
legacy_s16 far* td05_aerotable_op;
legacy_s16 far* trackdata6;
legacy_s16 far* trackdata7;
legacy_s16 far* td08_direction_related;
legacy_s16 far* trackdata9;
legacy_s16 far* td10_track_check_rel;
legacy_s8 far* td11_highscores;
legacy_s8 far* trackdata12;
legacy_s8 far* td13_rpl_header;
legacy_u8 far* td14_elem_map_main;
legacy_u8 far* td15_terr_map_main;
legacy_s8 far* td16_rpl_buffer;
legacy_s8 far* td17_trk_elem_ordered;
legacy_s8 far* trackdata18;
legacy_u8 far* trackdata19;
legacy_s8 far* td20_trk_file_appnd;
legacy_s8 far* td21_col_from_path;
legacy_s8 far* td22_row_from_path;
legacy_u8 far* trackdata23;

legacy_s8 g_path_buf[94];
legacy_s8 byte_3B80C[0x51];
legacy_s8 byte_3B85E[0x51];

legacy_s8 aCarcoun[] = "carcoun";
legacy_s8 aOpp1[] = "opp1";
legacy_s8 aNam[] = "nam";
legacy_s8 aPath[] = "path";
legacy_s8 aSped[] = "sped";
legacy_s8 gnam_string[32];
legacy_s8 gsna_string[32];
legacy_s8 unk_46464[3];

legacy_s8 textresprefix = 'e';
const legacy_s8 aLocateshape4_4sShapeNotF[] =
	"locateshape - %-4.4s SHAPE NOT FOUND\r\n";
const legacy_s8 aLocatesound4_4sSoundNotF[] =
	"locatesound - %-4.4s SOUND NOT FOUND\r\n";

/* Lookup tables whose byte-for-byte values affect replay simulation. */
legacy_u8 atantable[257] = {
	0, 1, 1, 2, 3, 3, 4, 4, 5, 6, 6, 7, 8, 8, 9, 10,
	10, 11, 11, 12, 13, 13, 14, 15, 15, 16, 16, 17, 18, 18, 19, 20,
	20, 21, 22, 22, 23, 23, 24, 25, 25, 26, 27, 27, 28, 28, 29, 30,
	30, 31, 31, 32, 33, 33, 34, 34, 35, 36, 36, 37, 38, 38, 39, 39,
	40, 41, 41, 42, 42, 43, 44, 44, 45, 45, 46, 46, 47, 48, 48, 49,
	49, 50, 51, 51, 52, 52, 53, 53, 54, 55, 55, 56, 56, 57, 57, 58,
	58, 59, 60, 60, 61, 61, 62, 62, 63, 63, 64, 65, 65, 66, 66, 67,
	67, 68, 68, 69, 69, 70, 70, 71, 71, 72, 72, 73, 74, 74, 75, 75,
	76, 76, 77, 77, 78, 78, 79, 79, 80, 80, 81, 81, 82, 82, 83, 83,
	84, 84, 84, 85, 85, 86, 86, 87, 87, 88, 88, 89, 89, 90, 90, 91,
	91, 91, 92, 92, 93, 93, 94, 94, 95, 95, 96, 96, 96, 97, 97, 98,
	98, 99, 99, 99, 100, 100, 101, 101, 102, 102, 102, 103, 103, 104,
	104, 104, 105, 105, 106, 106, 106, 107, 107, 108, 108, 108, 109,
	109, 110, 110, 110, 111, 111, 112, 112, 112, 113, 113, 113, 114,
	114, 115, 115, 115, 116, 116, 116, 117, 117, 118, 118, 118, 119,
	119, 119, 120, 120, 120, 121, 121, 121, 122, 122, 122, 123, 123,
	123, 124, 124, 124, 125, 125, 125, 126, 126, 126, 127, 127, 127,
	128, 128
};

legacy_s8 steerWhlRespTable_20fps[64] = {
	0, 8, -8, 0, 0, 7, -7, 0, 0, 6, -6, 0, 0, 5, -5, 0,
	0, 4, -4, 0, 0, 4, -4, 0, 0, 3, -3, 0, 0, 3, -3, 0,
	0, 2, -2, 0, 0, 2, -2, 0, 0, 2, -2, 0, 0, 1, -1, 0,
	0, 1, -1, 0, 0, 1, -1, 0, 0, 1, -1, 0, 0, 1, -1, 0
};

legacy_s8 steerWhlRespTable_10fps[62] = {
	0, 16, -16, 0, 0, 14, -14, 0, 0, 12, -12, 0, 0, 10, -10, 0,
	0, 8, -8, 0, 0, 8, -8, 0, 0, 6, -6, 0, 0, 6, -6, 0,
	0, 4, -4, 0, 0, 4, -4, 0, 0, 4, -4, 0, 0, 2, -2, 0,
	0, 2, -2, 0, 0, 1, -1, 0, 0, 1, -1, 0, 0, 1
};

legacy_s8* steerWhlRespTable_ptr;
legacy_s16 grassDecelDivTab[5] = { 255, 256, 192, 128, 64 };

legacy_u8 byte_3E71E[6] = { 0, 0, 1, 0, 1, 0 };
legacy_u8 byte_3E724[6] = { 0, 1, 0, 0, 1, 0 };
legacy_u8 terrConnDataEtoW[20] = {
	0, 0, 0, 0, 0, 0, 1, 2, 1, 3, 0, 2, 3, 0, 0, 1, 1, 3, 2, 0
};
legacy_u8 terrConnDataWtoE[20] = {
	0, 0, 0, 0, 0, 0, 1, 2, 0, 3, 1, 0, 0, 3, 2, 2, 3, 1, 1, 0
};
legacy_u8 terrConnDataNtoS[20] = {
	0, 0, 0, 0, 0, 0, 1, 1, 5, 0, 4, 5, 0, 0, 4, 1, 5, 4, 1, 0
};
legacy_u8 terrConnDataStoN[19] = {
	0, 0, 0, 0, 0, 0, 1, 0, 5, 1, 4, 0, 5, 4, 0, 5, 1, 1, 4
};

struct POINT2D unk_3BD5A[2] = { { 5, 40 }, { 5, 10 } };
struct POINT2D unk_3BD62[2] = { { 6, 121 }, { 6, 9 } };
struct POINT2D unk_3BD6A[2] = { { 1, 10 }, { 1, 10 } };
legacy_s16 word_3BD72[4] = { 21, 21, 15, 15 };

struct VECTOR unk_3E640[1] = { { 0, 0, 0 } };
struct VECTOR unk_3E646[8] = {
	{ -120, 0, -281 }, { -120, 0, -231 },
	{ -120, 0, 281 }, { -120, 0, 231 },
	{ 120, 0, -281 }, { 120, 0, -231 },
	{ 120, 0, 281 }, { 120, 0, 231 }
};
struct VECTOR unk_3E676[2] = { { -60, 0, -512 }, { 60, 0, 512 } };
struct VECTOR unk_3E682[2] = { { -392, 0, 0 }, { -632, 0, 0 } };
struct VECTOR unk_3E68E[2] = { { 392, 0, 0 }, { 632, 0, 0 } };
struct VECTOR unk_3E69A[4] = {
	{ 23, 0, -255 }, { 97, 0, -255 },
	{ -97, 0, 255 }, { -23, 0, 255 }
};

/* Shared simulation scratch data. */
struct MATRIX mat_unk;
struct MATRIX mat_unk2;
struct MATRIX mat_planetmp;
struct VECTOR vec_unk2;
struct VECTOR vec_planerotopresult;
legacy_s32 pState_lvec1_x;
legacy_s32 pState_lvec1_y;
legacy_s32 pState_lvec1_z;
legacy_s16 pState_minusRotate_z_1;
legacy_s16 pState_minusRotate_z_2;
legacy_s16 pState_minusRotate_y_1;
legacy_s16 pState_minusRotate_y_2;
legacy_s16 pState_minusRotate_x_1;
legacy_s16 pState_minusRotate_x_2;
legacy_s16 planindex;
legacy_s16 planindex_copy;
legacy_s16 pState_f36Mminf40sar2;
legacy_s16 word_3BE16 = 9999;
legacy_s16 f36f40_whlData = 9999;
legacy_s16 elem_xCenter;
legacy_s16 elem_zCenter;
legacy_s16 terrainHeight;
legacy_s8 current_surf_type;
legacy_s16 nextPosAndNormalIP;
legacy_s16 wallindex;
legacy_s16 elRdWallRelated;
legacy_s16 wallHeight;
legacy_s16 wallStartX;
legacy_s16 wallStartZ;
legacy_s16 wallOrientation;
legacy_s8 byte_4392C;
legacy_s8 corkFlag;
legacy_s16 penalty_time;
legacy_s16 track_pieces_counter;
legacy_u8 byte_45635;
legacy_u8 byte_45D90;
legacy_u8 byte_45E16;
legacy_u8 byte_4616E;
legacy_u8 oppnentSped[10];
legacy_u8 byte_4032A;
legacy_u8 byte_4032B;
legacy_u16 word_3BE30;
legacy_u16 word_3BE32;

/* Track collision bounds.  Adjacent legacy labels intentionally exposed
 * overlapping windows; the C arrays make each window explicit. */
legacy_s16 loopSurface_ZBounds0[6] = { 0, 224, 389, 449, 389, 224 };
legacy_s16 loopSurface_ZBounds1[6] = { 224, 389, 449, 389, 224, 0 };
legacy_s16 loopSurface_maxZ = 449;
legacy_s16 loopSurface_XBounds0[6] = { -400, -400, -352, -304, -270, -235 };
legacy_s16 loopSurface_XBounds1[6] = { -400, -352, -304, -270, -235, -200 };
legacy_s16 loopBase_ZBounds0[6] = { 0, 178, 360, 536, 704, 868 };
legacy_s16 loopBase_ZBounds1[6] = { 178, 360, 536, 704, 868, 2000 };
legacy_s16 loopBae_InnXBounds0[6] = { 0, -20, -40, -60, -80, -100 };
legacy_s16 loopBase_InnXBounds1[6] = { -20, -40, -60, -80, -100, -120 };
legacy_s16 loopBase_OutXBounds0[6] = { 400, 361, 320, 276, 226, 174 };
legacy_s16 loopBase_OutXBounds1[6] = { 361, 320, 276, 226, 174, 120 };
legacy_s16 bkRdEntr_triang_zAdjust[4] = { -251, -84, 84, 251 };
legacy_s16 corkLR_negZBound[12] = {
	0, -94, -187, -280, -373, -466, -559, -652, -745, -838, -931, -1024
};
legacy_s16 corkLR_posZBound[12] = {
	0, 1024, 931, 838, 745, 652, 559, 466, 373, 280, 187, 94
};
legacy_s16 highEntrZBounds0[6] = { -512, -334, -168, 0, 168, 334 };
legacy_s16 highEntrZBounds1[6] = { -334, -168, 0, 168, 334, 1000 };
legacy_s16 highEntrXInnBounds0[6] = { 0, 0, 0, 0, 0, 120 };
legacy_s16 highEntrXInnBounds1[6] = { 0, 0, 0, 0, 120, 120 };
legacy_s16 highEntrXOutBounds0[6] = { 120, 168, 216, 264, 312, 360 };
legacy_s16 highEntrXOutBounds1[6] = { 168, 216, 264, 312, 360, 360 };

#endif
