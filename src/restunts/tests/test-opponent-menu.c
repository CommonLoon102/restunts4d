#include <string.h>

/* Reuse the car-menu drawing fixture, replacing input and ownership hooks
 * so this test drives the real opponent menu and checks each refresh. */
#define main car_snapshot_main
#define input_checking car_fixture_input_checking
#define mouse_multi_hittest car_fixture_mouse_multi_hittest
#define locate_text_res car_fixture_locate_text_res
#define file_load_resfile car_fixture_file_load_resfile
#define unload_resource car_fixture_unload_resource
#define mmgr_free car_fixture_mmgr_free
#define sprite_make_wnd car_fixture_sprite_make_wnd
#define sprite_free_wnd car_fixture_sprite_free_wnd
#include "test-car-menu.c"
#undef main
#undef input_checking
#undef mouse_multi_hittest
#undef locate_text_res
#undef file_load_resfile
#undef unload_resource
#undef mmgr_free
#undef sprite_make_wnd
#undef sprite_free_wnd
#undef memcpy

#define OPPONENT_TEST_EVENT_CAPACITY 32U
#define OPPONENT_TEST_RESOURCE_COUNT 3U

static const legacy_u8 previous_opponent[7] = {6, 6, 1, 2, 3, 4, 5};
static const legacy_u8 next_opponent[7] = {1, 2, 3, 4, 5, 6, 1};
static legacy_u16 opponent_keys[OPPONENT_TEST_EVENT_CAPACITY];
static legacy_u8 expected_opponents[OPPONENT_TEST_EVENT_CAPACITY];
static legacy_u8 expected_loads[OPPONENT_TEST_EVENT_CAPACITY];
static legacy_u8 resource_live[OPPONENT_TEST_RESOURCE_COUNT];
static unsigned event_count, event_index, expected_load_count, load_count;
static unsigned resource_allocations, resource_releases, window_allocations, window_releases;
static unsigned case_count, transition_count;
static legacy_u8 window_live;

legacy_s16 input_checking(legacy_s16 elapsed)
{
	(void)elapsed;
	assert(event_index < event_count);
	assert((legacy_u8)gameconfig.game_opponenttype == expected_opponents[event_index]);
	return (legacy_s16)opponent_keys[event_index];
}

legacy_s16 mouse_multi_hittest(legacy_s16 count, const struct BUTTON_AREA *buttons)
{
	assert(count == 5 && buttons == opponentmenu_buttons);
	assert(event_index < event_count);
	event_index++;
	/* No hover overrides: Enter really activates the initially selected Last. */
	return -1;
}

static void *allocate_resource(unsigned index)
{
	assert(index < OPPONENT_TEST_RESOURCE_COUNT && resource_live[index] == 0);
	resource_live[index] = 1;
	resource_allocations++;
	return resource_bytes[index];
}

static void release_resource(void *resource)
{
	unsigned index;
	for (index = 0; index < OPPONENT_TEST_RESOURCE_COUNT; index++) {
		if (resource == resource_bytes[index]) {
			assert(resource_live[index] != 0);
			resource_live[index] = 0;
			resource_releases++;
			return;
		}
	}
	assert(!"Opponent menu released an unknown resource");
}

void *file_load_resfile(const legacy_s8 *filename)
{
	if (_strcmp(filename, opponent_misc_resource_name) == 0) {
		return allocate_resource(0);
	}
	/* A wrap failure used to request opp/ rather than the sixth opponent. */
	if (filename[0] != 'o' || filename[1] != 'p' || filename[2] != 'p' || filename[3] < '1' ||
		filename[3] > '6' || filename[4] != 0) {
		fprintf(stderr, "Invalid opponent resource requested: %s\n", (const char *)filename);
		assert(!"Opponent resource must be opp1 through opp6");
	}
	assert(load_count < expected_load_count);
	assert((legacy_u8)(filename[3] - '0') == expected_loads[load_count++]);
	assert(filename[3] - '0' == gameconfig.game_opponenttype);
	return allocate_resource(2);
}

void *file_load_resource(legacy_s16 type, const legacy_s8 *name)
{
	assert(type == FILE_RESOURCE_SHAPE2D_ALTERNATE);
	assert(_strcmp(name, opponent_menu_shapes_name) == 0);
	return allocate_resource(1);
}

void unload_resource(void *resource)
{
	release_resource(resource);
}

void *mmgr_free(legacy_s8 *resource)
{
	assert(resource == (legacy_s8 *)resource_bytes[1]);
	release_resource(resource);
	return 0;
}

struct SPRITE *sprite_make_wnd(legacy_u16 width, legacy_u16 height, legacy_u16 color)
{
	assert(width == 320 && height == 200 && color == 15);
	assert(window_live == 0);
	window_live = 1;
	window_allocations++;
	fixture_sprites[0].sprite_bitmapptr = &fixture_shapes[0];
	return &fixture_sprites[0];
}

void sprite_free_wnd(struct SPRITE *window)
{
	assert(window == &fixture_sprites[0] && window_live != 0);
	window_live = 0;
	window_releases++;
}

legacy_s8 *locate_text_res(legacy_s8 *resource, const legacy_s8 *name)
{
	if (_strcmp(name, opponent_description_id) == 0) {
		assert(resource == (legacy_s8 *)resource_bytes[2] && resource_live[2] != 0);
		return (legacy_s8 *)"Opponent]";
	}
	assert(resource == (legacy_s8 *)resource_bytes[0] && resource_live[0] != 0);
	return _strcmp(name, opponent_racing_car_label_id) == 0 ? (legacy_s8 *)"Clock]"
															: (legacy_s8 *)name;
}

