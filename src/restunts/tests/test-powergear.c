#include <assert.h>

#include "../c/externs.h"

legacy_s16 powergear_bug_enabled = 1;

extern legacy_s16 scale_acceleration_by_mass_for_test(
	legacy_s16 acceleration, legacy_s16 mass);

static void test_command_line_option(void)
{
	legacy_s8 program[] = "RESTUNTS";
	legacy_s8 disable[] = "/PG:OFF";
	legacy_s8 enable[] = "/pg:on";
	legacy_s8 unknown[] = "/pg:unknown";
	legacy_s8* disable_arguments[] = { program, disable };
	legacy_s8* enable_arguments[] = { program, enable };
	legacy_s8* unknown_arguments[] = { program, unknown };
	legacy_s8* ordered_arguments[] = { program, disable, enable };

	configure_powergear_bug(2, disable_arguments);
	assert(powergear_bug_enabled == 0);
	configure_powergear_bug(2, enable_arguments);
	assert(powergear_bug_enabled == 1);
	configure_powergear_bug(2, unknown_arguments);
	assert(powergear_bug_enabled == 1);
	configure_powergear_bug(3, ordered_arguments);
	assert(powergear_bug_enabled == 1);
}

static void test_mass_scaling_modes(void)
{
	powergear_bug_enabled = 1;
	assert(scale_acceleration_by_mass_for_test(-512, 40) == 12947);
	assert(scale_acceleration_by_mass_for_test(-512, 32) == -200);

	powergear_bug_enabled = 0;
	assert(scale_acceleration_by_mass_for_test(-512, 40) == -160);
	assert(scale_acceleration_by_mass_for_test(-512, 32) == -200);
	assert(scale_acceleration_by_mass_for_test(512, 40) == 160);
}

int main(void)
{
	test_command_line_option();
	test_mass_scaling_modes();
	return 0;
}
