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

enum MENU_BLIT_MODE {
	MENU_BLIT_MODE_REFRESH = 254,
	MENU_BLIT_MODE_INITIAL = 255
};

extern legacy_s8 main_menu_shapes_name[];
extern legacy_s8 main_menu_background_data[];
extern struct BUTTON_AREA menu_buttons[5];
extern legacy_s16 menu_highlight_second_color;
extern legacy_s16 menu_highlight_first_color;
extern legacy_s16 button_top_color;
extern legacy_s16 button_bottom_color;
extern legacy_s16 button_fill_color;
extern struct BUTTON_AREA trackmenu_buttons[3];
extern legacy_s8 opponent_misc_resource_name[];
extern legacy_s8 opponent_menu_shapes_name[];
extern legacy_s8 opponent_portrait_shape_ids[];
extern legacy_s8 opponent_menu_background_id[];
extern legacy_s8 opponent_previous_button_id[];
extern legacy_s8 opponent_next_button_id[];
extern legacy_s8 opponent_none_button_id[];
extern legacy_s8 opponent_car_button_id[];
extern legacy_s8 opponent_done_button_id[];
extern legacy_s8 opponent_portrait_clip_id[];
extern legacy_s8 opponent_description_id[];
extern legacy_s8 opponent_racing_car_label_id[];
extern legacy_s8 opponent_resource_name[];
extern struct BUTTON_AREA opponentmenu_buttons[5];
extern legacy_s8 far* opp_res;
extern legacy_s8 far* oppresources[7];
extern legacy_s8 car_resource_extension[];
extern legacy_s8 car_resource_wildcard[];
extern legacy_s8 car_menu_shapes_name[];
extern legacy_s8 car_misc_resource_name[];
extern legacy_s8 car_graph_shape_id[];
extern legacy_s8 car_graph_one_fifty_label[];
extern legacy_s8 car_graph_hundred_label[];
extern legacy_s8 car_graph_fifty_label[];
extern legacy_s8 car_graph_zero_label[];
extern legacy_s8 car_graph_time_labels[];
extern legacy_s8 car_done_button_id[];
extern legacy_s8 car_next_button_id[];
extern legacy_s8 car_previous_button_id[];
extern legacy_s8 car_automatic_button_id[];
extern legacy_s8 car_manual_button_id[];
extern legacy_s8 car_color_button_id[];
extern legacy_s8 car_description_id[];
extern legacy_s8 car_preview_top_shape_id[];
extern legacy_s8 car_automatic_toggle_id[];
extern legacy_s8 car_manual_toggle_id[];
extern struct BUTTON_AREA carmenu_buttons[5];
extern struct RECTANGLE carmenu_cliprect;
extern struct RECTANGLE car_menu_redraw_cliprect;
extern struct VECTOR carmenu_carpos;
extern legacy_s8 backlights_paint_override;
extern legacy_s8 results_misc_resource_name[];
extern legacy_s8 elapsed_time_label_id[];
extern legacy_s8 continued_race_label_id[];
extern legacy_s8 penalty_time_label_id[];
extern legacy_s8 player_did_not_finish_label_id[];
extern legacy_s8 opponent_unfinished_time_label_id[];
extern legacy_s8 opponent_did_not_finish_label_id[];
extern legacy_s8 opponent_win_time_label_id[];
extern legacy_s8 opponent_loss_time_label_id[];
extern legacy_s8 victory_song_id[];
extern legacy_s8 victory_instrument_resource_name[];
extern legacy_s8 victory_music_resource_name[];
extern legacy_s8 race_end_song_id[];
extern legacy_s8 race_end_instrument_resource_name[];
extern legacy_s8 race_end_music_resource_name[];
extern legacy_s8 average_speed_label_id[];
extern legacy_s8 average_speed_units_id[];
extern legacy_s8 impact_speed_label_id[];
extern legacy_s8 impact_speed_units_id[];
extern legacy_s8 top_speed_label_id[];
extern legacy_s8 top_speed_units_id[];
extern legacy_s8 jump_count_label_id[];
extern legacy_s8 opponent_win_text_id[];
extern legacy_s8 opponent_loss_text_id[];
extern legacy_s8 track_file_extension[];
extern legacy_s8 highscore_track_disk_prompt_id[];
extern legacy_s8 opponent_neutral_result_text_id[];
extern legacy_s8 result_continue_button_id[];
extern legacy_s8 opponent_highscore_prompt_id[];
extern legacy_s8 solo_highscore_prompt_id[];
extern legacy_s8 highscore_unavailable_message_id[];
extern legacy_s8 result_evaluation_button_id[];
extern legacy_s8 result_highscore_button_id[];
extern legacy_s8 result_replay_button_id[];
extern legacy_s8 result_race_button_id[];
extern legacy_s8 result_drive_button_id[];
extern legacy_s8 result_main_menu_button_data[];
extern legacy_s8 opponent_win_animation_name[];
extern legacy_s8 opponent_loss_animation_name[];
extern legacy_s8 opponent_animation_frame_id[];
extern legacy_s16 end_text_alternate_variant[];
extern legacy_s16 end_outcome_alternate_variant[];
extern legacy_s16 result_button_left[5];
extern legacy_s16 result_button_right[5];
extern legacy_s16 hiscore_buttons_y1[5];
extern legacy_s16 hiscore_buttons_y2[5];
extern legacy_s16 previous_end_opening_variant;
extern legacy_s16 previous_end_outcome_variant;
extern legacy_s16 previous_end_closing_variant;
#define HIGHSCORE_ENTRY_COUNT 7U
#define HIGHSCORE_LAST_ENTRY_INDEX (HIGHSCORE_ENTRY_COUNT - 1U)
#define HIGHSCORE_PLAYER_NAME_BYTES 17U
#define HIGHSCORE_CAR_NAME_BYTES 24U
#define HIGHSCORE_OPPONENT_BYTES 8U
#define HIGHSCORE_COMBINED_NAME_TEXT_BYTES \
	(HIGHSCORE_PLAYER_NAME_BYTES + HIGHSCORE_CAR_NAME_BYTES - 1U)
