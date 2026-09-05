#include "externs.h"
#include "fatal.h"
#include "game_input.h"
#include "keyboard.h"
#include "platform.h"
#include "timing.h"

static legacy_u32 timer_wait_target;
static legacy_s8 input_callback_overflow_message[] =
	"NO ROOM LEFT ON TIMER INTERRUPT ROUTINE LIST\r";

void timer_reg_callback(void (far* callback)(void))
{
	if (dos_timer_register_callback(callback) == 0) {
		fatal_error(input_callback_overflow_message);
	}
}

void timer_remove_callback(void (far* callback)(void))
{
	dos_timer_unregister_callback(callback);
}

legacy_s16 nopsub_30A77(void)
{
	legacy_s16 key;

	do {
		key = kb_call_readchar_callback();
		if (key != 0)
			return key;
	} while (timer_get_counter() < timer_wait_target);
	return 0;
}

legacy_s16 nopsub_30A97(legacy_u32 ticks)
{
	legacy_u32 target;
	legacy_s16 key;

	target = (legacy_u32)(timer_get_counter() + ticks);
	do {
		key = kb_call_readchar_callback();
		if (key != 0)
			return key;
	} while ((legacy_u32)timer_get_counter() < target);
	return 0;
}

legacy_u32 timer_get_delta_alt(void)
{
	return timer_get_delta();
}

legacy_u32 timer_custom_delta(legacy_u32 ticks)
{
	return timer_get_counter() - ticks;
}

void timer_reset()
{
	dos_timer_reset_counter();
}

legacy_u32 timer_copy_counter(legacy_u32 ticks)
{
	timer_wait_target = timer_get_counter() + ticks;
	return timer_wait_target;
}

legacy_u32 timer_wait_for_dx(void)
{
	legacy_u32 res;
	do {
		res = timer_get_counter();
	} while (res < timer_wait_target);

	return res;
}

legacy_s16 timer_compare_dx(void)
{
	return timer_get_counter() >= timer_wait_target;
}

legacy_u32 timer_get_counter_unk(legacy_u32 ticks)
{
	legacy_u32 target, res;
	target = timer_get_counter() + ticks;

	do {
		res = timer_get_counter();
	} while (res < target);

	return res;
}

static legacy_u32 secondary_timer_target(void)
{
	return ((legacy_u32)word_3F1C4 << LEGACY_WORD_BITS) | word_3F1C2;
}

static legacy_s16 secondary_timer_target_reached(
	legacy_u32 current,
	legacy_u32 target
) {
	return (legacy_u16)(current >> LEGACY_WORD_BITS) >=
		(legacy_u16)(target >> LEGACY_WORD_BITS) &&
		(legacy_u16)current >= (legacy_u16)target;
}

legacy_u32 set_add_value(legacy_u32 ticks)
{
	legacy_u32 target;

	target = (legacy_u32)(timer_get_slow_counter() + ticks);
	word_3F1C2 = (legacy_u16)target;
	word_3F1C4 = (legacy_u16)(target >> LEGACY_WORD_BITS);
	return target;
}

legacy_s16 sub_2EB07(void)
{
	return secondary_timer_target_reached(
		timer_get_slow_counter(), secondary_timer_target());
}

legacy_u32 sub_2EB1E(legacy_u32 ticks)
{
	legacy_u32 current;
	legacy_u32 target;

	target = (legacy_u32)(timer_get_slow_counter() + ticks);
	do {
		current = timer_get_slow_counter();
	} while (!secondary_timer_target_reached(current, target));
	return current;
}
