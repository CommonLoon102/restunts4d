#include <dos.h>
#include "../../c/platform.h"

#define DOS_JOYSTICK_GAME_PORT 513
#define DOS_JOYSTICK_AXIS_INPUT_MASK 3
#define DOS_JOYSTICK_AXIS_INITIAL_VALUE 80
#define DOS_JOYSTICK_SAMPLE_DELAY 20
#define DOS_JOYSTICK_SAMPLE_TIMEOUT 4000
#define DOS_JOYSTICK_AXIS1_INPUT_BIT 1
#define DOS_JOYSTICK_AXIS2_INPUT_BIT 2
#define DOS_JOYSTICK_AXIS_SCALE_FACTOR 16384U
#define DOS_JOYSTICK_CALIBRATION_TICKS 20U
#define DOS_JOYSTICK_HIGH_CANDIDATE 20000U
#define DOS_JOYSTICK_SCALE_SHIFT LEGACY_BYTE_BITS
#define DOS_JOYSTICK_SCALED_CENTER_OFFSET 31U
#define DOS_JOYSTICK_ENABLED_BIT 1U
#define DOS_JOYSTICK_AXIS1_LOW_FLAG 8U
#define DOS_JOYSTICK_AXIS1_HIGH_FLAG 4U
#define DOS_JOYSTICK_AXIS2_LOW_FLAG 1U
#define DOS_JOYSTICK_AXIS2_HIGH_FLAG 2U
#define DOS_JOYSTICK_BUTTON_BITS 48U

static legacy_u8 dos_joystick_enabled;
static legacy_u16 dos_joystick_axis1;
static legacy_u16 dos_joystick_axis2;
static legacy_u16 dos_joystick_axis1_min =
	DOS_JOYSTICK_AXIS_INITIAL_VALUE;
static legacy_u16 dos_joystick_axis1_max;
static legacy_u16 dos_joystick_axis2_min =
	DOS_JOYSTICK_AXIS_INITIAL_VALUE;
static legacy_u16 dos_joystick_axis2_max;
static legacy_u16 dos_joystick_axis1_scale;
static legacy_u16 dos_joystick_axis2_scale;

static legacy_u8 dos_joystick_button_mask;
static legacy_u8 dos_joystick_input;
static legacy_u16 dos_joystick_axis1_low_candidate;
static legacy_u16 dos_joystick_axis1_high_candidate;
static legacy_u16 dos_joystick_axis1_candidate_ticks =
	DOS_JOYSTICK_CALIBRATION_TICKS;
static legacy_u16 dos_joystick_axis1_low_threshold;
static legacy_u16 dos_joystick_axis1_high_threshold;
static legacy_u16 dos_joystick_axis2_low_candidate;
static legacy_u16 dos_joystick_axis2_high_candidate;
static legacy_u16 dos_joystick_axis2_candidate_ticks =
	DOS_JOYSTICK_CALIBRATION_TICKS;
static legacy_u16 dos_joystick_axis2_low_threshold;
static legacy_u16 dos_joystick_axis2_high_threshold;

static void dos_sample_joystick_axes(void)
{
	/* The discharge loop is deliberately kept as one instruction-timed DOS
	 * block.  Moving port reads into an ordinary C loop changes calibration. */
	__asm {
		mov     dos_joystick_input, 0
		mov     dx, DOS_JOYSTICK_GAME_PORT
		in      al, dx
		mov     dos_joystick_button_mask, al
		mov     bl, DOS_JOYSTICK_AXIS_INPUT_MASK
		mov     dos_joystick_axis1, DOS_JOYSTICK_AXIS_INITIAL_VALUE
		mov     dos_joystick_axis2, DOS_JOYSTICK_AXIS_INITIAL_VALUE
		cli
		out     dx, al
		mov     cx, DOS_JOYSTICK_SAMPLE_DELAY
	joy_sample_delay:
		loop    joy_sample_delay
		xor     cx, cx
	joy_sample_loop:
		in      al, dx
		and     al, bl
		xor     al, bl
		jnz     joy_sample_axis_done
		inc     cx
		cmp     cx, DOS_JOYSTICK_SAMPLE_TIMEOUT
		jl      joy_sample_loop
		jmp     joy_sample_complete
	joy_sample_axis_done:
		test    al, DOS_JOYSTICK_AXIS1_INPUT_BIT
		jnz     joy_sample_axis1_done
	joy_sample_axis2_test:
		test    al, DOS_JOYSTICK_AXIS2_INPUT_BIT
		jz      joy_sample_loop
		mov     dos_joystick_axis2, cx
		and     bl, DOS_JOYSTICK_AXIS1_INPUT_BIT
		jnz     joy_sample_loop
		jmp     joy_sample_complete
	joy_sample_axis1_done:
		mov     dos_joystick_axis1, cx
		and     bl, DOS_JOYSTICK_AXIS2_INPUT_BIT
		jnz     joy_sample_axis2_test
	joy_sample_complete:
		sti
	}
}

