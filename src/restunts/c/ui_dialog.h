#ifndef RESTUNTS_UI_DIALOG_H
#define RESTUNTS_UI_DIALOG_H

#include "legacy.h"

#define DIALOG_AUTO_POSITION 65535U
#define DIALOG_FAILURE_RESULT 65535U

enum DIALOG_BACKGROUND_POLICY {
	DIALOG_NO_BACKGROUND_SAVE = 0,
	DIALOG_SAVE_BACKGROUND = 1
};

enum DIALOG_TYPE {
	DIALOG_TYPE_MESSAGE = 0,
	DIALOG_TYPE_ACKNOWLEDGEMENT = 1,
	DIALOG_TYPE_MENU = 2,
	DIALOG_TYPE_PLACEHOLDERS = 3,
	DIALOG_TYPE_DELAY = 4
};

legacy_u16 show_dialog(legacy_s16 dialog_type, legacy_s16 save_background,
	void far* text_resource, legacy_u16 x_argument, legacy_u16 y_argument,
	legacy_s16 border_color, legacy_s16* disabled_choices,
	legacy_s16 initial_choice);
legacy_s8 do_fileselect_dialog(legacy_s8* directory, legacy_s8* filename,
	legacy_s8* extension, legacy_s8 far* prompt);
legacy_s16 do_savefile_dialog(legacy_s8* primary, legacy_s8* secondary,
	legacy_s8 far* prompt);
void ensure_file_exists(legacy_s16 unused);
void show_waiting(void);
legacy_s16 show_disk_error_dialog(void);
void security_check(legacy_s16 question_index);
void sprite_xor_rect_outline(legacy_s16 left, legacy_s16 top, legacy_s16 right,
	legacy_s16 bottom, legacy_s16 color);

void show_insufficient_memory_dialog(void);

#endif
