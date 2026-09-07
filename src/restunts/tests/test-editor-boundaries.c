#include <stdint.h>
#include <stdio.h>
#define main editor_placement_test_main
#include "test-track-editor.c"
#undef main
#undef printf

static uint64_t editor_trace = UINT64_C(1469598103934665603);
static struct SHAPE2D drawing_shapes[256];

static void record_editor_word(legacy_u16 value)
{
	editor_trace = (editor_trace ^ (value & 255U)) * UINT64_C(1099511628211);
	editor_trace = (editor_trace ^ (value >> 8)) * UINT64_C(1099511628211);
}

static void record_editor_shape(unsigned int call, struct SHAPE2D *shape, legacy_s16 x,
								legacy_s16 y)
{
	record_editor_word(call);
	record_editor_word((legacy_u16)(shape - drawing_shapes));
	record_editor_word(x);
	record_editor_word(y);
}

legacy_u16 dos_mouse_get_button_count(void)
{
	return 0;
}
legacy_u8 subst_hillroad_track(legacy_u8 terrain, legacy_u8 tile)
{
	return terrain == 7U && tile == 1U ? 1U : 0U;
}
void sprite_shape_to_1(struct SHAPE2D *shape, legacy_s16 x, legacy_s16 y)
{
	record_editor_shape(1, shape, x, y);
}
void sprite_copy_image_at(struct SHAPE2D *shape, legacy_s16 x, legacy_s16 y)
{
	record_editor_shape(2, shape, x, y);
}
void sprite_putimage_and(struct SHAPE2D *shape, legacy_u16 x, legacy_u16 y)
{
	record_editor_shape(3, shape, x, y);
}
void sprite_putimage_or(struct SHAPE2D *shape, legacy_u16 x, legacy_u16 y)
{
	record_editor_shape(4, shape, x, y);
}
void putpixel_iconMask(struct SHAPE2D *shape, legacy_s16 x, legacy_s16 y)
{
	record_editor_shape(5, shape, x, y);
}
void putpixel_iconFillings(struct SHAPE2D *shape, legacy_s16 x, legacy_s16 y)
{
	record_editor_shape(6, shape, x, y);
}

static void record_maps(void)
{
	unsigned int i;
	for (i = 0; i < 900U; i++) {
		record_editor_word(element_tiles[i]);
		record_editor_word(terrain_tiles[i]);
	}
}

static void test_validation_boundaries(void)
{
	static const legacy_u8 tiles[] = {1, 33, 34, 35, 36, 102, 103, 108, 109, 170, 171, 174, 175};
	unsigned int terrain, index;
	struct TRACKOBJECT saved;
	for (index = 0; index < sizeof(tiles); index++) {
		saved = trkObjectList[tiles[index]];
		trkObjectList[tiles[index]].ss_multiTileFlag = 0;
		for (terrain = 0; terrain <= 11U; terrain++) {
			reset_editor();
			element_tiles[10 * 30 + 10] = tiles[index];
			terrain_tiles[19 * 30 + 10] = terrain;
			record_editor_word(track_editor_remove_invalid_terrain_tiles());
			record_maps();
		}
		trkObjectList[tiles[index]] = saved;
	}
}

static void test_multitile_cleanup(void)
{
	unsigned int flags, missing, index;
	struct TRACKOBJECT saved = trkObjectList[1];
	for (flags = 1; flags < 4; flags++) {
		for (missing = 0; missing < 8; missing++) {
			reset_editor();
			trkObjectList[1].ss_multiTileFlag = flags;
			element_tiles[310] = 1;
			element_tiles[311] = missing & 1U ? 0 : TRACK_TILE_CONTINUATION_EAST;
			element_tiles[340] = missing & 2U ? 0 : TRACK_TILE_CONTINUATION_SOUTH;
			element_tiles[341] = missing & 4U ? 0 : TRACK_TILE_CONTINUATION_SOUTHEAST;
			element_tiles[313] = TRACK_TILE_CONTINUATION_SOUTH;
			track_editor_remove_invalid_multitile_links();
			for (index = 0; index < 900U; index++) {
				record_editor_word(element_tiles[index]);
			}
		}
	}
	trkObjectList[1] = saved;
}

