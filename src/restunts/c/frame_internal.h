#ifndef RESTUNTS_FRAME_INTERNAL_H
#define RESTUNTS_FRAME_INTERNAL_H

#include "externs.h"
#include "fileio.h"
#include "legacy.h"
#include "math.h"
#include "memmgr.h"
#include "shape2d.h"
#include "shape3d.h"

extern struct RECTANGLE* rectptr_unk2;
extern struct RECTANGLE rect_array_unk[];
extern struct RECTANGLE rect_array_unk2[];
extern struct RECTANGLE rect_array_unk3[];
extern legacy_s8 rect_array_unk_indices[];
extern legacy_s16 rect_array_unk3_indices[];
extern legacy_s8 rect_array_unk3_length;
extern struct RECTANGLE rect_unk[];
/* These legacy labels were views into consecutive elements of rect_unk. */
#define rect_unk2  rect_unk[1]
#define rect_unk6  rect_unk[2]
#define rect_unk12 rect_unk[3]
#define rect_unk15 rect_unk[4]
#define rect_skybox rect_unk[5]
#define rect_unk11 rect_unk[6]
#define rect_unk9  rect_unk[7]
extern struct RECTANGLE rect_unk3;
extern struct RECTANGLE rect_unk5;
extern struct RECTANGLE cliprect_unk;
extern struct RECTANGLE rect_ingame_text2;
extern struct RECTANGLE rect_ingame_text3;
extern struct RECTANGLE rect_ingame_text4;
extern struct VECTOR vec_unk2;
extern struct VECTOR vec_planerotopresult;
extern struct MATRIX mat_temp;
extern legacy_s16 camera_track_height_offset;
extern legacy_s8 detail_threshold_by_level[];
extern legacy_s8 byte_3C0C6[];
extern legacy_u16 frame_callback_count;
extern legacy_s16 word_3BE34[];
extern legacy_s8* lookahead_tiles_tables[];
extern struct SHAPE3D* off_3BE44[];
extern legacy_s16 terrainHeight;
extern legacy_s16 planindex;
extern legacy_s16 planindex_copy;
extern legacy_s8 byte_4392C;
extern struct TRANSFORMEDSHAPE3D currenttransshape[29];
//extern struct TRANSFORMEDSHAPE3D transshapeunk;
extern struct TRANSFORMEDSHAPE3D* curtransshape_ptr;
extern struct TRACKOBJECT trkObjectList[215]; // 215 entries
extern legacy_u8 fence_TrkObjCodes[];
extern legacy_s16 pState_minusRotate_z_2, pState_minusRotate_x_2, pState_minusRotate_y_2, pState_f36Mminf40sar2;

extern legacy_s8 unk_3C0EE[];
extern legacy_s8 unk_3C0F0[];
extern legacy_s8 unk_3C0F8[];
extern legacy_s8 unk_3C0F4[];
extern legacy_s16 word_3C0D6[];
extern legacy_s16 unk_3C0A2[];
extern legacy_s16 unk_3C0A6[];
extern legacy_s16 unk_3C0AE[];
extern legacy_s16 unk_3C0B6[];
extern struct TRACKOBJECT sceneshapes2[];
extern struct TRACKOBJECT sceneshapes3[];
extern struct SHAPE3D game3dshapes[130];
extern struct VECTOR carshapevec[2];
extern struct VECTOR carshapevecs[24];
extern legacy_s16 word_443E8[];
extern struct VECTOR oppcarshapevec[2];
extern struct VECTOR oppcarshapevecs[24];
extern legacy_s16 word_4448A[];
extern legacy_s8 backlights_paint_override;
extern legacy_s16 word_449FC[];
extern legacy_s16 word_463D6;
extern legacy_s16 transformedshape_zarray[];
extern legacy_s16 transformedshape_indices[];
extern legacy_s8 transformedshape_arg2array[];
extern legacy_s16 sdgame2_widths[];
extern void far* sdgame2shapes[];
extern void far* fontledresptr;
extern legacy_s16 dialog_fnt_colour;
extern legacy_s8 transformedshape_counter;
extern struct SPRITE far* render_window_sprite;
extern legacy_s16 fontdef_unk_0E;
extern legacy_u16 skybox_current;
extern legacy_u16 word_454CE;
extern legacy_u16 skybox_ptr1;
extern legacy_u16 skybox_ptr2;
extern legacy_u16 skybox_ptr3;
extern legacy_u16 skybox_ptr4;
extern legacy_s16 skybox_sky_color;
extern legacy_s16 skybox_grd_color;
extern legacy_s16 skybox_wat_color;
extern struct RECTANGLE rect_ingame_text;
extern struct RECTANGLE intro_cliprect;
extern struct SHAPE2D far* skyboxes[];
extern legacy_s16 penalty_time;
extern legacy_s16 intro_colorvalue;
extern legacy_s16 word_407CC;
extern struct SHAPE3D logoshape;
extern struct SHAPE3D logo2shape;
extern struct SHAPE3D bravshape;
extern legacy_s16 word_44DCC;
extern legacy_s16 word_3C108;
extern legacy_s16 word_3C10A;
extern legacy_s16 word_3C10C;
extern legacy_s16 word_3C10E;
extern legacy_s16 word_3C110;
extern legacy_s16 word_3C112;
extern struct VECTOR unk_3C114;
extern struct RECTANGLE trackpreview_cliprect;

void build_track_object(struct VECTOR* a, struct VECTOR* b);
struct TRACKOBJECT* frame_track_object_from_legacy_index(legacy_u8 index);
void transformed_shape_add_for_sort(legacy_s16 z_adjust, legacy_s16 type);
void skybox_op_helper2(struct RECTANGLE* rect, legacy_s16 angle, legacy_s16 horizon);
legacy_u8 subst_hillroad_track(legacy_u8 a, legacy_u8 b);
legacy_s16 skybox_op(legacy_s16 a, struct RECTANGLE* rectptr, legacy_s16 c, struct MATRIX* matptr, legacy_s16 e, legacy_s16 f, legacy_s16 g);
legacy_s16 font_op2_alt(const legacy_s8* text);
struct RECTANGLE* draw_ingame_text(void);
struct RECTANGLE* init_crak(legacy_s16 frame, legacy_s16 top, legacy_s16 height);
struct RECTANGLE* do_sinking(legacy_s16 frame, legacy_s16 top, legacy_s16 height);
struct RECTANGLE* intro_draw_text(legacy_s8* str, legacy_s16 a, legacy_s16 b, legacy_s16 c, legacy_s16 d);
void intro_op(legacy_s16 camera_x, legacy_s16 camera_y, legacy_s16 camera_z, legacy_s16 rotate_y,
	legacy_s16 rotate_x, legacy_s16 draw_car, legacy_s16 primary_logo, struct VECTOR* stars,
	struct POINT2D* previous_points, legacy_s16* previous_point_count,
	struct RECTANGLE previous_rect, struct RECTANGLE* shape_rect,
	struct RECTANGLE* combined_rect);
void init_plantrak(void);
void do_opponent_op(void);
legacy_u32 timer_get_delta(void);
legacy_s16 get_0(void);
void font_set_fontdef2(void far* data);
void set_fontdefseg(void far* data);
void format_frame_as_string(legacy_s8* s, legacy_s16 time, legacy_s16 c);
void heapsort_by_order(legacy_s16 n, legacy_s16* heap, legacy_s16* data);

#endif