#define HIGHSCORE_OPPONENT_TEXT_BYTES (HIGHSCORE_OPPONENT_BYTES - 1U)
#define HIGHSCORE_ENTRY_SIZE_BYTES 52U
#define HIGHSCORE_CAR_NAME_OFFSET 17U
#define HIGHSCORE_CAR_FLAG_OFFSET 41U
#define HIGHSCORE_OPPONENT_OFFSET 42U
#define HIGHSCORE_TIME_OFFSET 50U
#define HIGHSCORE_TABLE_SIZE_BYTES \
	(HIGHSCORE_ENTRY_COUNT * HIGHSCORE_ENTRY_SIZE_BYTES)

#pragma pack (push, 1)

/* One line of a track's .HIG table. The file is the raw array, so the
   layout is fixed by the on-disk format. The two name areas each hold a
   pair of strings laid end to end. */
struct HIGHSCORE_ENTRY {
	legacy_s8 player_name[HIGHSCORE_PLAYER_NAME_BYTES];
	legacy_s8 car_name[HIGHSCORE_CAR_NAME_BYTES];
	legacy_u8 car_flag;
	legacy_s8 opponent[HIGHSCORE_OPPONENT_BYTES];
	legacy_u16 time;
};

#pragma pack (pop)

typedef char highscore_entry_must_have_expected_size[
	(sizeof(struct HIGHSCORE_ENTRY) == HIGHSCORE_ENTRY_SIZE_BYTES) ? 1 : -1];
typedef char highscore_entry_car_name_must_have_expected_offset[
	(offsetof(struct HIGHSCORE_ENTRY, car_name) ==
		HIGHSCORE_CAR_NAME_OFFSET) ? 1 : -1];
typedef char highscore_entry_car_flag_must_have_expected_offset[
	(offsetof(struct HIGHSCORE_ENTRY, car_flag) ==
		HIGHSCORE_CAR_FLAG_OFFSET) ? 1 : -1];
typedef char highscore_entry_opponent_must_have_expected_offset[
	(offsetof(struct HIGHSCORE_ENTRY, opponent) ==
		HIGHSCORE_OPPONENT_OFFSET) ? 1 : -1];
typedef char highscore_entry_time_must_have_expected_offset[
	(offsetof(struct HIGHSCORE_ENTRY, time) ==
		HIGHSCORE_TIME_OFFSET) ? 1 : -1];