static void test_map_drawing(void)
{
	legacy_u8 track_cache[132], terrain_cache[132];
	unsigned int flags, viewport, index;
	struct TRACKOBJECT saved = trkObjectList[1];
	for (index = 0; index < 19U; index++) {
		track_editor_terrain_shapes[index] = &drawing_shapes[index];
	}
	track_editor_tile_masks[1] = &drawing_shapes[30];
	track_editor_tile_shapes[1] = &drawing_shapes[31];
	for (flags = 0; flags < 4; flags++) {
		for (viewport = 0; viewport < 4; viewport++) {
			reset_editor();
			memset(track_cache, 255, sizeof(track_cache));
			memset(terrain_cache, 255, sizeof(terrain_cache));
			trkObjectList[1].ss_multiTileFlag = flags;
			element_tiles[310] = 1;
			if (flags & 2U) {
				element_tiles[311] = TRACK_TILE_CONTINUATION_EAST;
			}
			if (flags & 1U) {
				element_tiles[340] = TRACK_TILE_CONTINUATION_SOUTH;
			}
			if (flags == 3U) {
				element_tiles[341] = TRACK_TILE_CONTINUATION_SOUTHEAST;
			}
			terrain_tiles[580] = 1;
			terrain_tiles[581] = 2;
			terrain_tiles[550] = 3;
			terrain_tiles[551] = 4;
			draw_2DtrackMap(10U + viewport % 2U, 10U + viewport / 2U, track_cache, terrain_cache);
			draw_2DtrackMap(10U + viewport % 2U, 10U + viewport / 2U, track_cache, terrain_cache);
			for (index = 0; index < 132U; index++) {
				record_editor_word(track_cache[index]);
				record_editor_word(terrain_cache[index]);
			}
		}
	}
	trkObjectList[1] = saved;
}

static legacy_s16 editor_dialog_result;
static legacy_u8 fixture_terrain[901];

void *__fmemcpy(void *dst, const void *src, legacy_u16 count)
{
	unsigned i;
	for (i = 0; i < count; i++) {
		((legacy_u8 *)dst)[i] = ((const legacy_u8 *)src)[i];
	}
	return dst;
}
void check_input(void)
{
	record_editor_word(201);
}
void sprite_select_screen_compat(void)
{
	record_editor_word(202);
}
legacy_s16 track_setup(void)
{
	record_editor_word(203);
	return 0;
}
legacy_s8 *locate_text_res(legacy_s8 *data, const legacy_s8 *name)
{
	(void)data;
	record_editor_word(204);
	record_editor_word(name[0]);
	return (legacy_s8 *)name;
}
legacy_u16 show_dialog(legacy_s16 type, legacy_s16 save, void *text, legacy_u16 x, legacy_u16 y,
					   legacy_s16 border, legacy_s16 *disabled, legacy_s16 initial)
{
	(void)text;
	(void)disabled;
	record_editor_word(205);
	record_editor_word(type);
	record_editor_word(save);
	record_editor_word(x);
	record_editor_word(y);
	record_editor_word(border);
	record_editor_word(initial);
	return editor_dialog_result;
}
legacy_s8 *locate_shape_alt(legacy_s8 *data, const legacy_s8 *name)
{
	(void)data;
	record_editor_word(206);
	record_editor_word(name[3]);
	return (legacy_s8 *)fixture_terrain;
}
legacy_s8 do_fileselect_dialog(legacy_s8 *dir, legacy_s8 *name, legacy_s8 *ext, legacy_s8 *prompt)
{
	(void)dir;
	(void)name;
	(void)ext;
	(void)prompt;
	record_editor_word(207);
	return 0;
}
legacy_s16 do_savefile_dialog(legacy_s8 *dir, legacy_s8 *name, legacy_s8 *prompt)
{
	(void)dir;
	(void)name;
	(void)prompt;
	record_editor_word(208);
	return 0;
}
void file_build_path(const legacy_s8 *dir, const legacy_s8 *name, const legacy_s8 *ext,
					 legacy_s8 *dst)
{
	(void)dir;
	(void)name;
	(void)ext;
	record_editor_word(209);
	dst[0] = 0;
}
void *file_read_fatal(const legacy_s8 *name, void *dest)
{
	(void)name;
	record_editor_word(210);
	return dest;
}
const legacy_s8 *file_find(const legacy_s8 *name)
{
	(void)name;
	record_editor_word(211);
	return 0;
}
legacy_s16 file_write_fatal(const legacy_s8 *name, void *src, legacy_u32 length)
{
	(void)name;
	(void)src;
	record_editor_word(212);
	record_editor_word((legacy_u16)length);
	return 0;
}
legacy_s16 highscore_load_or_create(legacy_s16 mode)
{
	record_editor_word(213);
	return mode;
}

