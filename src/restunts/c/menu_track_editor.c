#include "externs.h"
#include "fileio.h"
#include "legacy.h"
#include "memmgr.h"
#include "menu_internal.h"
#include "platform.h"
#include "shape2d.h"
#include "shape3d.h"

#define TRACK_TILE_CONTINUATION_SOUTHEAST 253U
#define TRACK_TILE_CONTINUATION_SOUTH 254U
#define TRACK_TILE_CONTINUATION_EAST 255U
#define TRACK_EDITOR_REFRESH_BLIT_MODE 254U
#define TRACK_EDITOR_CACHE_INVALID 255U
#define TRACK_EDITOR_POSITION_UNSET 255U
#define TRACK_EDITOR_PAGE_UNSET 255U
#define TRACK_EDITOR_INITIAL_BLIT_MODE 255U
#define TRACK_EDITOR_HOVER_UNSET 255U
#define TRACK_EDITOR_MOUSE_NO_HIT 255U
#define TRACK_EDITOR_SAVE_CANCELLED 255U
#define TRACK_EDITOR_DIALOG_CANCELLED 255U
#define TRACK_EDITOR_SCREEN_WIDTH 320U
#define TRACK_EDITOR_SCREEN_HEIGHT 200U
#define TRACK_EDITOR_MAP_LEFT 8
#define TRACK_EDITOR_MAP_TOP 4
#define TRACK_EDITOR_MAP_CLIP_RIGHT 200
#define TRACK_EDITOR_MAP_CLIP_BOTTOM 179
#define TRACK_EDITOR_TILE_SIZE 16U
#define TRACK_EDITOR_PALETTE_LEFT 220U
#define TRACK_EDITOR_PALETTE_TOP 36U
#define TRACK_EDITOR_PALETTE_WIDTH 96U
#define TRACK_EDITOR_PALETTE_ACTION_TOP 28
#define TRACK_EDITOR_PALETTE_ACTION_WIDTH 48U
#define TRACK_EDITOR_MULTITILE_CURSOR_SIZE 32U
#define TRACK_EDITOR_LABEL_Y 192
#define TRACK_EDITOR_HORIZONTAL_SCROLLBAR_X 9
#define TRACK_EDITOR_HORIZONTAL_SCROLLBAR_LENGTH 181
#define TRACK_EDITOR_VERTICAL_SCROLLBAR_X 202
#define TRACK_EDITOR_VERTICAL_SCROLLBAR_LENGTH 176
#define TRACK_EDITOR_PAGE_BAR_X 221
#define TRACK_EDITOR_PAGE_BAR_WIDTH 95
#define TRACK_EDITOR_PAGE_BAR_Y 133
#define TRACK_EDITOR_TITLE_X 217
#define TRACK_EDITOR_TITLE_Y 3
#define TRACK_EDITOR_TITLE_WIDTH 102
#define TRACK_EDITOR_TITLE_HEIGHT 22
#define TRACK_EDITOR_LEFT_FRAME_X 5
#define TRACK_EDITOR_LEFT_FRAME_Y 0
#define TRACK_EDITOR_LEFT_FRAME_WIDTH 206
#define TRACK_EDITOR_LEFT_FRAME_HEIGHT 190
#define TRACK_EDITOR_RIGHT_FRAME_X 217
#define TRACK_EDITOR_RIGHT_FRAME_Y 32
#define TRACK_EDITOR_RIGHT_FRAME_WIDTH 102
#define TRACK_EDITOR_RIGHT_FRAME_HEIGHT 158
#define TRACK_EDITOR_WIDE_BUTTON_X 221
#define TRACK_EDITOR_WIDE_BUTTON_Y 140
#define TRACK_EDITOR_WIDE_BUTTON_WIDTH 94
#define TRACK_EDITOR_BUTTON_LEFT_X 221
#define TRACK_EDITOR_BUTTON_RIGHT_X 269
#define TRACK_EDITOR_BUTTON_UPPER_Y 156
#define TRACK_EDITOR_BUTTON_LOWER_Y 172
#define TRACK_EDITOR_BUTTON_WIDTH 46
#define TRACK_EDITOR_BUTTON_HEIGHT 14

static legacy_u8 far* progress_box_shape;

extern struct TRACKOBJECT trkObjectList[];
extern struct SHAPE2D far* tracksmenushapes1[];
extern struct SHAPE2D far* tracksmenushape2dunk[];
extern struct SHAPE2D far* tracksmenushape2dunk2[];

legacy_u8 subst_hillroad_track(legacy_u8 terrain, legacy_u8 track);

void preRender_icons(legacy_u8 page)
{
	legacy_u16 row;
	legacy_u16 column;
	legacy_u16 x;
	legacy_u16 y;
	legacy_u8 tile;
	legacy_u8 multi_tile;

	for (row = 0; row < 6U; row++) {
		for (column = 0; column < 6U; column++) {
			tile = progress_box_shape[(legacy_u16)page * 36U + row * 6U +
				column];
			x = LEGACY_U16_WRAP_ADD(TRACK_EDITOR_PALETTE_LEFT,
				LEGACY_U16_WRAP_MUL(column, TRACK_EDITOR_TILE_SIZE));
			y = LEGACY_U16_WRAP_ADD(TRACK_EDITOR_PALETTE_TOP,
				LEGACY_U16_WRAP_MUL(row, TRACK_EDITOR_TILE_SIZE));
			if (page == 0) {
				sprite_shape_to_1(tracksmenushapes1[tile], x, y);
				continue;
			}
			if (tile >= TRACK_TILE_CONTINUATION_SOUTHEAST)
				continue;

			sprite_shape_to_1(tracksmenushapes1[0], x, y);
			multi_tile = trkObjectList[tile].ss_multiTileFlag;
			if (multi_tile == 1U || multi_tile == 3U)
				sprite_shape_to_1(tracksmenushapes1[0], x,
					LEGACY_U16_WRAP_ADD(y, 16U));
			if (multi_tile == 2U || multi_tile == 3U)
				sprite_shape_to_1(tracksmenushapes1[0],
					LEGACY_U16_WRAP_ADD(x, 16U), y);
			if (multi_tile == 3U)
				sprite_shape_to_1(tracksmenushapes1[0],
					LEGACY_U16_WRAP_ADD(x, 16U),
					LEGACY_U16_WRAP_ADD(y, 16U));
			putpixel_iconMask(tracksmenushape2dunk2[tile], x, y);
			putpixel_iconFillings(tracksmenushape2dunk[tile], x, y);
		}
	}
}

static legacy_u16 track_menu_previous_row(legacy_u16 row);
static legacy_u8 track_editor_map_tile(legacy_u8 column, legacy_u8 row);

