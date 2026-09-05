#ifndef RESTUNTS_MENU_INTERNAL_H
#define RESTUNTS_MENU_INTERNAL_H

#include <stddef.h>
#include "externs.h"
#include "game_input.h"
#include "keyboard.h"
#include "shape2d.h"
#include "shape3d.h"
#include "timing.h"
#include "ui_dialog.h"
#include "ui_input.h"
#include "ui_text.h"

extern legacy_s8 aSdmsel[];
extern legacy_s8 aScrn[];
extern legacy_s16 menu_buttons_x1[];
extern legacy_s16 menu_buttons_x2[];
extern legacy_s16 menu_buttons_y1[];
extern legacy_s16 menu_buttons_y2[];
extern legacy_s16 word_407CE;
extern legacy_s16 word_407D0;
extern legacy_s16 word_407F4;
extern legacy_s16 word_407F6;
extern legacy_s16 word_407F8;
extern legacy_s16 trackmenu_buttons_x1[];
extern legacy_s16 trackmenu_buttons_x2[];
extern legacy_s16 trackmenu_buttons_y1[];
extern legacy_s16 trackmenu_buttons_y2[];
extern legacy_s8 aMisc[];
extern legacy_s8 aSdosel[];
extern legacy_s8 aOpp0opp1opp2op[];
extern legacy_s8 aScrn_0[];
extern legacy_s8 aBla[];
extern legacy_s8 aBnx[];
extern legacy_s8 aBcl[];
extern legacy_s8 aBca[];
extern legacy_s8 aBdo[];
extern legacy_s8 aClip[];
extern legacy_s8 aDes_0[];
extern legacy_s8 aRac[];
extern legacy_s8 aOpp1[];
extern legacy_s16 opponentmenu_buttons_x1[];
extern legacy_s16 opponentmenu_buttons_x2[];
extern legacy_s16 opponentmenu_buttons_y1[];
extern legacy_s16 opponentmenu_buttons_y2[];
extern legacy_s8 far* opp_res;
extern legacy_s8 far* oppresources[7];
extern legacy_s8 a_res_0[];
extern legacy_s8 aCar[];
extern legacy_s8 aSdcsel[];
extern legacy_s8 aMisc_0[];
extern legacy_s8 aGrap[];
extern legacy_s8 a150[];
extern legacy_s8 a100[];
extern legacy_s8 a50[];
extern legacy_s8 a0[];
extern legacy_s8 a02040[];
extern legacy_s8 aBdo_0[];
extern legacy_s8 aBnx_0[];
extern legacy_s8 aBla_0[];
extern legacy_s8 aBau[];
extern legacy_s8 aBma[];
extern legacy_s8 aBco[];
extern legacy_s8 aDes_1[];
extern legacy_s8 aStop_1[];
extern legacy_s8 aBau_0[];
extern legacy_s8 aBma_0[];
extern legacy_s16 carmenu_buttons_y1[];
extern legacy_s16 carmenu_buttons_y2[];
extern legacy_s16 carmenu_buttons_x1[];
extern legacy_s16 carmenu_buttons_x2[];
extern struct RECTANGLE carmenu_cliprect;
extern struct RECTANGLE rect_unk16;
extern struct VECTOR carmenu_carpos;
extern legacy_s8 backlights_paint_override;
extern legacy_s8 aMisc_2[];
extern legacy_s8 aElt[];
extern legacy_s8 aCon[];
extern legacy_s8 aPpt[];
extern legacy_s8 aDnf[];
extern legacy_s8 aOlt[];
extern legacy_s8 aDnf_0[];
extern legacy_s8 aOwt[];
extern legacy_s8 aOlt_0[];
extern legacy_s8 aVict[];
extern legacy_s8 aSkidms_1[];
extern legacy_s8 aSkidvict[];
extern legacy_s8 aOver[];
extern legacy_s8 aSkidms_2[];
extern legacy_s8 aSkidover[];
extern legacy_s8 aAvs[];
extern legacy_s8 aMph[];
extern legacy_s8 aImp[];
extern legacy_s8 aMph_0[];
extern legacy_s8 aTop[];
extern legacy_s8 aMph_1[];
extern legacy_s8 aJum[];
extern legacy_s8 aWinn[];
extern legacy_s8 aLose[];
extern legacy_s8 a_trk_5[];
extern legacy_s8 aIhd[];
extern legacy_s8 aD4a[];
extern legacy_s8 aBct[];
extern legacy_s8 aInh[];
extern legacy_s8 aInh_0[];
extern legacy_s8 aHna[];
extern legacy_s8 aBev[];
extern legacy_s8 aBhi[];
extern legacy_s8 aBrp[];
extern legacy_s8 aBra[];
extern legacy_s8 aBdr[];
extern legacy_s8 aBmm_0[];
extern legacy_s8 aOpp2win[];
extern legacy_s8 aOpp2lose[];
extern legacy_s8 aOp01[];
extern legacy_s16 word_3BCDE[];
extern legacy_s16 word_3BCE4[];
extern legacy_s16 word_3BCEC[5];
extern legacy_s16 word_3BCF6[5];
extern legacy_s16 hiscore_buttons_y1[5];
extern legacy_s16 hiscore_buttons_y2[5];
extern legacy_s16 word_40D3A;
extern legacy_s16 word_40D3C;
extern legacy_s16 word_40D3E;
#define HIGHSCORE_ENTRY_COUNT 7U