static void record_editor_selection(void)
{
	unsigned i;
	record_editor_word(editor.key);
	record_editor_word(editor.page);
	record_editor_word(editor.focus);
	record_editor_word(editor.selected_tile);
	record_editor_word(editor.multi_tile);
	record_editor_word(editor.palette_dirty);
	record_editor_word(editor.map_dirty);
	record_editor_word(editor.menu_active);
	record_editor_word(editor.track_changed);
	record_editor_word(editor.validate_track);
	record_editor_word(editor.last_column);
	record_editor_word(editor.last_row);
	for (i = 0; i < 2U; i++) {
		record_editor_word(editor.selection_row[i]);
		record_editor_word(editor.selection_column[i]);
	}
	record_editor_word(editor.map_row_offset);
	record_editor_word(editor.map_column_offset);
}
static void test_key_dispatch(void)
{
	static const legacy_u16 keys[] = {KEY_ENTER,	KEY_SPACE, KEY_INSERT, '+',		   '-',
									  KEY_SHIFT_F1, KEY_HOME,  KEY_UP,	   KEY_DOWN,   KEY_LEFT,
									  KEY_RIGHT,	'c',	   'C',		   KEY_ESCAPE, 0};
	unsigned focus, page, i;
	for (focus = 0; focus < 2U; focus++) {
		for (page = 0; page <= 10U; page += 5U) {
			for (i = 0; i < sizeof(keys) / sizeof(keys[0]) + 10U; i++) {
				reset_editor();
				editor.focus = focus;
				editor.page = page;
				editor.key = i < sizeof(keys) / sizeof(keys[0])
								 ? keys[i]
								 : track_editor_page_keys[i - sizeof(keys) / sizeof(keys[0])];
				editor_dialog_result = -1;
				track_editor_handle_key(&editor);
				record_editor_selection();
			}
		}
	}
}
static void test_palette_activation(void)
{
	unsigned flags, page, row, column, dialog;
	struct TRACKOBJECT saved = trkObjectList[1];
	for (flags = 0; flags < 4U; flags++) {
		trkObjectList[1].ss_multiTileFlag = flags;
		for (page = 0; page <= 10U; page += 5U) {
			reset_editor();
			memset(palette, 1, sizeof(palette));
			editor.page = page;
			editor.selection_row[0] = editor.map_row_offset + 10U;
			editor.selection_column[0] = editor.map_column_offset + 11U;
			track_editor_activate_palette(&editor);
			record_editor_selection();
		}
	}
	trkObjectList[1] = saved;
	for (row = 6; row <= 9U; row++) {
		for (column = 0; column <= 3U; column += 3U) {
			for (dialog = 0; dialog < 3U; dialog++) {
				reset_editor();
				editor.selection_row[1] = row;
				editor.selection_column[1] = column;
				editor.page = 10;
				editor_dialog_result = dialog == 0 ? -1 : dialog == 1 ? 5 : 2;
				track_editor_activate_palette(&editor);
				record_editor_selection();
				record_editor_word(element_tiles[900]);
				record_editor_word(terrain_tiles[900]);
			}
		}
	}
}

int main(void)
{
	editor_placement_test_main();
	test_validation_boundaries();
	test_multitile_cleanup();
	test_map_drawing();
	test_key_dispatch();
	test_palette_activation();
	assert(editor_trace == UINT64_C(0x62b148357fd3e8a2));
	printf("test-editor-boundaries: passed\n");
	return 0;
}