void draw_2DtrackMap(
	legacy_u8 column_offset,
	legacy_u8 row_offset,
	legacy_u8* cached_track,
	legacy_u8* cached_terrain
) {
	legacy_u16 map_row;
	legacy_u16 map_column;
	legacy_u16 source_row;
	legacy_u16 source_column;
	legacy_u16 source_index;
	legacy_u16 cache_index;
	legacy_s16 x;
	legacy_s16 y;
	legacy_u8 tile;
	legacy_u8 terrain;
	legacy_u8 neighbor_tile;
	legacy_u8 multi_tile;

	for (map_row = 0; map_row < 11U; map_row++) {
		for (map_column = 0; map_column < 12U; map_column++) {
			source_row = LEGACY_U16_WRAP_ADD(row_offset, map_row);
			source_column = LEGACY_U16_WRAP_ADD(
				column_offset, map_column);
			source_index = LEGACY_U16_WRAP_ADD(
				(legacy_u16)trackrows[source_row], source_column);
			tile = td14_elem_map_main[source_index];
			terrain = td15_terr_map_main[LEGACY_U16_WRAP_ADD(
				(legacy_u16)terrainrows[source_row], source_column)];
			cache_index = LEGACY_U16_WRAP_ADD(
				LEGACY_U16_WRAP_MUL(map_row, 12U), map_column);
			x = LEGACY_S16_WRAP_ADD(
				LEGACY_S16_WRAP_MUL((legacy_s16)map_column, 16), 8);
			y = LEGACY_S16_WRAP_ADD(
				LEGACY_S16_WRAP_MUL((legacy_s16)map_row, 16), 4);

			if (tile < TRACK_TILE_CONTINUATION_SOUTHEAST) {
				if (tile == 0) {
					if (cached_track[cache_index] == 0 &&
						cached_terrain[cache_index] == terrain)
						continue;
					sprite_shape_to_1(tracksmenushapes1[terrain], x, y);
					cached_track[cache_index] = 0;
					cached_terrain[cache_index] = terrain;
					continue;
				}

				if (cached_track[cache_index] == tile &&
					cached_terrain[cache_index] == terrain)
					continue;
				cached_track[cache_index] = tile;
				cached_terrain[cache_index] = terrain;
				sprite_shape_to_1(tracksmenushapes1[terrain], x, y);
				multi_tile = (legacy_u8)
					trkObjectList[tile].ss_multiTileFlag;
				switch (multi_tile) {
				case 0:
					putpixel_iconMask(tracksmenushape2dunk2[tile], x, y);
					putpixel_iconFillings(tracksmenushape2dunk[tile], x, y);
					break;

				case 1:
					terrain = td15_terr_map_main[LEGACY_U16_WRAP_ADD(
						(legacy_u16)terrainrows[source_row + 1U],
						source_column)];
					sprite_putimage_and_alt(tracksmenushapes1[terrain],
						x, LEGACY_S16_WRAP_ADD(y, 16));
					sprite_putimage_and(tracksmenushape2dunk2[tile], x, y);
					sprite_putimage_or(tracksmenushape2dunk[tile], x, y);
					break;

				case 2:
					terrain = td15_terr_map_main[LEGACY_U16_WRAP_ADD(
						(legacy_u16)terrainrows[source_row],
						source_column + 1U)];
					sprite_putimage_and_alt(tracksmenushapes1[terrain],
						LEGACY_S16_WRAP_ADD(x, 16), y);
					sprite_putimage_and(tracksmenushape2dunk2[tile], x, y);
					sprite_putimage_or(tracksmenushape2dunk[tile], x, y);
					break;

				case 3:
					terrain = td15_terr_map_main[LEGACY_U16_WRAP_ADD(
						(legacy_u16)terrainrows[source_row],
						source_column + 1U)];
					sprite_putimage_and_alt(tracksmenushapes1[terrain],
						LEGACY_S16_WRAP_ADD(x, 16), y);
					terrain = td15_terr_map_main[LEGACY_U16_WRAP_ADD(
						(legacy_u16)terrainrows[source_row + 1U],
						source_column)];
					sprite_putimage_and_alt(tracksmenushapes1[terrain],
						x, LEGACY_S16_WRAP_ADD(y, 16));
					terrain = td15_terr_map_main[LEGACY_U16_WRAP_ADD(
						(legacy_u16)terrainrows[source_row + 1U],
						source_column + 1U)];
					sprite_putimage_and_alt(tracksmenushapes1[terrain],
						LEGACY_S16_WRAP_ADD(x, 16),
						LEGACY_S16_WRAP_ADD(y, 16));
					sprite_putimage_and(tracksmenushape2dunk2[tile], x, y);
					sprite_putimage_or(tracksmenushape2dunk[tile], x, y);
					break;
				}
				continue;
			}

			if (map_row != 0 && map_column != 0) {
				cached_track[cache_index] = TRACK_EDITOR_CACHE_INVALID;
				cached_terrain[cache_index] = TRACK_EDITOR_CACHE_INVALID;
				continue;
			}
			cached_track[cache_index] = TRACK_EDITOR_CACHE_INVALID;

			if (tile == TRACK_TILE_CONTINUATION_EAST && map_column == 0) {
				sprite_putimage_and_alt(tracksmenushapes1[terrain], x, y);
				terrain = td15_terr_map_main[LEGACY_U16_WRAP_ADD(
					(legacy_u16)terrainrows[source_row + 1U],
					source_column)];
				sprite_putimage_and_alt(tracksmenushapes1[terrain],
					x, LEGACY_S16_WRAP_ADD(y, 16));
				neighbor_tile = td14_elem_map_main[
					LEGACY_U16_WRAP_SUB(source_index, 1U)];
				sprite_putimage_and(tracksmenushape2dunk2[neighbor_tile],
					LEGACY_S16_WRAP_SUB(x, 16), y);
				sprite_putimage_or(tracksmenushape2dunk[neighbor_tile],
					LEGACY_S16_WRAP_SUB(x, 16), y);
			} else if (tile == TRACK_TILE_CONTINUATION_SOUTH && map_row == 0) {
				sprite_putimage_and_alt(tracksmenushapes1[terrain], x, y);
				terrain = td15_terr_map_main[LEGACY_U16_WRAP_ADD(
					(legacy_u16)terrainrows[source_row],
					source_column + 1U)];
				sprite_putimage_and_alt(tracksmenushapes1[terrain],
					LEGACY_S16_WRAP_ADD(x, 16), y);
				neighbor_tile = td14_elem_map_main[LEGACY_U16_WRAP_ADD(
					track_menu_previous_row(source_row), source_column)];
				sprite_putimage_and(tracksmenushape2dunk2[neighbor_tile],
					x, LEGACY_S16_WRAP_SUB(y, 16));
				sprite_putimage_or(tracksmenushape2dunk[neighbor_tile],
					x, LEGACY_S16_WRAP_SUB(y, 16));
			} else if (tile == TRACK_TILE_CONTINUATION_SOUTHEAST && map_row == 0 && map_column == 0) {
				sprite_putimage_and_alt(tracksmenushapes1[terrain], x, y);
				neighbor_tile = td14_elem_map_main[LEGACY_U16_WRAP_SUB(
					LEGACY_U16_WRAP_ADD(
						track_menu_previous_row(source_row), source_column),
					1U)];
				sprite_putimage_and(tracksmenushape2dunk2[neighbor_tile],
					LEGACY_S16_WRAP_SUB(x, 16),
					LEGACY_S16_WRAP_SUB(y, 16));
				sprite_putimage_or(tracksmenushape2dunk[neighbor_tile],
					LEGACY_S16_WRAP_SUB(x, 16),
					LEGACY_S16_WRAP_SUB(y, 16));
			}
		}
	}
}

static legacy_u16 track_menu_next_row(legacy_u16 row)
{
	if (row == 29U)
		return dos_mouse_get_button_count();
	return (legacy_u16)trackrows[row + 1U];
}

static legacy_u16 track_menu_previous_row(legacy_u16 row)
{
	if (row == 0)
		return (legacy_u16)word_45D3E;
	return (legacy_u16)trackrows[row - 1U];
}

