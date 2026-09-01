#include <assert.h>

#include "../c/penaltyroute.h"

static void test_backtracks_from_dead_end_to_nearest_junction(void)
{
	const legacy_s16 next_tracks[] = { 1, 2, 0, 4, -1 };
	const legacy_s16 alternate_tracks[] = { 3, -1, -1, -1, -1 };

	assert(penalty_route_find_backtrack(
		4, 5, next_tracks, alternate_tracks) == 0);
}

static void test_accepts_dead_end_on_primary_route(void)
{
	const legacy_s16 next_tracks[] = { 1, -1, 3, 0 };
	const legacy_s16 alternate_tracks[] = { 2, -1, -1, -1 };

	assert(penalty_route_find_backtrack(
		1, 4, next_tracks, alternate_tracks) == 0);
}

static void test_uses_nearest_of_nested_junctions(void)
{
	const legacy_s16 next_tracks[] = { 1, 2, 0, 4, -1, 6, 3 };
	const legacy_s16 alternate_tracks[] = { 5, 3, -1, -1, -1, -1, -1 };

	assert(penalty_route_find_backtrack(
		4, 7, next_tracks, alternate_tracks) == 1);
}

static void test_stops_when_no_alternate_route_exists(void)
{
	const legacy_s16 next_tracks[] = { 1, 2, -1 };
	const legacy_s16 alternate_tracks[] = { -1, -1, -1 };

	assert(penalty_route_find_backtrack(
		2, 3, next_tracks, alternate_tracks) == -1);
}

static void test_ignores_an_invalid_alternate_route(void)
{
	const legacy_s16 next_tracks[] = { 1, -1 };
	const legacy_s16 alternate_tracks[] = { -1, -1 };

	assert(penalty_route_find_backtrack(
		1, 2, next_tracks, alternate_tracks) == -1);
}

int main(void)
{
	test_backtracks_from_dead_end_to_nearest_junction();
	test_accepts_dead_end_on_primary_route();
	test_uses_nearest_of_nested_junctions();
	test_stops_when_no_alternate_route_exists();
	test_ignores_an_invalid_alternate_route();
	return 0;
}