static void joystick_recalculate_axis1(void)
{
	legacy_u16 range;
	legacy_u16 half;
	legacy_u16 quarter;

	range = LEGACY_U16_WRAP_SUB(dos_joystick_axis1_max, dos_joystick_axis1_min);
	if (LEGACY_S16_FROM_BITS(range) > 0)
		dos_joystick_axis1_scale = (legacy_u16)(
			DOS_JOYSTICK_AXIS_SCALE_FACTOR / range);
	half = range >> 1;
	quarter = half >> 1;
	dos_joystick_axis1_high_threshold = LEGACY_U16_WRAP_ADD(
		LEGACY_U16_WRAP_ADD(dos_joystick_axis1_min, half), quarter);
	dos_joystick_axis1_low_threshold = LEGACY_U16_WRAP_SUB(
		LEGACY_U16_WRAP_SUB(dos_joystick_axis1_high_threshold, quarter), quarter);
}

static void joystick_recalculate_axis2(void)
{
	legacy_u16 range;
	legacy_u16 half;
	legacy_u16 quarter;

	range = LEGACY_U16_WRAP_SUB(dos_joystick_axis2_max, dos_joystick_axis2_min);
	if (LEGACY_S16_FROM_BITS(range) > 0)
		dos_joystick_axis2_scale = (legacy_u16)(
			DOS_JOYSTICK_AXIS_SCALE_FACTOR / range);
	half = range >> 1;
	quarter = half >> 1;
	dos_joystick_axis2_high_threshold = LEGACY_U16_WRAP_ADD(
		LEGACY_U16_WRAP_ADD(dos_joystick_axis2_min, half), quarter);
	dos_joystick_axis2_low_threshold = LEGACY_U16_WRAP_SUB(
		LEGACY_U16_WRAP_SUB(dos_joystick_axis2_high_threshold, quarter), quarter);
}

static void joystick_reset_axis1_candidates(void)
{
	dos_joystick_axis1_candidate_ticks = DOS_JOYSTICK_CALIBRATION_TICKS;
	dos_joystick_axis1_high_candidate = DOS_JOYSTICK_HIGH_CANDIDATE;
	dos_joystick_axis1_low_candidate = 0;
}

static void joystick_reset_axis2_candidates(void)
{
	dos_joystick_axis2_candidate_ticks = DOS_JOYSTICK_CALIBRATION_TICKS;
	dos_joystick_axis2_high_candidate = DOS_JOYSTICK_HIGH_CANDIDATE;
	dos_joystick_axis2_low_candidate = 0;
}

void dos_joystick_reset_calibration(void)
{
	dos_joystick_enabled = 1;
	dos_joystick_axis1_min = DOS_JOYSTICK_AXIS_INITIAL_VALUE;
	dos_joystick_axis1_max = 0;
	dos_joystick_axis2_min = DOS_JOYSTICK_AXIS_INITIAL_VALUE;
	dos_joystick_axis2_max = 0;
}

void dos_joystick_set_enabled(legacy_u8 enabled)
{
	dos_joystick_enabled = enabled;
}

legacy_u8 dos_joystick_is_enabled(void)
{
	return dos_joystick_enabled;
}

legacy_s16 dos_joystick_get_scaled_axis(legacy_u16 axis_index)
{
	legacy_u16 axis;
	legacy_u16 minimum;
	legacy_u16 scale;
	legacy_u16 difference;
	legacy_u32 scaled;

	if (axis_index == 0U) {
		axis = dos_joystick_axis1;
		minimum = dos_joystick_axis1_min;
		scale = dos_joystick_axis1_scale;
	} else {
		axis = dos_joystick_axis2;
		minimum = dos_joystick_axis2_min;
		scale = dos_joystick_axis2_scale;
	}
	if (LEGACY_S16_FROM_BITS(axis) < LEGACY_S16_FROM_BITS(minimum))
		difference = 0;
	else
		difference = LEGACY_U16_WRAP_SUB(axis, minimum);
	scaled = (legacy_u32)difference * scale;
	return LEGACY_S16_FROM_BITS(
		(legacy_u16)((legacy_u16)(scaled >> DOS_JOYSTICK_SCALE_SHIFT) -
		DOS_JOYSTICK_SCALED_CENTER_OFFSET));
}