void sub_2C9B4(void)
{
	legacy_u8 used[900];
	legacy_u16 row;
	legacy_u16 column;
	legacy_u16 current_index;
	legacy_u16 next_index;
	legacy_u16 east_index;
	legacy_u8 tile;
	legacy_u8 multi_tile;

	for (current_index = 0; current_index < 900U; current_index++)
		used[current_index] = 0;

	for (row = 0; row < 30U; row++) {
		for (column = 0; column < 30U; column++) {
			current_index = LEGACY_U16_WRAP_ADD(trackrows[row],
				column);
			tile = td14_elem_map_main[current_index];
			if (tile == 0)
				continue;
			if (tile >= TRACK_TILE_CONTINUATION_SOUTHEAST) {
				if (used[current_index] == 0)
					td14_elem_map_main[current_index] = 0;
				continue;
			}

			multi_tile = trkObjectList[tile].ss_multiTileFlag;
			switch (multi_tile) {
			case 1:
				next_index = LEGACY_U16_WRAP_ADD(
					track_menu_next_row(row), column);
				if (used[next_index] != 0 ||
					td14_elem_map_main[next_index] != TRACK_TILE_CONTINUATION_SOUTH)
					td14_elem_map_main[current_index] = 0;
				else
					used[next_index] = 1;
				break;

			case 2:
				east_index = LEGACY_U16_WRAP_ADD(current_index, 1U);
				if (used[east_index] != 0 ||
					td14_elem_map_main[east_index] !=
					TRACK_TILE_CONTINUATION_EAST)
					td14_elem_map_main[current_index] = 0;
				else
					used[east_index] = 1;
				break;

			case 3:
				east_index = LEGACY_U16_WRAP_ADD(current_index, 1U);
				next_index = LEGACY_U16_WRAP_ADD(
					track_menu_next_row(row), column);
				if (used[east_index] != 0 || used[next_index] != 0 ||
					used[LEGACY_U16_WRAP_ADD(next_index, 1U)] != 0 ||
					td14_elem_map_main[east_index] !=
					TRACK_TILE_CONTINUATION_EAST ||
					td14_elem_map_main[next_index] != TRACK_TILE_CONTINUATION_SOUTH ||
					td14_elem_map_main[
						LEGACY_U16_WRAP_ADD(next_index, 1U)] != TRACK_TILE_CONTINUATION_SOUTHEAST) {
					td14_elem_map_main[current_index] = 0;
				} else {
					used[east_index] = 1;
					used[next_index] = 1;
					used[LEGACY_U16_WRAP_ADD(next_index, 1U)] = 1;
				}
				break;
			}
		}
	}
}

legacy_s16 sub_2C81C(void)
{
	legacy_u16 row;
	legacy_u16 column;
	legacy_u16 current_index;
	legacy_u8 terrain;
	legacy_u8 tile;
	legacy_u8 error;

	sub_2C9B4();
	error = 0;
	for (row = 0; row < 30U; row++) {
		for (column = 0; column < 30U; column++) {
			terrain = td15_terr_map_main[
				LEGACY_U16_WRAP_ADD(terrainrows[row], column)];
			current_index = LEGACY_U16_WRAP_ADD(trackrows[row],
				column);
			tile = td14_elem_map_main[current_index];
			if (tile == 0 || terrain == 0 || terrain == 6U)
				continue;

			if (terrain >= 1U && terrain <= 5U) {
				tile = track_editor_map_tile((legacy_u8)column,
					(legacy_u8)row);

				if (!((tile >= 0x22U && tile <= 0x23U) ||
					(tile >= 0x67U && tile <= 0x6CU) ||
					(tile >= 0xABU && tile <= 0xAEU))) {
					td14_elem_map_main[current_index] = 0;
					error = 0x0C;
				}
			} else if (terrain >= 7U && terrain <= 10U) {
				if (subst_hillroad_track(terrain, tile) == 0) {
					td14_elem_map_main[current_index] = 0;
					error = 0x0D;
				}
			} else {
				td14_elem_map_main[current_index] = 0;
				error = 0x0E;
			}
		}
	}
	if (error != 0)
		sub_2C9B4();
	return error;
}

static legacy_u8 track_editor_palette_tile(legacy_u8 page,
	legacy_u8 row, legacy_u8 column)
{
	return progress_box_shape[(legacy_u16)page * 36U +
		(legacy_u16)row * 6U + column];
}

static void track_editor_skip_previous_placeholders(legacy_u8 page,
	legacy_u8* row, legacy_u8* column)
{
	legacy_u8 tile;

	while (track_editor_palette_tile(page, *row, *column) >= TRACK_TILE_CONTINUATION_SOUTH) {
		tile = track_editor_palette_tile(page, *row, *column);
		if (tile == TRACK_TILE_CONTINUATION_EAST)
			(*column)--;
		else
			(*row)--;
	}
}

static legacy_u8 track_editor_map_tile(legacy_u8 column, legacy_u8 row)
{
	legacy_u16 source_index;
	legacy_u8 tile;

	source_index = LEGACY_U16_WRAP_ADD((legacy_u16)trackrows[row], column);
	tile = td14_elem_map_main[source_index];
	if (tile == TRACK_TILE_CONTINUATION_SOUTHEAST) {
		source_index = LEGACY_U16_WRAP_SUB(
			LEGACY_U16_WRAP_ADD(track_menu_previous_row(row), column), 1U);
		tile = td14_elem_map_main[source_index];
	} else if (tile == TRACK_TILE_CONTINUATION_SOUTH) {
		source_index = LEGACY_U16_WRAP_ADD(
			track_menu_previous_row(row), column);
		tile = td14_elem_map_main[source_index];
	} else if (tile == TRACK_TILE_CONTINUATION_EAST) {
		tile = td14_elem_map_main[LEGACY_U16_WRAP_SUB(source_index, 1U)];
	}
	return tile;
}

static void track_editor_toggle_highlight(legacy_s16 x, legacy_s16 y,
	legacy_u8 width, legacy_u8 height)
{
	sub_3702E(x, LEGACY_S16_WRAP_SUB(y, 1),
		LEGACY_S16_WRAP_ADD(x, width),
		LEGACY_S16_WRAP_SUB(LEGACY_S16_WRAP_ADD(y, height), 1),
		word_407F2);
}

static void track_editor_show_message(legacy_s8 far* text_resource,
	const legacy_s8* resource_id)
{
	show_dialog(DIALOG_TYPE_ACKNOWLEDGEMENT, DIALOG_SAVE_BACKGROUND,
		locate_text_res(text_resource, (legacy_s8*)resource_id),
		DIALOG_AUTO_POSITION, DIALOG_AUTO_POSITION,
		performGraphColor, 0, 0);
}

static void track_editor_save_track(legacy_u8* track_changed,
	legacy_u8* map_dirty)
{
	legacy_s8 far* text;
	legacy_u8 save_status;
	legacy_s16 result;
	legacy_s16 write_result;

	save_status = 0;
	g_is_busy = 1;
	while (save_status == 0) {
		sprite_copy_2_to_1_2();
		*map_dirty = 1;
		text = locate_text_res((legacy_s8 far*)mainresptr, "trk");
		if (do_savefile_dialog(byte_3B80C,
			gameconfig.game_trackname, text) == 0) {
			save_status = TRACK_EDITOR_SAVE_CANCELLED;
			break;
		}
		file_build_path(byte_3B80C,
			gameconfig.game_trackname, ".trk", g_path_buf);
		save_status = 1;
		if (file_find(g_path_buf) != 0) {
			result = LEGACY_S16_FROM_BITS(show_dialog(DIALOG_TYPE_MENU,
				DIALOG_SAVE_BACKGROUND,
				locate_text_res((legacy_s8 far*)mainresptr, "fex"),
				DIALOG_AUTO_POSITION, DIALOG_AUTO_POSITION,
				performGraphColor, 0, 0));
			if (result == -1) {
				save_status = TRACK_EDITOR_SAVE_CANCELLED;
				break;
			}
			if (result == 0) {
				save_status = 0;
				continue;
			}
		}
		write_result = file_write_fatal(g_path_buf,
			td14_elem_map_main, 0x70AUL);
		if (write_result == 0)
			highscore_write_a(1);
		if (write_result != 0) {
			track_editor_show_message(
				(legacy_s8 far*)mainresptr, "ser");
			save_status = 0;
		} else {
			*track_changed = 0;
		}
	}
	g_is_busy = 0;
}

