#include <assert.h>

#include "../c/externs.h"

static void test_default_is_legacy(void)
{
	legacy_s8 program[] = "restunts";
	legacy_s8* arguments[] = { program };

	legacy_penalty_route_enabled = 0;
	parse_penalty_route_mode(1, arguments);
	assert(legacy_penalty_route_enabled == 1);
}

static void test_new_penalty_switch(void)
{
	legacy_s8 program[] = "restunts";
	legacy_s8 option[] = "/NEWPENALTY";
	legacy_s8* arguments[] = { program, option };

	parse_penalty_route_mode(2, arguments);
	assert(legacy_penalty_route_enabled == 0);
}

static void test_explicit_legacy_switch(void)
{
	legacy_s8 program[] = "restunts";
	legacy_s8 new_option[] = "/newpenalty";
	legacy_s8 legacy_option[] = "/legacypenalty";
	legacy_s8* arguments[] = { program, new_option, legacy_option };

	parse_penalty_route_mode(3, arguments);
	assert(legacy_penalty_route_enabled == 1);
}

int main(void)
{
	test_default_is_legacy();
	test_new_penalty_switch();
	test_explicit_legacy_switch();
	return 0;
}