legacy_s16 dos_get_joy_flags(void)
{
	legacy_u16 axis;
	legacy_u8 buttons;

	if ((dos_joystick_enabled & DOS_JOYSTICK_ENABLED_BIT) == 0)
		return 0;

	dos_sample_joystick_axes();

	axis = dos_joystick_axis1;
	if (LEGACY_S16_FROM_BITS(axis) < LEGACY_S16_FROM_BITS(dos_joystick_axis1_min)) {
		dos_joystick_axis1_candidate_ticks = LEGACY_U16_WRAP_SUB(dos_joystick_axis1_candidate_ticks, 1U);
		if (LEGACY_S16_FROM_BITS(dos_joystick_axis1_candidate_ticks) <= 0) {
			dos_joystick_axis1_min = dos_joystick_axis1_low_candidate;
			joystick_recalculate_axis1();
			joystick_reset_axis1_candidates();
		} else if (LEGACY_S16_FROM_BITS(axis) >=
			LEGACY_S16_FROM_BITS(dos_joystick_axis1_low_candidate)) {
			dos_joystick_axis1_low_candidate = axis;
		}
	} else if (LEGACY_S16_FROM_BITS(axis) >
		LEGACY_S16_FROM_BITS(dos_joystick_axis1_max)) {
		dos_joystick_axis1_candidate_ticks = LEGACY_U16_WRAP_SUB(dos_joystick_axis1_candidate_ticks, 1U);
		if (LEGACY_S16_FROM_BITS(dos_joystick_axis1_candidate_ticks) <= 0) {
			dos_joystick_axis1_max = dos_joystick_axis1_high_candidate;
			joystick_recalculate_axis1();
			joystick_reset_axis1_candidates();
		} else if (LEGACY_S16_FROM_BITS(axis) <
			LEGACY_S16_FROM_BITS(dos_joystick_axis1_high_candidate)) {
			dos_joystick_axis1_high_candidate = axis;
		}
	} else {
		joystick_reset_axis1_candidates();
	}

	if (LEGACY_S16_FROM_BITS(axis) < LEGACY_S16_FROM_BITS(dos_joystick_axis1_low_threshold))
		dos_joystick_input |= DOS_JOYSTICK_AXIS1_LOW_FLAG;
	else if (LEGACY_S16_FROM_BITS(axis) >=
		LEGACY_S16_FROM_BITS(dos_joystick_axis1_high_threshold))
		dos_joystick_input |= DOS_JOYSTICK_AXIS1_HIGH_FLAG;

	axis = dos_joystick_axis2;
	if (axis < dos_joystick_axis2_min) {
		dos_joystick_axis2_candidate_ticks = LEGACY_U16_WRAP_SUB(dos_joystick_axis2_candidate_ticks, 1U);
		if (LEGACY_S16_FROM_BITS(dos_joystick_axis2_candidate_ticks) <= 0) {
			dos_joystick_axis2_min = dos_joystick_axis2_low_candidate;
			joystick_recalculate_axis2();
			joystick_reset_axis2_candidates();
		} else if (LEGACY_S16_FROM_BITS(axis) >=
			LEGACY_S16_FROM_BITS(dos_joystick_axis2_low_candidate)) {
			dos_joystick_axis2_low_candidate = axis;
		}
	} else if (LEGACY_S16_FROM_BITS(axis) >
		LEGACY_S16_FROM_BITS(dos_joystick_axis2_max)) {
		dos_joystick_axis2_candidate_ticks = LEGACY_U16_WRAP_SUB(dos_joystick_axis2_candidate_ticks, 1U);
		if (dos_joystick_axis2_candidate_ticks == 0) {
			dos_joystick_axis2_max = dos_joystick_axis2_high_candidate;
			joystick_recalculate_axis2();
			joystick_reset_axis2_candidates();
		} else if (LEGACY_S16_FROM_BITS(axis) <
			LEGACY_S16_FROM_BITS(dos_joystick_axis2_high_candidate)) {
			dos_joystick_axis2_high_candidate = axis;
		}
	} else {
		joystick_reset_axis2_candidates();
	}

	if (axis < dos_joystick_axis2_low_threshold)
		dos_joystick_input |= DOS_JOYSTICK_AXIS2_LOW_FLAG;
	else if (axis >= dos_joystick_axis2_high_threshold)
		dos_joystick_input |= DOS_JOYSTICK_AXIS2_HIGH_FLAG;

	buttons = (legacy_u8)inp(DOS_JOYSTICK_GAME_PORT);
	buttons &= dos_joystick_button_mask;
	buttons &= DOS_JOYSTICK_BUTTON_BITS;
	dos_joystick_input |= buttons ^ DOS_JOYSTICK_BUTTON_BITS;
	return dos_joystick_input;
}