static void track_editor_swap_tiles(legacy_u8* selected_tile,
	legacy_u8* saved_tile, legacy_u8* palette_dirty)
{
	legacy_u8 value;

	value = *selected_tile;
	*selected_tile = *saved_tile;
	*saved_tile = value;
	*palette_dirty = 1;
}

void load_tracks_menu_shapes(void)
{
	static legacy_s8 terrain_shape_names[] =
		"flatlakelak1lak2lak3lak4highgoungouwgousgouegou1gou2gou3gou4gou5gou6gou7gou8";
	static legacy_s8 cursor_shape_names[] = "crs0crs1crs2crs3";
	static legacy_s8 under_cursor_shape_names[] = "ucr0ucr1ucr2ucr3";
	static const legacy_s8 error_resource_ids[] =
		"eokenseieemseedewwefuenpestejsejdeteewaefteat";
	static const struct BUTTON_AREA buttons[5] = {
		{ 9, 199, 181, 187 },
		{ 202, 206, 4, 179 },
		{ 220, 315, 132, 139 },
		{ 8, 199, 4, 179 },
		{ 220, 315, 36, 187 }
	};
	static const legacy_u16 page_keys[10] = {
		KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5,
		KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10
	};
	static const legacy_u8 maximum_columns[2] = { 30, 6 };
	static const legacy_u8 maximum_rows[2] = { 29, 9 };
	legacy_u8 cached_track[132];
	legacy_u8 cached_terrain[132];
	struct SPRITE far* cursor_sprites[4];
	legacy_s8 far* shape_resource;
	legacy_s8 far* text_resource;
	legacy_s8 far* shape_name_resource;
	legacy_s8 far* mask_name_resource;
	legacy_s8 far* text_name_resource;
	legacy_s8 far* text;
	struct SHAPE2D far* terrain_shape;
	legacy_u8 selection_column[2];
	legacy_u8 selection_row[2];
	legacy_u8 map_column_offset;
	legacy_u8 map_row_offset;
	legacy_u8 previous_column_offset;
	legacy_u8 previous_row_offset;
	legacy_u8 page;
	legacy_u8 previous_page;
	legacy_u8 selected_tile;
	legacy_u8 saved_tile;
	legacy_u8 last_column;
	legacy_u8 last_row;
	legacy_u8 hovered_tile;
	legacy_u8 previous_hovered_tile;
	legacy_u8 multi_tile;
	legacy_u8 tile_width;
	legacy_u8 tile_height;
	legacy_u8 cursor_width;
	legacy_u8 cursor_height;
	legacy_u8 map_dirty;
	legacy_u8 palette_dirty;
	legacy_u8 scrollbars_dirty;
	legacy_u8 validate_track;
	legacy_u8 validation_error;
	legacy_u8 track_changed;
	legacy_u8 menu_active;
	legacy_u8 blit_mode;
	legacy_u8 cursor_drawn;
	legacy_u8 focus;
	legacy_u8 blink_focus;
	legacy_u8 hit;
	legacy_u8 clicked_column;
	legacy_u8 clicked_row;
	legacy_u8 dialog_result;
	legacy_u8 step;
	legacy_u8 value;
	legacy_u8 animation_saved_tile;
	legacy_u16 index;
	legacy_u16 source_index;
	legacy_u16 path_animation_index;
	legacy_u16 blink_timer;
	legacy_u16 delta;
	legacy_u16 key;
	legacy_u16 key_index;
	legacy_s16 label_width;
	legacy_s16 previous_label_width;
	legacy_s16 cursor_x;
	legacy_s16 cursor_y;
	legacy_s16 result;
	legacy_s8 terrain_id[5];
	legacy_s8* resource_id;

	shape_resource = (legacy_s8 far*)file_load_shape2d_fatal("sdtedit");
	locate_many_resources(shape_resource, terrain_shape_names,
		(legacy_s8 far**)tracksmenushapes1);
	locate_many_resources(shape_resource, cursor_shape_names,
		(legacy_s8 far**)tracksmenushapes2);
	locate_many_resources(shape_resource, under_cursor_shape_names,
		(legacy_s8 far**)tracksmenushapes3);
	for (index = 0; index < 4U; index++) {
		cursor_sprites[index] = sprite_make_wnd(
			LEGACY_U16_WRAP_MUL(shape2d_get_width(
				tracksmenushapes2[index]),
				(legacy_u16)video_flag1_is1),
			shape2d_get_height(tracksmenushapes2[index]), 0x0FU);
	}

	text_resource = (legacy_s8 far*)file_load_resfile("tedit");
	render_window_sprite = sprite_make_wnd(0x140U, 0xC8U, 0x0FU);
	progress_box_shape = (legacy_u8 far*)locate_shape_alt(text_resource, "pbox");
	shape_name_resource = locate_shape_alt(text_resource, "snam");
	mask_name_resource = locate_shape_alt(text_resource, "mnam");
	text_name_resource = locate_shape_alt(text_resource, "tnam");

	for (index = 0; index < 132U; index++) {
		cached_track[index] = TRACK_EDITOR_CACHE_INVALID;
		cached_terrain[index] = TRACK_EDITOR_CACHE_INVALID;
	}
	for (index = 0; index < 186U; index++) {
		__fmemcpy(&resID_byte1, shape_name_resource + index * 4U, 4U);
		tracksmenushape2dunk[index] = (struct SHAPE2D far*)
			locate_shape_fatal(shape_resource, &resID_byte1);
		__fmemcpy(&resID_byte1, mask_name_resource + index * 4U, 4U);
		tracksmenushape2dunk2[index] = (struct SHAPE2D far*)
			locate_shape_fatal(shape_resource, &resID_byte1);
	}

	last_column = TRACK_EDITOR_POSITION_UNSET;
	last_row = TRACK_EDITOR_POSITION_UNSET;
	saved_tile = 0;
	selected_tile = 0;
	previous_page = TRACK_EDITOR_PAGE_UNSET;
	map_dirty = 1;
	palette_dirty = 1;
	scrollbars_dirty = 1;
	validate_track = 1;
	validation_error = 0;
	track_changed = 0;
	menu_active = 1;
	blit_mode = TRACK_EDITOR_INITIAL_BLIT_MODE;
	page = 1;
	selection_column[0] = byte_45D90;
	selection_row[0] = byte_45E16;
	selection_column[1] = 0;
	selection_row[1] = 7;
	map_column_offset = 0;
	map_row_offset = 0;
	previous_column_offset = TRACK_EDITOR_POSITION_UNSET;
	previous_row_offset = TRACK_EDITOR_POSITION_UNSET;
	previous_hovered_tile = TRACK_EDITOR_HOVER_UNSET;
	previous_label_width = 0;
	focus = 0;
	path_animation_index = 0;

	sprite_copy_wnd_to_1_clear();
	draw_button(locate_text_res(text_resource, "bti"),
		TRACK_EDITOR_TITLE_X, TRACK_EDITOR_TITLE_Y, TRACK_EDITOR_TITLE_WIDTH,
		TRACK_EDITOR_TITLE_HEIGHT, word_407F4, word_407F6, word_407F8, 0);
	draw_lines_unk(TRACK_EDITOR_LEFT_FRAME_X, TRACK_EDITOR_LEFT_FRAME_Y,
		TRACK_EDITOR_LEFT_FRAME_WIDTH, TRACK_EDITOR_LEFT_FRAME_HEIGHT,
		11, 9, 1);
	draw_lines_unk(TRACK_EDITOR_RIGHT_FRAME_X, TRACK_EDITOR_RIGHT_FRAME_Y,
		TRACK_EDITOR_RIGHT_FRAME_WIDTH, TRACK_EDITOR_RIGHT_FRAME_HEIGHT,
		11, 9, 1);
	draw_button(locate_text_res(text_resource, "bsc"),
		TRACK_EDITOR_WIDE_BUTTON_X, TRACK_EDITOR_WIDE_BUTTON_Y,
		TRACK_EDITOR_WIDE_BUTTON_WIDTH, TRACK_EDITOR_BUTTON_HEIGHT,
		word_407F4, word_407F6, word_407F8, 0);
	draw_button(locate_text_res(text_resource, "blo"),
		TRACK_EDITOR_BUTTON_LEFT_X, TRACK_EDITOR_BUTTON_UPPER_Y,
		TRACK_EDITOR_BUTTON_WIDTH, TRACK_EDITOR_BUTTON_HEIGHT,
		word_407F4, word_407F6, word_407F8, 0);
	draw_button(locate_text_res(text_resource, "bsa"),
		TRACK_EDITOR_BUTTON_LEFT_X, TRACK_EDITOR_BUTTON_LOWER_Y,
		TRACK_EDITOR_BUTTON_WIDTH, TRACK_EDITOR_BUTTON_HEIGHT,
		word_407F4, word_407F6, word_407F8, 0);
	draw_button(locate_text_res(text_resource, "bcl"),
		TRACK_EDITOR_BUTTON_RIGHT_X, TRACK_EDITOR_BUTTON_UPPER_Y,
		TRACK_EDITOR_BUTTON_WIDTH, TRACK_EDITOR_BUTTON_HEIGHT,
		word_407F4, word_407F6, word_407F8, 0);
	draw_button(locate_text_res(text_resource, "bex"),
		TRACK_EDITOR_BUTTON_RIGHT_X, TRACK_EDITOR_BUTTON_LOWER_Y,
		TRACK_EDITOR_BUTTON_WIDTH, TRACK_EDITOR_BUTTON_HEIGHT,
		word_407F4, word_407F6, word_407F8, 0);

	while (menu_active != 0) {
		if (palette_dirty != 0 || page != previous_page) {
			tile_width = 1;
			tile_height = 1;
			multi_tile = 0;
			if (page != 0) {
				multi_tile = (legacy_u8)
					trkObjectList[selected_tile].ss_multiTileFlag;
				if (multi_tile == 1U) {
					tile_height = 2;
					multi_tile = 1;
				} else if (multi_tile == 2U) {
					tile_width = 2;
					multi_tile = 2;
				} else if (multi_tile == 3U) {
					tile_width = 2;
					tile_height = 2;
					multi_tile = 3;
				}
			}

			if (focus == 0) {
				if (selection_column[0] == 29U && tile_width == 2U)
					selection_column[0]--;
				if (selection_row[0] == 29U && tile_height == 2U)
					selection_row[0]--;
				while ((legacy_s16)(selection_column[0] - map_column_offset +
					tile_width) > 12)
					map_column_offset++;
				while (selection_column[0] < map_column_offset)
					map_column_offset--;
				while ((legacy_s16)(selection_row[0] - map_row_offset +
					tile_height) > 11)
					map_row_offset++;
				while (selection_row[0] < map_row_offset)
					map_row_offset--;
				if (map_column_offset != previous_column_offset ||
					map_row_offset != previous_row_offset) {
					previous_column_offset = map_column_offset;
					previous_row_offset = map_row_offset;
					map_dirty = 1;
					scrollbars_dirty = 1;
				}
			}

			if (page != previous_page) {
				palette_dirty = 1;
				previous_page = page;
				while (selection_row[1] < 6U &&
					track_editor_palette_tile(page,
						selection_row[1], selection_column[1]) >= TRACK_TILE_CONTINUATION_SOUTH) {
					value = track_editor_palette_tile(page,
						selection_row[1], selection_column[1]);
					if (value == TRACK_TILE_CONTINUATION_EAST)
						selection_column[1]--;
					else
						selection_row[1]--;
				}
				sprite_copy_wnd_to_1();
				preRender_icons(page);
				if (page == 0)
					mouse_track_op(0, TRACK_EDITOR_PAGE_BAR_X,
						TRACK_EDITOR_PAGE_BAR_WIDTH, TRACK_EDITOR_PAGE_BAR_Y,
						5, 0, 1, 1);
				else
					mouse_track_op(0, TRACK_EDITOR_PAGE_BAR_X,
						TRACK_EDITOR_PAGE_BAR_WIDTH, TRACK_EDITOR_PAGE_BAR_Y,
						5, page - 1U, 1, 10);
			}
		}

		if (validate_track != 0) {
			validate_track = 0;
			validation_error = (legacy_u8)sub_2C81C();
		}

		if (map_dirty != 0 || palette_dirty != 0) {
			sprite_copy_wnd_to_1();
			if (map_dirty != 0) {
				map_dirty = 0;
				if (scrollbars_dirty != 0) {
					scrollbars_dirty = 0;
					mouse_track_op(0, TRACK_EDITOR_HORIZONTAL_SCROLLBAR_X,
						TRACK_EDITOR_LABEL_Y,
						TRACK_EDITOR_HORIZONTAL_SCROLLBAR_LENGTH, 5,
						map_column_offset, 12, 30);
					mouse_track_op(0, TRACK_EDITOR_VERTICAL_SCROLLBAR_X,
						5, 4, TRACK_EDITOR_VERTICAL_SCROLLBAR_LENGTH,
						map_row_offset, 11, 30);
				}
				sprite_set_1_size(TRACK_EDITOR_MAP_LEFT,
					TRACK_EDITOR_MAP_CLIP_RIGHT, TRACK_EDITOR_MAP_TOP,
					TRACK_EDITOR_MAP_CLIP_BOTTOM);
				draw_2DtrackMap(map_column_offset, map_row_offset,
					cached_track, cached_terrain);
				sprite_set_1_size(0, TRACK_EDITOR_SCREEN_WIDTH,
					0, TRACK_EDITOR_SCREEN_HEIGHT);
			}

			if (palette_dirty != 0) {
				palette_dirty = 0;
				sprite_set_1_from_argptr(cursor_sprites[multi_tile]);
				if (page == 0) {
					sprite_shape_to_1(tracksmenushapes1[selected_tile], 0, 0);
					preRender_line(1, 0, 0x0F, 0, performGraphColor);
					preRender_line(1, 0x0E, 0x0F, 0x0E, performGraphColor);
					preRender_line(1, 0, 1, 0x0E, performGraphColor);
					preRender_line(0x0F, 0, 0x0F, 0x0E, performGraphColor);
				} else {
					sprite_shape_to_1(tracksmenushapes2[multi_tile], 0, 0);
					if (selected_tile != 0) {
						putpixel_iconMask(tracksmenushape2dunk2[selected_tile], 0, 0);
						putpixel_iconFillings(tracksmenushape2dunk[selected_tile], 0, 0);
					}
				}
			}

			sprite_blit_to_video(render_window_sprite, LEGACY_S8_FROM_BITS(blit_mode));
			blit_mode = TRACK_EDITOR_REFRESH_BLIT_MODE;
			previous_hovered_tile = TRACK_EDITOR_HOVER_UNSET;
		}

		sprite_copy_2_to_1_2();
		if (focus == 0) {
			cursor_width = (legacy_u8)(tile_width << 4);
			cursor_height = (legacy_u8)(tile_height << 4);
			cursor_x = (legacy_s16)((selection_column[0] -
				map_column_offset) * 16U + 8U);
			cursor_y = (legacy_s16)((selection_row[0] -
				map_row_offset) * 16U + 4U);
			hovered_tile = track_editor_map_tile(
				selection_column[0], selection_row[0]);
		} else {
			cursor_width = TRACK_EDITOR_TILE_SIZE;
			cursor_height = TRACK_EDITOR_TILE_SIZE;
			cursor_y = (legacy_s16)(selection_row[1] *
				TRACK_EDITOR_TILE_SIZE + TRACK_EDITOR_PALETTE_TOP);
			if (selection_row[1] == 6U) {
				cursor_x = TRACK_EDITOR_PALETTE_LEFT;
				cursor_width = TRACK_EDITOR_PALETTE_WIDTH;
				cursor_height = 8U;
			} else if (selection_row[1] == 7U) {
				selection_column[1] = 0;
				cursor_x = TRACK_EDITOR_PALETTE_LEFT;
				cursor_width = TRACK_EDITOR_PALETTE_WIDTH;
				cursor_y = LEGACY_S16_WRAP_SUB(cursor_y, 8);
			} else if (selection_row[1] > 7U) {
				cursor_y = LEGACY_S16_WRAP_SUB(cursor_y, 8);
				selection_column[1] = selection_column[1] < 3U ? 0U : 3U;
				cursor_x = (legacy_s16)(selection_column[1] *
					TRACK_EDITOR_TILE_SIZE + TRACK_EDITOR_PALETTE_LEFT);
				cursor_width = TRACK_EDITOR_PALETTE_ACTION_WIDTH;
			} else {
				cursor_x = (legacy_s16)(selection_column[1] *
					TRACK_EDITOR_TILE_SIZE + TRACK_EDITOR_PALETTE_LEFT);
				if (track_editor_palette_tile(page, selection_row[1],
					selection_column[1] + 6U) == TRACK_TILE_CONTINUATION_SOUTH)
					cursor_height = TRACK_EDITOR_MULTITILE_CURSOR_SIZE;
				if (selection_column[1] < 5U &&
					track_editor_palette_tile(page, selection_row[1],
						selection_column[1] + 1U) ==
						TRACK_TILE_CONTINUATION_EAST)
					cursor_width = TRACK_EDITOR_MULTITILE_CURSOR_SIZE;
			}
			hovered_tile = selection_row[1] < 6U ?
				track_editor_palette_tile(page, selection_row[1],
					selection_column[1]) : 0;
			if (hovered_tile >= TRACK_TILE_CONTINUATION_SOUTHEAST || page == 0)
				hovered_tile = 0;
		}

		if (hovered_tile != previous_hovered_tile) {
			mouse_draw_opaque_check();
			font_set_unk(dialog_fnt_colour, 0);
			resource_id = &resID_byte1;
			__fmemcpy(resource_id, text_name_resource +
				(legacy_u16)hovered_tile * 3U, 3U);
			resource_id[3] = 0;
			text = locate_text_res(text_resource, resource_id);
			copy_string(resource_id, text);
			label_width = (legacy_s16)font_op2(resource_id);
			sub_345BC(resource_id, TRACK_EDITOR_MAP_LEFT,
				TRACK_EDITOR_LABEL_Y);
			if (previous_label_width > label_width) {
				sprite_1_unk(LEGACY_S16_WRAP_ADD(label_width,
					TRACK_EDITOR_MAP_LEFT), TRACK_EDITOR_LABEL_Y,
					LEGACY_S16_WRAP_SUB(previous_label_width,
						label_width), 8, 0);
			}
			mouse_draw_transparent_check();
			previous_label_width = label_width;
			previous_hovered_tile = hovered_tile;
		}

		if (validation_error != 0) {
			resource_id = (legacy_s8*)error_resource_ids +
				(legacy_u16)validation_error * 3U;
			__fmemcpy(&resID_byte1, resource_id, 3U);
			*(&resID_byte1 + 3) = 0;
			track_editor_show_message(text_resource, &resID_byte1);
			validation_error = 0;
		}

		blink_timer = 0x63U;
		cursor_drawn = 0;
		mouse_draw_opaque_check();
		blink_focus = focus;
		if (blink_focus == 0)
			sprite_clear_shape_alt(tracksmenushapes3[multi_tile],
				cursor_x, cursor_y);

		key = 0;
		while (key == 0) {
			if (blink_timer > 0x0FU) {
				mouse_draw_opaque_check();
				if (blink_focus == 0) {
					if (cursor_drawn != 0)
						sprite_shape_to_1(tracksmenushapes3[multi_tile],
							cursor_x, cursor_y);
					else
						sprite_shape_to_1(
							cursor_sprites[multi_tile]->sprite_bitmapptr,
							cursor_x, cursor_y);
				} else {
					track_editor_toggle_highlight(cursor_x, cursor_y,
						cursor_width, cursor_height);
				}
				mouse_draw_transparent_check();
				cursor_drawn ^= 1U;
				blink_timer = 0;
			}

			delta = (legacy_u16)timer_get_delta_alt();
			blink_timer = LEGACY_U16_WRAP_ADD(blink_timer, delta);
			key = (legacy_u16)input_checking(LEGACY_S16_FROM_BITS(delta));
			hit = (legacy_u8)mouse_multi_hittest(5, buttons);
			if (hit != TRACK_EDITOR_MOUSE_NO_HIT) {
				if (hit == 0U && (mouse_butstate & 3) != 0) {
					focus = 0;
					clicked_column = (legacy_u8)mouse_track_op(1,
						TRACK_EDITOR_HORIZONTAL_SCROLLBAR_X,
						TRACK_EDITOR_LABEL_Y,
						TRACK_EDITOR_HORIZONTAL_SCROLLBAR_LENGTH, 5,
						map_column_offset, 12, 30);
					selection_column[0] = (legacy_u8)(selection_column[0] +
						clicked_column - map_column_offset);
					map_column_offset = clicked_column;
					key = 1;
				} else if (hit == 1U && (mouse_butstate & 3) != 0) {
					focus = 0;
					clicked_row = (legacy_u8)mouse_track_op(1,
						TRACK_EDITOR_VERTICAL_SCROLLBAR_X, 5, 4,
						TRACK_EDITOR_VERTICAL_SCROLLBAR_LENGTH,
						map_row_offset, 11, 30);
					selection_row[0] = (legacy_u8)(selection_row[0] +
						clicked_row - map_row_offset);
					map_row_offset = clicked_row;
					key = 1;
				} else if (hit == 2U) {
					if ((mouse_butstate & 3) != 0) {
						focus = 1;
						page = (legacy_u8)mouse_track_op(1,
							TRACK_EDITOR_PAGE_BAR_X,
							TRACK_EDITOR_PAGE_BAR_WIDTH,
							TRACK_EDITOR_PAGE_BAR_Y, 5, page - 1U, 1, 10) + 1U;
						key = 1;
					}
				} else if (hit == 3U) {
					clicked_column = (legacy_u8)LEGACY_S16_DIV_OR_ZERO(
						LEGACY_S16_WRAP_SUB(mouse_xpos, 8), 16);
					clicked_row = (legacy_u8)LEGACY_S16_DIV_OR_ZERO(
						LEGACY_S16_WRAP_SUB(mouse_ypos, 4), 16);
					if (page != 0) {
						if (clicked_row == 10U &&
							((legacy_u8)trkObjectList[selected_tile].ss_multiTileFlag & 1U) != 0)
							clicked_row--;
						if (clicked_column == 11U &&
							((legacy_u8)trkObjectList[selected_tile].ss_multiTileFlag & 2U) != 0)
							clicked_column--;
					}
					clicked_column = (legacy_u8)(clicked_column + map_column_offset);
					clicked_row = (legacy_u8)(clicked_row + map_row_offset);
					if (focus != 0 ||
						selection_column[0] != clicked_column ||
						selection_row[0] != clicked_row) {
						focus = 0;
						selection_column[0] = clicked_column;
						selection_row[0] = clicked_row;
						key = 1;
					}
					if (key == KEY_SPACE)
						key = KEY_ENTER;
				} else if (hit == 4U) {
					clicked_column = (legacy_u8)LEGACY_S16_DIV_OR_ZERO(
						LEGACY_S16_WRAP_SUB(mouse_xpos,
							TRACK_EDITOR_PALETTE_LEFT), TRACK_EDITOR_TILE_SIZE);
					clicked_row = (legacy_u8)LEGACY_S16_DIV_OR_ZERO(
						LEGACY_S16_WRAP_SUB(mouse_ypos,
							TRACK_EDITOR_PALETTE_TOP), TRACK_EDITOR_TILE_SIZE);
					if (clicked_row < 6U) {
						if (track_editor_palette_tile(page, clicked_row,
							clicked_column) == TRACK_TILE_CONTINUATION_SOUTH)
							clicked_row--;
						if (track_editor_palette_tile(page, clicked_row,
							clicked_column) == TRACK_TILE_CONTINUATION_EAST)
							clicked_column--;
					} else {
						clicked_row = (legacy_u8)LEGACY_S16_DIV_OR_ZERO(
							LEGACY_S16_WRAP_SUB(mouse_ypos,
								TRACK_EDITOR_PALETTE_ACTION_TOP),
							TRACK_EDITOR_TILE_SIZE);
						if (clicked_row == 7U)
							clicked_column = 0;
						else if (clicked_column >= 3U)
							clicked_column = 3;
						else
							clicked_column = 0;
					}
					if (focus == 0 ||
						selection_column[1] != clicked_column ||
						selection_row[1] != clicked_row) {
						selection_column[1] = clicked_column;
						selection_row[1] = clicked_row;
						focus = 1;
						key = 1;
					}
					if (key == KEY_SPACE)
						key = KEY_ENTER;
				}
			}
			if (key == 1U)
				last_column = TRACK_EDITOR_POSITION_UNSET;
			if (key == 0 && path_animation_index != 0)
				key = 1;
		}
		if (path_animation_index != 0)
			(void)timer_get_counter_unk(10UL);

		if (cursor_drawn != 0) {
			mouse_draw_opaque_check();
			if (blink_focus == 0) {
				sprite_shape_to_1(tracksmenushapes3[multi_tile],
					cursor_x, cursor_y);
			} else {
				track_editor_toggle_highlight(cursor_x, cursor_y,
					cursor_width, cursor_height);
			}
			mouse_draw_transparent_check();
		}

		if (path_animation_index != 0) {
			if (key == 1U && focus == 0)
				path_animation_index = (legacy_u16)
					(track_pieces_counter - 1);
			selection_column[0] = (legacy_u8)
				td21_col_from_path[path_animation_index];
			selection_row[0] = (legacy_u8)
				td22_row_from_path[path_animation_index];
			selected_tile = track_editor_map_tile(
				selection_column[0], selection_row[0]);
			map_dirty = 1;
			palette_dirty = 1;
			path_animation_index++;
			if (path_animation_index >= (legacy_u16)track_pieces_counter) {
				selected_tile = animation_saved_tile;
				path_animation_index = 0;
			}
			palette_dirty = 1;
			map_dirty = 1;
			continue;
		}

		for (key_index = 0; key_index < 10U; key_index++) {
			if (key == page_keys[key_index]) {
				page = (legacy_u8)(key_index + 1U);
				key = 0;
				break;
			}
		}

		if (key == 0x63U || key == 0x43U) {
			result = (legacy_s8)track_setup();
			resource_id = (legacy_s8*)error_resource_ids +
				(legacy_u16)(legacy_u8)result * 3U;
			__fmemcpy(&resID_byte1, resource_id, 3U);
			*(&resID_byte1 + 3) = 0;
			track_editor_show_message(text_resource, &resID_byte1);
			if (result > 1) {
				focus = 0;
				if (track_pieces_counter == 0) {
					selection_column[0] = byte_45D90;
					selection_row[0] = byte_45E16;
				} else {
					animation_saved_tile = selected_tile;
					selection_column[0] = (legacy_u8)td21_col_from_path[0];
					selection_row[0] = (legacy_u8)td22_row_from_path[0];
					selected_tile = track_editor_map_tile(
						selection_column[0], selection_row[0]);
					path_animation_index = 1;
					palette_dirty = 1;
				}
			}
			check_input();
		} else if (key == KEY_ENTER) {
			if (focus != 0) {
				if (selection_row[1] < 6U) {
					selected_tile = track_editor_palette_tile(page,
						selection_row[1], selection_column[1]);
					if (page != 0) {
						multi_tile = (legacy_u8)
							trkObjectList[selected_tile].ss_multiTileFlag;
						if ((multi_tile & 1U) != 0 &&
							selection_row[0] - map_row_offset == 10U)
							selection_row[0]--;
						if ((multi_tile & 2U) != 0 &&
							selection_column[0] - map_column_offset == 11U)
							selection_column[0]--;
					}
					palette_dirty = 1;
					focus = 0;
				} else if (selection_row[1] == 6U) {
					page++;
					if (page > 10U)
						page = 1;
				} else if (selection_row[1] == 7U) {
					dialog_result = LEGACY_S8_FROM_BITS(show_dialog(
						DIALOG_TYPE_MENU, DIALOG_SAVE_BACKGROUND,
						locate_text_res(text_resource, "mss"),
						DIALOG_AUTO_POSITION, DIALOG_AUTO_POSITION,
						dialogarg2, 0,
						td14_elem_map_main[0x384]));
					if (dialog_result != TRACK_EDITOR_DIALOG_CANCELLED &&
						dialog_result != 5U) {
						td14_elem_map_main[0x384] = dialog_result;
						map_dirty = 1;
						track_changed = 1;
					}
				} else if (selection_row[1] == 8U &&
					selection_column[1] != 0) {
					dialog_result = LEGACY_S8_FROM_BITS(show_dialog(
						DIALOG_TYPE_MENU, DIALOG_SAVE_BACKGROUND,
						locate_text_res(text_resource, "men"),
						DIALOG_AUTO_POSITION, DIALOG_AUTO_POSITION,
						dialogarg2, 0, 0));
					if (dialog_result != TRACK_EDITOR_DIALOG_CANCELLED &&
						dialog_result != 5U) {
						for (index = 0; index < 900U; index++)
							td14_elem_map_main[index] = 0;
						terrain_id[0] = 't';
						terrain_id[1] = 'e';
						terrain_id[2] = 'r';
						terrain_id[3] = (legacy_s8)('0' + dialog_result);
						terrain_id[4] = 0;
						terrain_shape = (struct SHAPE2D far*)
							locate_shape_alt(text_resource, terrain_id);
						__fmemcpy(td15_terr_map_main, terrain_shape, 901U);
						gameconfig.game_trackname[0] = 0;
						map_dirty = 1;
						track_changed = 1;
					}
				} else if (selection_row[1] == 8U) {
					result = 1;
					if (track_changed != 0) {
						result = (legacy_s16)show_dialog(DIALOG_TYPE_MENU,
							DIALOG_SAVE_BACKGROUND,
							locate_text_res(text_resource, "chl"),
							DIALOG_AUTO_POSITION, DIALOG_AUTO_POSITION,
							performGraphColor, 0, 0);
					}
					if (result == 0) {
						track_editor_save_track(
							&track_changed, &map_dirty);
					} else {
						g_is_busy = 1;
						map_dirty = 1;
						text = locate_text_res(
							(legacy_s8 far*)mainresptr, "trk");
						result = do_fileselect_dialog(byte_3B80C,
							gameconfig.game_trackname, ".trk", text);
						file_build_path(byte_3B80C,
							gameconfig.game_trackname, ".trk", g_path_buf);
						if (result > 0) {
							file_read_fatal(g_path_buf, td14_elem_map_main);
							track_setup();
							focus = 0;
							selection_column[0] = byte_45D90;
							selection_row[0] = byte_45E16;
							track_changed = 0;
							map_dirty = 1;
						}
						g_is_busy = 0;
					}
				} else if (selection_column[1] == 0) {
					track_editor_save_track(
						&track_changed, &map_dirty);
				} else {
					result = 1;
					if (track_changed != 0) {
						result = (legacy_s16)show_dialog(DIALOG_TYPE_MENU,
							DIALOG_SAVE_BACKGROUND,
							locate_text_res(text_resource, "chx"),
							DIALOG_AUTO_POSITION, DIALOG_AUTO_POSITION,
							performGraphColor, 0, 0);
					}
					if (result == 0)
						track_editor_save_track(
							&track_changed, &map_dirty);
					else
						menu_active = 0;
				}
			} else if (page == 0) {
				if (selection_column[0] == last_column &&
					selection_row[0] == last_row) {
					track_editor_swap_tiles(&selected_tile,
						&saved_tile, &palette_dirty);
				} else {
					source_index = LEGACY_U16_WRAP_ADD(
						(legacy_u16)terrainrows[selection_row[0]],
						selection_column[0]);
					saved_tile = td15_terr_map_main[source_index];
					last_column = selection_column[0];
					last_row = selection_row[0];
				}
				source_index = LEGACY_U16_WRAP_ADD(
					(legacy_u16)terrainrows[last_row], last_column);
				td15_terr_map_main[source_index] = selected_tile;
				track_changed = 1;
				validate_track = 1;
				map_dirty = 1;
			} else {
				multi_tile = (legacy_u8)
					trkObjectList[selected_tile].ss_multiTileFlag;
				if (((multi_tile & 1U) != 0 && selection_row[0] > 28U) ||
					((multi_tile & 2U) != 0 && selection_column[0] > 28U)) {
					palette_dirty = 1;
					map_dirty = 1;
					continue;
				}
				if (selection_column[0] == last_column &&
					selection_row[0] == last_row) {
					track_editor_swap_tiles(&selected_tile,
						&saved_tile, &palette_dirty);
				} else {
					source_index = LEGACY_U16_WRAP_ADD(
						(legacy_u16)trackrows[selection_row[0]],
						selection_column[0]);
					saved_tile = td14_elem_map_main[source_index];
					if (saved_tile >= TRACK_TILE_CONTINUATION_SOUTHEAST)
						saved_tile = 0;
					last_column = selection_column[0];
					last_row = selection_row[0];
				}
				source_index = LEGACY_U16_WRAP_ADD(
					(legacy_u16)trackrows[last_row], last_column);
				td14_elem_map_main[source_index] = selected_tile;
				track_changed = 1;
				validate_track = 1;
				map_dirty = 1;
				if (multi_tile == 1U) {
					td14_elem_map_main[LEGACY_U16_WRAP_ADD(
						(legacy_u16)trackrows[last_row + 1U],
						last_column)] = TRACK_TILE_CONTINUATION_SOUTH;
				} else if (multi_tile == 2U) {
					td14_elem_map_main[LEGACY_U16_WRAP_ADD(
						source_index, 1U)] = TRACK_TILE_CONTINUATION_EAST;
				} else if (multi_tile == 3U) {
					td14_elem_map_main[LEGACY_U16_WRAP_ADD(
						source_index, 1U)] = TRACK_TILE_CONTINUATION_EAST;
					source_index = LEGACY_U16_WRAP_ADD(
						(legacy_u16)trackrows[last_row + 1U], last_column);
					td14_elem_map_main[source_index] = TRACK_TILE_CONTINUATION_SOUTH;
					td14_elem_map_main[LEGACY_U16_WRAP_ADD(
						source_index, 1U)] = TRACK_TILE_CONTINUATION_SOUTHEAST;
				}
			}
		} else if (key == KEY_SPACE || key == KEY_INSERT) {
			focus ^= 1U;
		} else if (key == (legacy_u16)'+') {
			if (page < 10U)
				page++;
		} else if (key == (legacy_u16)'-') {
			if (page > 1U)
				page--;
		} else if (key == KEY_SHIFT_F1) {
			page = 0;
			selected_tile = 0;
		} else if (key == KEY_HOME) {
			if (focus != 0) {
				selection_row[1] = 0;
			} else {
				if (selection_column[0] == map_column_offset &&
					selection_row[0] == map_row_offset) {
					map_column_offset = 0;
					map_row_offset = 0;
				}
				selection_column[0] = map_column_offset;
				selection_row[0] = map_row_offset;
			}
		} else if (key == KEY_UP) {
			if (selection_row[focus] != 0) {
				last_column = TRACK_EDITOR_POSITION_UNSET;
				selection_row[focus]--;
				if (focus != 0 && selection_row[1] < 6U) {
					track_editor_skip_previous_placeholders(page,
						&selection_row[1], &selection_column[1]);
				}
			}
		} else if (key == KEY_DOWN) {
			if (selection_row[focus] < maximum_rows[focus]) {
				last_column = TRACK_EDITOR_POSITION_UNSET;
				selection_row[focus]++;
				if (focus != 0 && selection_row[1] < 6U) {
					value = track_editor_palette_tile(page,
						selection_row[1], selection_column[1]);
					if (value == TRACK_TILE_CONTINUATION_EAST)
						selection_column[1]--;
					else if (value == TRACK_TILE_CONTINUATION_SOUTH)
						selection_row[1]++;
				}
			}
		} else if (key == KEY_LEFT) {
			if (focus != 0 && selection_row[1] == 6U) {
				if (page > 1U)
					page--;
			} else if (selection_column[focus] != 0) {
				last_column = TRACK_EDITOR_POSITION_UNSET;
				selection_column[focus]--;
				if (focus != 0) {
					if (selection_row[1] > 5U) {
						selection_column[1] = 0;
					} else {
						track_editor_skip_previous_placeholders(page,
							&selection_row[1], &selection_column[1]);
					}
				}
			}
		} else if (key == KEY_RIGHT) {
			if (focus != 0 && selection_row[1] == 6U) {
				if (page < 10U)
					page++;
			} else {
				step = focus != 0 && selection_row[1] > 5U ? 3U : 1U;
				while ((legacy_u16)selection_column[focus] + step <
					maximum_columns[focus]) {
					if (focus == 0 || selection_row[1] > 5U)
						break;
					value = track_editor_palette_tile(page,
						selection_row[1],
						(legacy_u8)(selection_column[1] + step));
					if (value < TRACK_TILE_CONTINUATION_SOUTH)
						break;
					if (value == TRACK_TILE_CONTINUATION_EAST)
						step++;
					else
						selection_row[1]--;
				}
				if ((legacy_u16)selection_column[focus] + step <
					maximum_columns[focus]) {
					last_column = TRACK_EDITOR_POSITION_UNSET;
					selection_column[focus] = (legacy_u8)
						(selection_column[focus] + step);
				}
			}
		}

		if (menu_active != 0) {
			palette_dirty = 1;
			map_dirty = 1;
		}
	}

	sprite_free_wnd(render_window_sprite);
	for (index = 4U; index != 0; index--)
		sprite_free_wnd(cursor_sprites[index - 1U]);
	unload_resource(text_resource);
	mmgr_free(shape_resource);
}