#pragma pack (push, 1)

/* One line of a track's .HIG table. The file is the raw array, so the
   layout is fixed by the on-disk format. The two name areas each hold a
   pair of strings laid end to end. */
struct HIGHSCORE_ENTRY {
	legacy_s8 player_name[17];
	legacy_s8 car_name[24];
	legacy_u8 car_flag;
	legacy_s8 opponent[8];
	legacy_u16 time;
};

#pragma pack (pop)

typedef char highscore_entry_must_be_52_bytes[
	(sizeof(struct HIGHSCORE_ENTRY) == 0x34) ? 1 : -1];
typedef char highscore_entry_car_name_must_be_at_11[
	(offsetof(struct HIGHSCORE_ENTRY, car_name) == 0x11) ? 1 : -1];
typedef char highscore_entry_car_flag_must_be_at_29[
	(offsetof(struct HIGHSCORE_ENTRY, car_flag) == 0x29) ? 1 : -1];
typedef char highscore_entry_opponent_must_be_at_2A[
	(offsetof(struct HIGHSCORE_ENTRY, opponent) == 0x2A) ? 1 : -1];
typedef char highscore_entry_time_must_be_at_32[
	(offsetof(struct HIGHSCORE_ENTRY, time) == 0x32) ? 1 : -1];

extern legacy_s16 word_40D40;
extern legacy_s16 end_hiscore_random;
extern legacy_s16 word_40D44;
extern legacy_s16 word_407D2;
extern legacy_s32 gState_travDist;
extern legacy_s16 gState_total_finish_time;
extern legacy_s16 gState_144;
extern legacy_s16 gState_pEndFrame;
extern legacy_s16 gState_oEndFrame;
extern legacy_s16 gState_penalty;
extern legacy_s16 gState_impactSpeed;
extern legacy_s16 gState_topSpeed;
extern legacy_s16 gState_jumpCount;
extern legacy_s8 aCred[];
extern legacy_s8 aArowarrwarw1ar[];
extern legacy_s8 aCre[];
extern legacy_s8 aGds0[];
extern legacy_s8 aGds1[];
extern legacy_s8 aDes[];
extern legacy_s8 aGdon[];
extern legacy_s8 aGkev[];
extern legacy_s8 aGbra[];
extern legacy_s8 aGrob[];
extern legacy_s8 aGsta[];
extern legacy_s8 aMus[];
extern legacy_s8 aGmsy[];
extern legacy_s8 aGkri[];
extern legacy_s8 aGbri[];
extern legacy_s8 aPro[];
extern legacy_s8 aGkev_0[];
extern legacy_s8 aOpr[];
extern legacy_s8 aGbra_0[];
extern legacy_s8 aGric[];
extern legacy_s8 aArt[];
extern legacy_s8 aGmsm[];
extern legacy_s8 aGdav[];
extern legacy_s8 aGnic[];
extern legacy_s8 aGkev_1[];
extern legacy_s16 word_407D4;
extern legacy_s16 word_407D6;
extern legacy_s16 word_407D8;
extern legacy_s16 word_407DA;
extern legacy_s16 word_407DC;
extern legacy_s16 word_407DE;
extern legacy_s16 word_407E0;
extern legacy_s16 word_407E2;
extern legacy_s16 word_407E4;
extern legacy_s16 word_407E6;
extern legacy_s16 word_407E8;
extern legacy_s16 word_407EA;

