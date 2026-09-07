#include <string.h>
#define main car_snapshot_main
#define input_checking car_fixture_input_checking
#define mouse_multi_hittest car_fixture_mouse_multi_hittest
#define locate_text_res car_fixture_locate_text_res
#define menu_update_idle_counter car_fixture_menu_update_idle_counter
#include "test-car-menu.c"
#undef main
#undef input_checking
#undef mouse_multi_hittest
#undef locate_text_res
#undef menu_update_idle_counter
#include "../c/highscore.h"
#include "../c/skybox.h"

legacy_s16 ranking_entry_order[HIGHSCORE_ENTRY_COUNT];

void copy_string(legacy_s8 *destination, legacy_s8 *source)
{
	_strcpy(destination, source);
}

static legacy_u16 menu_keys[16];
static legacy_s16 menu_hits[16];
static unsigned int active_menu;
static legacy_u8 menu_track_map[1802];
static struct HIGHSCORE_ENTRY menu_scores[8];

legacy_s16 input_checking(legacy_s16 delta)
{
	trace_word(2000);
	trace_word(delta);
	assert(frame_index < 16U);
	return menu_keys[frame_index];
}
legacy_s16 mouse_multi_hittest(legacy_s16 count, const struct BUTTON_AREA *buttons)
{
	trace_word(2001);
	trace_word(count);
	trace_pointer(buttons);
	return menu_hits[frame_index++];
}
legacy_s8 *locate_text_res(legacy_s8 *resource, const legacy_s8 *name)
{
	trace_word(2002);
	trace_pointer(resource);
	trace_text(name);
	if (_strcmp(name, opponent_description_id) == 0 ||
		_strcmp(name, opponent_racing_car_label_id) == 0) {
		return (legacy_s8 *)"First]Second]]Third]";
	}
	return (legacy_s8 *)name;
}
void menu_update_idle_counter(legacy_u16 elapsed, legacy_s16 limit)
{
	trace_word(2003);
	trace_word(elapsed);
	trace_word(limit);
	if (active_menu == 1U && scenario % 6U == 5U) {
		idle_expired = 1;
	}
}
void check_input(void)
{
	trace_word(2004);
}
void show_waiting(void)
{
	trace_word(2005);
	trace_word(waitflag);
}
void *file_load_resource(legacy_s16 type, const legacy_s8 *name)
{
	trace_word(2006);
	trace_word(type);
	return file_load_resfile(name);
}
void locate_many_resources(legacy_s8 *resource, const legacy_s8 *names, legacy_s8 **pointers)
{
	unsigned int i;
	trace_word(2007);
	trace_pointer(resource);
	trace_text(names);
	for (i = 0; i < 7; i++) {
		pointers[i] = (legacy_s8 *)&fixture_shapes[1];
	}
}
void sprite_draw_palette_mapped(struct SHAPE2D *shape)
{
	trace_word(2008);
	trace_pointer(shape);
}
void run_car_menu(legacy_s8 *id, legacy_s8 *material, legacy_s8 *transmission, legacy_u16 opponent)
{
	unsigned int i;
	trace_word(2009);
	trace_word(opponent);
	for (i = 0; i < 4U; i++) {
		trace_word((legacy_u8)id[i]);
		id[i] = "VETT"[i];
	}
	*material = 2;
	*transmission = 1;
}
legacy_s8 *_strcat(legacy_s8 *destination, const legacy_s8 *source)
{
	legacy_s8 *end = destination;
	while (*end != 0) {
		end++;
	}
	_strcpy(end, source);
	return destination;
}
legacy_s8 *locate_shape_alt(legacy_s8 *resource, const legacy_s8 *name)
{
	trace_word(2010);
	trace_pointer(resource);
	trace_text(name);
	return (legacy_s8 *)name;
}
struct RECTANGLE *intro_draw_text(legacy_s8 *text, legacy_s16 x, legacy_s16 y, legacy_s16 color,
								  legacy_s16 flag)
{
	static struct RECTANGLE rectangle;
	trace_word(2011);
	trace_text(text);
	trace_word(x);
	trace_word(y);
	trace_word(color);
	trace_word(flag);
	return &rectangle;
}
legacy_s16 font_centered_text_x(const legacy_s8 *text)
{
	trace_word(2012);
	trace_text(text);
	return 40;
}
legacy_s16 track_setup(void)
{
	trace_word(2013);
	return 0;
}
void load_tracks_menu_shapes(void)
{
	trace_word(2014);
}
void load_skybox(legacy_s8 index)
{
	trace_word(2015);
	trace_word(index);
}
void unload_skybox(void)
{
	trace_word(2016);
}
legacy_s16 shape3d_load_all(void)
{
	trace_word(2017);
	return 0;
}
void shape3d_free_all(void)
{
	trace_word(2018);
}
void draw_track_preview(void)
{
	trace_word(2019);
}
legacy_s16 highscore_load_or_create(legacy_s16 create)
{
	trace_word(2020);
	trace_word(create);
	track_highscore_table = (legacy_s8 *)menu_scores;
	ranking_entry_order[0] = 0;
	menu_scores[0].time = scenario % 2U == 0 ? 65535U : 123U;
	return scenario % 3U == 0;
}
void print_highscore_entry(legacy_s16 entry, legacy_u8 *offsets)
{
	unsigned int i;
	trace_word(2021);
	trace_word(entry);
	for (i = 0; i < 4; i++) {
		offsets[i] = i * 2;
		*(&resID_byte1 + i * 2) = 'a' + i;
		*(&resID_byte1 + i * 2 + 1) = 0;
	}
}
legacy_s8 do_fileselect_dialog(legacy_s8 *directory, legacy_s8 *name, legacy_s8 *extension,
							   legacy_s8 *prompt)
{
	trace_word(2022);
	trace_text(directory);
	trace_text(name);
	trace_text(extension);
	trace_text(prompt);
	return scenario % 2U;
}
void file_build_path(const legacy_s8 *directory, const legacy_s8 *name, const legacy_s8 *extension,
					 legacy_s8 *destination)
{
	trace_word(2023);
	trace_text(directory);
	trace_text(name);
	trace_text(extension);
	_strcpy(destination, (legacy_s8 *)"fixture.trk");
}
void *file_read_fatal(const legacy_s8 *name, void *destination)
{
	trace_word(2024);
	trace_text(name);
	assert(destination == menu_track_map);
	return destination;
}

