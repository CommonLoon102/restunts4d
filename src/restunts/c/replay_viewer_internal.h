#ifndef RESTUNTS_REPLAY_VIEWER_INTERNAL_H
#define RESTUNTS_REPLAY_VIEWER_INTERNAL_H

#include "legacy.h"

struct RECTANGLE;
struct SHAPE2D;

extern legacy_u8 byte_3E9DB;
extern legacy_u8 byte_3E9DC[10];
extern legacy_u8 byte_3E9E6[10];
extern legacy_u8 byte_3E9F0[10];
extern legacy_u8 byte_3E9FA[10];
extern legacy_u8 game_camera_buttons_count[4];
extern legacy_s16 game_camera_buttons_x1[9];
extern legacy_s16 game_camera_buttons_x2[9];
extern legacy_s16 game_camera_buttons_y1[9];
extern legacy_s16 game_camera_buttons_y2[9];
extern legacy_s16 word_3EA18;
extern legacy_s16 word_3EA2A;
extern legacy_s16 word_3EA3A;
extern legacy_s16 word_3EA3C;
extern legacy_s16 word_3EA4C;
extern legacy_s16 word_3EA4E;
extern legacy_s16 gameunk_button_x1;
extern legacy_s16 gameunk_button_x2;
extern legacy_s16 gameunk_button_y1;
extern legacy_s16 gameunk_button_y2;
extern legacy_s16 custom_camera_distance;
extern legacy_s16 custom_camera_azimuth_angle;
extern legacy_s16 custom_camera_elevation_angle;
extern legacy_s16 word_40E04[2];
extern legacy_u8 byte_40E08[2];
extern legacy_s16 word_40E0A[2];
extern struct SHAPE2D far* rplyshapes[23];
extern legacy_u8 byte_40E6A[9];
extern legacy_u8 byte_40E6C;
extern legacy_u8 byte_40E6D;
extern legacy_u8 byte_40E74[2];
extern legacy_s16 word_40E76[2];
extern legacy_u8 byte_40E7A[18];
extern struct RECTANGLE* rectptr_unk2;
extern legacy_s16 word_407FC;
extern legacy_s16 word_407FE;
extern legacy_s8 aMen_0[];
extern legacy_s8 aCon_0[];
extern legacy_s8 aRep_1[];
extern legacy_s8 a_rpl_2[];
extern legacy_s8 aFex_0[];
extern legacy_s8 aSer_0[];
extern legacy_s8 aMdo[];
extern legacy_s8 aRplyrpicrpacrpmcrptcbof6bof5b[];

#endif