void load_skybox(legacy_s8 skybox_index);
void unload_skybox(void);
void draw_track_preview(void);
void load_tracks_menu_shapes(void);
void draw_button(legacy_s8 far* text, legacy_s16 x, legacy_s16 y, legacy_s16 width, legacy_s16 height,
	legacy_s16 top_color, legacy_s16 bottom_color, legacy_s16 fill_color, legacy_s16 font_color);
legacy_s16 highscore_write_a(legacy_s16 create_default);
extern struct SHAPE2D far* tracksmenushapes2[];
extern struct SHAPE2D far* tracksmenushapes3[];
extern legacy_s16 word_407F2;
extern legacy_s16 track_pieces_counter;
extern legacy_u8 byte_45D90;
extern legacy_u8 byte_45E16;

extern struct RECTANGLE shaperect;
extern struct TRANSFORMEDSHAPE3D transshape;
extern struct RECTANGLE cliprect;
extern struct VECTOR carpos;
extern struct SPRITE far* render_window_sprite;
extern legacy_s16 menu_idle_counter;
extern legacy_s16 word_3EB90;
extern legacy_s16 fontdef_unk_0E;
extern struct RECTANGLE word_42248;
extern struct RECTANGLE word_42250;
extern struct RECTANGLE cliprect_unk;
extern struct SHAPE3D game3dshapes[];
extern void far* miscptr;
extern legacy_s16 dialog_fnt_colour;
extern legacy_s16 word_407FA;
extern legacy_s16 ranking_entry_order[7];
extern legacy_s8 aCarcoun[];
extern legacy_s8 aDefault_1[];
extern legacy_s8 aDos_0[];
extern legacy_s8 aKey[];
extern legacy_s8 aMer[];
extern legacy_s8 aMof[];
extern legacy_s8 aMon[];
extern legacy_s8 aMrl[];
extern legacy_s8 aMrs[];
extern legacy_s8 aMou[];
extern legacy_s8 aPau[];
extern legacy_s8 aSof[];
extern legacy_s8 aSon[];

void sub_29772(void);
void menu_update_idle_counter(legacy_u16 elapsed, legacy_s16 limit);
legacy_s16 mouse_timer_sprite_unk(legacy_s16 item_index,
	const legacy_s16* x_values, const legacy_s16* width_values,
	const legacy_s16* y_values, const legacy_s16* height_values,
	legacy_s16 second_state, legacy_s16 first_state);
void draw_button(legacy_s8 far* text, legacy_s16 x, legacy_s16 y,
	legacy_s16 width, legacy_s16 height, legacy_s16 top_color,
	legacy_s16 bottom_color, legacy_s16 fill_color, legacy_s16 font_color);
void font_draw_text(const legacy_s8* text, legacy_s16 x, legacy_s16 y);
void font_set_fontdef2(void far* data);
void audio_suspend(void);
void audio_resume(void);
void call_exitlist2(void);
void print_highscore_entry(legacy_s16 entry, legacy_u8* text_offsets);
void update_car_speed(legacy_s8 input, legacy_s16 multiplayer,
	struct CARSTATE* carstate, struct SIMD* simd);

#endif