static void reset_menu_case(unsigned int index, unsigned int kind)
{
	unsigned int i;
	scenario = index;
	active_menu = kind;
	frame_index = allocation_index = sprite_index = 0;
	idle_expired = 0;
	video_uses_page_flipping = index % 2U;
	fontnptr = (legacy_s8 *)resource_bytes[62];
	font_glyph_height = 5 + index % 4U;
	memset(&gameconfig, 0, sizeof(gameconfig));
	gameconfig.game_playercarid[0] = 'C';
	gameconfig.game_playercarid[1] = 'O';
	gameconfig.game_playercarid[2] = 'U';
	gameconfig.game_playercarid[3] = 'N';
	_strcpy(gameconfig.game_trackname, (legacy_s8 *)"DEFAULT");
	gameconfig.game_playermaterial = index % 8U;
	gameconfig.game_opponenttype = index % 7U;
	gameconfig.game_opponentcarid[0] = -1;
	if (index % 2U) {
		for (i = 0; i < 4U; i++) {
			gameconfig.game_opponentcarid[i] = "VETT"[i];
		}
	}
	if (kind == 0U && index % 6U == 1U && gameconfig.game_opponenttype == 0) {
		gameconfig.game_opponenttype = 1;
	}
	track_element_map = menu_track_map;
	menu_track_map[900] = index % 5U;
	for (i = 0; i < 16; i++) {
		menu_keys[i] = KEY_ENTER;
		menu_hits[i] = kind == 0 ? 4 : 2;
	}
	trace_word(kind);
	trace_word(index);
}

static void test_opponent_navigation(void)
{
	unsigned int index, i;
	for (index = 0; index < 42U; index++) {
		reset_menu_case(index, 0);
		switch (index % 6U) {
			case 0:
				menu_keys[0] = KEY_ESCAPE;
				break;
			case 1:
				menu_hits[0] = 0;
				menu_hits[1] = 1;
				break;
			case 2:
				menu_hits[0] = 2;
				menu_keys[0] = KEY_SPACE;
				menu_hits[1] = 3;
				break;
			case 3:
				menu_hits[0] = 3;
				if (gameconfig.game_opponenttype == 0) {
					menu_keys[0] = 0;
				}
				break;
			case 4:
				menu_keys[0] = KEY_LEFT;
				menu_hits[0] = -1;
				menu_keys[1] = KEY_LEFT;
				menu_hits[1] = -1;
				menu_keys[2] = KEY_SPACE;
				menu_hits[2] = -1;
				break;
			case 5:
				menu_keys[0] = KEY_RIGHT;
				menu_hits[0] = -1;
				menu_keys[1] = KEY_RIGHT;
				menu_hits[1] = -1;
				menu_keys[2] = KEY_RIGHT;
				menu_hits[2] = -1;
				menu_keys[3] = KEY_ESCAPE;
				menu_hits[3] = -1;
				break;
		}
		run_opponent_menu();
		trace_word(gameconfig.game_opponenttype);
		for (i = 0; i < 4U; i++) {
			trace_word((legacy_u8)gameconfig.game_opponentcarid[i]);
		}
		trace_word(gameconfig.game_opponentmaterial);
		trace_word(gameconfig.game_opponenttransmission);
	}
}

static void test_track_navigation(void)
{
	unsigned int index;
	for (index = 0; index < 18U; index++) {
		reset_menu_case(index, 1);
		switch (index % 6U) {
			case 0:
				menu_keys[0] = KEY_ESCAPE;
				menu_hits[0] = -1;
				break;
			case 1:
				menu_hits[0] = 0;
				break;
			case 2:
				menu_hits[0] = 1;
				break;
			case 3:
				menu_keys[0] = KEY_LEFT;
				menu_hits[0] = -1;
				menu_hits[1] = -1;
				break;
			case 4:
				menu_keys[0] = KEY_RIGHT;
				menu_hits[0] = -1;
				menu_keys[1] = KEY_RIGHT;
				menu_hits[1] = -1;
				menu_hits[2] = -1;
				break;
		}
		run_tracks_menu(index % 2U);
		trace_word(waitflag);
		trace_word(idle_expired);
	}
}

int main(void)
{
	test_opponent_navigation();
	test_track_navigation();
	assert(trace_hash == UINT64_C(0x8a70343f496ab350));
	printf("test-menu-navigation: passed\n");
	return 0;
}