void locate_many_resources(legacy_s8 *resource, const legacy_s8 *names, legacy_s8 **pointers)
{
	unsigned index;
	assert(resource == (legacy_s8 *)resource_bytes[1]);
	assert(names == opponent_portrait_shape_ids);
	for (index = 0; index < 7; index++) {
		pointers[index] = (legacy_s8 *)&fixture_shapes[1];
	}
}

void sprite_draw_palette_mapped(struct SHAPE2D *shape)
{
	assert(shape == &fixture_shapes[0] || shape == &fixture_shapes[1]);
	assert((legacy_u8)gameconfig.game_opponenttype <= 6);
}

void check_input(void)
{
}

void show_waiting(void)
{
}

void run_car_menu(legacy_s8 *id, legacy_s8 *material, legacy_s8 *transmission, legacy_u16 opponent)
{
	(void)id;
	(void)material;
	(void)transmission;
	(void)opponent;
	assert(!"Navigation unexpectedly entered the car menu");
}

static void expect_load(legacy_u8 opponent)
{
	if (opponent != 0) {
		assert(expected_load_count < OPPONENT_TEST_EVENT_CAPACITY);
		expected_loads[expected_load_count++] = opponent;
	}
}

static void add_event(legacy_u16 key, legacy_u8 opponent)
{
	assert(event_count < OPPONENT_TEST_EVENT_CAPACITY);
	opponent_keys[event_count] = key;
	expected_opponents[event_count++] = opponent;
}

static void begin_case(legacy_u8 opponent, legacy_u8 page_flipping)
{
	assert(resource_allocations == resource_releases && window_allocations == window_releases);
	memset(resource_live, 0, sizeof(resource_live));
	memset(&gameconfig, 0, sizeof(gameconfig));
	memcpy(gameconfig.game_playercarid, "COUN", 4);
	gameconfig.game_opponentcarid[0] = -1;
	gameconfig.game_opponenttype = (legacy_s8)opponent;
	gameconfig.game_playermaterial = 3;
	video_uses_page_flipping = page_flipping;
	fontnptr = (legacy_s8 *)resource_bytes[62];
	font_glyph_height = 8;
	event_count = event_index = expected_load_count = load_count = 0;
	resource_allocations = resource_releases = window_allocations = window_releases = 0;
	expect_load(opponent);
}

static void finish_case(legacy_u8 opponent, unsigned refresh_count)
{
	unsigned index;
	run_opponent_menu();
	assert(event_index == event_count && load_count == expected_load_count);
	assert((legacy_u8)gameconfig.game_opponenttype == opponent);
	assert(resource_allocations == resource_releases && window_allocations == window_releases);
	assert(resource_allocations == expected_load_count + 2 && window_allocations == refresh_count);
	assert(window_live == 0);
	for (index = 0; index < OPPONENT_TEST_RESOURCE_COUNT; index++) {
		assert(resource_live[index] == 0);
	}
	if (opponent != 0) {
		assert(memcmp(gameconfig.game_opponentcarid, "COUN", 4) == 0);
		assert(gameconfig.game_opponentmaterial == 0);
		assert(gameconfig.game_opponenttransmission == TRANSMISSION_MANUAL);
	} else {
		assert(gameconfig.game_opponentcarid[0] == -1);
	}
	case_count++;
}

static void test_direction(legacy_u8 initial, legacy_u8 direction, unsigned repeats,
						   legacy_u8 page_flipping)
{
	legacy_u8 opponent = initial;
	unsigned index;
	begin_case(initial, page_flipping);
	if (direction != 0) {
		add_event(KEY_RIGHT, opponent);
	}
	for (index = 0; index < repeats; index++) {
		add_event(KEY_ENTER, opponent);
		opponent = direction != 0 ? next_opponent[opponent] : previous_opponent[opponent];
		expect_load(opponent);
		transition_count++;
	}
	/* Navigate from Last/Next to Done, keeping hover disabled throughout. */
	add_event(KEY_LEFT, opponent);
	if (direction != 0) {
		add_event(KEY_LEFT, opponent);
	}
	add_event(KEY_ENTER, opponent);
	finish_case(opponent, repeats + 1);
}

static void test_return_to_clock(legacy_u8 page_flipping)
{
	begin_case(6, page_flipping);
	add_event(KEY_RIGHT, 6);
	add_event(KEY_RIGHT, 6);
	add_event(KEY_ENTER, 6); /* None unloads the existing opponent. */
	add_event(KEY_LEFT, 0);
	add_event(KEY_LEFT, 0);
	add_event(KEY_ENTER, 0); /* Last after None must select the sixth opponent. */
	expect_load(6);
	add_event(KEY_RIGHT, 6);
	add_event(KEY_RIGHT, 6);
	add_event(KEY_ENTER, 6);
	add_event(KEY_RIGHT, 0); /* Disabled Car is skipped on the way to Done. */
	add_event(KEY_ENTER, 0);
	finish_case(0, 4);
	transition_count += 3;
}

int main(void)
{
	legacy_u8 initial, direction, page_flipping;
	for (page_flipping = 0; page_flipping < 2; page_flipping++) {
		for (initial = 0; initial < 7; initial++) {
			for (direction = 0; direction < 2; direction++) {
				test_direction(initial, direction, 1, page_flipping);
				test_direction(initial, direction, 19, page_flipping);
			}
		}
		test_return_to_clock(page_flipping);
	}
	printf("test-opponent-menu: passed %u sessions, %u transitions\n", case_count,
		   transition_count);
	return 0;
}
