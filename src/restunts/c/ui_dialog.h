#ifndef RESTUNTS_UI_DIALOG_H
#define RESTUNTS_UI_DIALOG_H

#include "legacy.h"

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
legacy_s16 do_dea_textres(void);
void security_check(legacy_s16 question_index);
void sub_3702E(legacy_s16 left, legacy_s16 top, legacy_s16 right,
	legacy_s16 bottom, legacy_s16 color);

#endif