extern legacy_s16 end_opening_variant;
extern legacy_s16 end_outcome_variant;
extern legacy_s16 end_closing_variant;
extern legacy_s16 end_animation_border_shadow_color;
extern legacy_s32 gState_travDist;
extern legacy_s16 gState_total_finish_time;
extern legacy_s16 gState_opponent_finish_time;
extern legacy_s16 gState_pEndFrame;
extern legacy_s16 gState_oEndFrame;
extern legacy_s16 gState_penalty;
extern legacy_s16 gState_impactSpeed;
extern legacy_s16 gState_topSpeed;
extern legacy_s16 gState_jumpCount;
extern legacy_s8 credits_resource_name[];
extern legacy_s8 credits_shape_ids[];
extern legacy_s8 credits_title_id[];
extern legacy_s8 credits_first_logo_shape_id[];
extern legacy_s8 credits_second_logo_shape_id[];
extern legacy_s8 credits_design_heading_id[];
extern legacy_s8 credits_first_designer_shape_id[];
extern legacy_s8 credits_second_designer_shape_id[];
extern legacy_s8 credits_third_designer_shape_id[];
extern legacy_s8 credits_fourth_designer_shape_id[];
extern legacy_s8 credits_fifth_designer_shape_id[];
extern legacy_s8 credits_music_heading_id[];
extern legacy_s8 credits_first_musician_shape_id[];
extern legacy_s8 credits_second_musician_shape_id[];
extern legacy_s8 credits_third_musician_shape_id[];
extern legacy_s8 credits_production_heading_id[];
extern legacy_s8 credits_producer_shape_id[];
extern legacy_s8 credits_opponent_heading_id[];
extern legacy_s8 credits_first_opponent_shape_id[];
extern legacy_s8 credits_second_opponent_shape_id[];
extern legacy_s8 credits_art_heading_id[];
extern legacy_s8 credits_first_artist_shape_id[];
extern legacy_s8 credits_second_artist_shape_id[];
extern legacy_s8 credits_third_artist_shape_id[];
extern legacy_s8 credits_fourth_artist_shape_id[];
extern legacy_s16 credits_text_color;
extern legacy_s16 credits_text_shadow_color;
extern legacy_s16 credits_title_color;
extern legacy_s16 credits_title_shadow_color;
extern legacy_s16 credits_design_heading_color;
extern legacy_s16 credits_design_heading_shadow_color;
extern legacy_s16 credits_production_heading_color;
extern legacy_s16 credits_production_heading_shadow_color;
extern legacy_s16 credits_art_heading_color;
extern legacy_s16 credits_art_heading_shadow_color;
extern legacy_s16 credits_music_heading_color;
extern legacy_s16 credits_music_heading_shadow_color;

void load_skybox(legacy_s8 skybox_index);
void unload_skybox(void);
void draw_track_preview(void);
void load_tracks_menu_shapes(void);
void draw_button(legacy_s8 far* text, legacy_s16 x, legacy_s16 y, legacy_s16 width, legacy_s16 height,
	legacy_s16 top_color, legacy_s16 bottom_color, legacy_s16 fill_color, legacy_s16 font_color);
legacy_s16 highscore_load_or_create(legacy_s16 create_default);
extern struct SHAPE2D far* track_editor_cursor_shapes[];
extern struct SHAPE2D far* track_editor_under_cursor_shapes[];
extern legacy_s16 track_editor_highlight_color;
extern legacy_s16 track_pieces_counter;
extern legacy_u8 track_validation_column;
extern legacy_u8 track_validation_row;

extern struct RECTANGLE shaperect;
extern struct TRANSFORMEDSHAPE3D transshape;
extern struct RECTANGLE cliprect;
extern struct VECTOR carpos;
extern struct SPRITE far* render_window_sprite;
extern legacy_s16 menu_idle_counter;
extern legacy_s16 dialog_background_color;
extern legacy_s16 font_glyph_height;
extern struct RECTANGLE intro_text_bounds;
extern struct RECTANGLE highscore_text_bounds;
extern struct RECTANGLE empty_rect;
extern struct SHAPE3D game3dshapes[];
extern void far* miscptr;
extern legacy_s16 dialog_fnt_colour;
extern legacy_s16 graphics_menu_background_color;
extern legacy_s16 ranking_entry_order[HIGHSCORE_ENTRY_COUNT];
extern legacy_s8 car_resource_name[];
extern legacy_s8 replay_filename_input[];
extern legacy_s8 exit_to_dos_dialog_id[];
extern legacy_s8 keyboard_driving_dialog_id[];
extern legacy_s8 insufficient_memory_dialog_id[];
extern legacy_s8 music_disabled_message_id[];
extern legacy_s8 music_enabled_message_id[];
extern legacy_s8 graphics_options_dialog_id[];
extern legacy_s8 frame_rate_changed_message_id[];
extern legacy_s8 mouse_driving_dialog_id[];
extern legacy_s8 pause_dialog_id[];
extern legacy_s8 effects_disabled_message_id[];
extern legacy_s8 effects_enabled_message_id[];

void menu_reset_animation_timers(void);
void menu_update_idle_counter(legacy_u16 elapsed, legacy_s16 limit);
legacy_s16 menu_animate_button_highlight(legacy_s16 item_index,
	const struct BUTTON_AREA* buttons,
	legacy_s16 second_color, legacy_s16 first_color);
void draw_button(legacy_s8 far* text, legacy_s16 x, legacy_s16 y,
	legacy_s16 width, legacy_s16 height, legacy_s16 top_color,
	legacy_s16 bottom_color, legacy_s16 fill_color, legacy_s16 font_color);
void font_draw_text(const legacy_s8* text, legacy_s16 x, legacy_s16 y);
void font_set_fontdef2(void far* data);
void audio_suspend(void);
void audio_resume(void);
void call_exitlist2(void);
void print_highscore_entry(legacy_s16 entry, legacy_u8* text_offsets);
void update_car_speed(legacy_s8 input, legacy_s16 car_index,
	struct CARSTATE* carstate, struct SIMD* simd);

#endif
