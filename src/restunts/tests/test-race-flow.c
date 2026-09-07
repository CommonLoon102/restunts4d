#include <assert.h>
#include <string.h>

/* Exercise the frame scheduler and viewport policy without a DOS display. */
#include "../c/race.c"

static unsigned updates;
static unsigned analog_updates;
static legacy_u8 joystick_enabled;

legacy_u8 dos_joystick_is_enabled(void)
{
	return joystick_enabled;
}

void replay_apply_analog_steering_history(void)
{
	assert(updates == 0);
	analog_updates++;
}

void update_gamestate(void)
{
	updates++;
}

static void reset_frame(void)
{
	memset(&state, 0, sizeof(state));
	updates = 0;
	analog_updates = 0;
	joystick_enabled = 0;
	mouse_driving_enabled = 0;
	game_replay_mode = REPLAY_MODE_LIVE;
	race_exit_request = 0;
	state.game_inputmode = GAME_INPUT_MODE_ACTIVE;
	state.game_frame = 12;
	elapsed_time2 = 12;
}

static void test_frame_scheduling(void)
{
	legacy_s16 last_frame = -1;

	reset_frame();
	assert(race_frame_is_ready(&last_frame) == 1);
	assert(last_frame == 12);
	assert(race_frame_is_ready(&last_frame) == 0);
	assert(updates == 0);
	race_exit_request = 1;
	assert(race_frame_is_ready(&last_frame) == 1);
	race_exit_request = 0;
	state.game_inputmode = GAME_INPUT_MODE_WAITING;
	assert(race_frame_is_ready(&last_frame) == 1);
	game_replay_mode = REPLAY_MODE_PLAYBACK;
	assert(race_frame_is_ready(&last_frame) == 1);
}

static void test_frame_catchup(void)
{
	legacy_s16 last_frame = -1;

	reset_frame();
	elapsed_time2 = 13;
	mouse_driving_enabled = 1;
	assert(race_frame_is_ready(&last_frame) == 0);
	assert(updates == 1);
	assert(analog_updates == 1);
	assert(last_frame == -1);

	reset_frame();
	elapsed_time2 = 13;
	joystick_enabled = 1;
	assert(race_frame_is_ready(&last_frame) == 0);
	assert(updates == 1);
	assert(analog_updates == 1);

	reset_frame();
	elapsed_time2 = 13;
	game_replay_mode = REPLAY_MODE_PLAYBACK;
	mouse_driving_enabled = 1;
	assert(race_frame_is_ready(&last_frame) == 0);
	assert(updates == 1);
	assert(analog_updates == 0);
}

struct DASHBOARD_CASE {
	legacy_s8 mode;
	legacy_s8 idle;
	legacy_s8 dashboard;
	legacy_s8 following_opponent;
	legacy_s8 replay;
	legacy_s8 replay_bar;
	legacy_s16 expected_bottom;
	legacy_s8 expected_dashboard;
	legacy_s8 expected_bar;
};

static void test_dashboard_layout(void)
{
	static const struct DASHBOARD_CASE cases[] = {
		{REPLAY_MODE_LIVE, 0, 1, 0, 0, 1, 140, 1, 0},
		{REPLAY_MODE_LIVE, 0, 1, 1, 0, 1, 200, 0, 0},
		{REPLAY_MODE_PLAYBACK, 0, 0, 0, 1, 1, 151, 0, 1},
		{REPLAY_MODE_PLAYBACK, 0, 0, 0, 0, 0, 200, 0, 0},
		{REPLAY_MODE_PLAYBACK, 0, 1, 0, 1, 1, 140, 1, 1},
		{REPLAY_MODE_PLAYBACK, 0, 1, 0, 0, 0, 140, 1, 0},
		{REPLAY_MODE_PLAYBACK, 1, 1, 0, 1, 1, 200, 0, 0},
	};
	unsigned index;

	for (index = 0; index < sizeof(cases) / sizeof(cases[0]); index++) {
		game_replay_mode = cases[index].mode;
		idle_expired = cases[index].idle;
		dashb_toggle = cases[index].dashboard;
		followOpponentFlag = cases[index].following_opponent;
		is_in_replay = cases[index].replay;
		replaybar_toggle = cases[index].replay_bar;
		dashbmp_y = 140;
		roofbmpheight = 13;
		height_above_replaybar = 777;
		race_update_dashboard_layout();
		assert(dashbmp_y_copy == cases[index].expected_bottom);
		assert(dashboard_visible == cases[index].expected_dashboard);
		assert(replaybar_enabled == cases[index].expected_bar);
		assert(roofbmpheight_copy == (dashboard_visible ? 13 : 0));
		assert(game_replay_mode_copy == game_replay_mode);
		assert(followOpponentFlag_copy == followOpponentFlag);
		if (dashboard_visible) {
			assert(height_above_replaybar == (replaybar_enabled ? 151 : 200));
		} else {
			assert(height_above_replaybar == 777);
		}
	}
}

int main(void)
{
	test_frame_scheduling();
	test_frame_catchup();
	test_dashboard_layout();
	return 0;
}
