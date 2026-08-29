#include <dos.h>
#include "../../c/legacy.h"

legacy_u8 dos_joystick_enabled;
legacy_u16 dos_joystick_axis1;
legacy_u16 dos_joystick_axis2;
legacy_u16 dos_joystick_axis1_min = 80U;
legacy_u16 dos_joystick_axis1_max;
legacy_u16 dos_joystick_axis2_min = 80U;
legacy_u16 dos_joystick_axis2_max;
legacy_u16 dos_joystick_axis1_scale;
legacy_u16 dos_joystick_axis2_scale;

static legacy_u8 dos_joystick_button_mask;
static legacy_u8 dos_joystick_input;
static legacy_u16 dos_joystick_axis1_low_candidate;
static legacy_u16 dos_joystick_axis1_high_candidate;
static legacy_u16 dos_joystick_axis1_candidate_ticks = 20U;
static legacy_u16 dos_joystick_axis1_low_threshold;
static legacy_u16 dos_joystick_axis1_high_threshold;
static legacy_u16 dos_joystick_axis2_low_candidate;
static legacy_u16 dos_joystick_axis2_high_candidate;
static legacy_u16 dos_joystick_axis2_candidate_ticks = 20U;
static legacy_u16 dos_joystick_axis2_low_threshold;
static legacy_u16 dos_joystick_axis2_high_threshold;

static void dos_sample_joystick_axes(void)
{
	/* The discharge loop is deliberately kept as one instruction-timed DOS
	 * block.  Moving port reads into an ordinary C loop changes calibration. */
	__asm {
		mov     dos_joystick_input, 0
		mov     dx, 201h
		in      al, dx
		mov     dos_joystick_button_mask, al
		mov     bl, 3
		mov     dos_joystick_axis1, 50h
		mov     dos_joystick_axis2, 50h
		cli
		out     dx, al
		mov     cx, 14h
	joy_sample_delay:
		loop    joy_sample_delay
		xor     cx, cx
	joy_sample_loop:
		in      al, dx
		and     al, bl
		xor     al, bl
		jnz     joy_sample_axis_done
		inc     cx
		cmp     cx, 0FA0h
		jl      joy_sample_loop
		jmp     joy_sample_complete
	joy_sample_axis_done:
		test    al, 1
		jnz     joy_sample_axis1_done
	joy_sample_axis2_test:
		test    al, 2
		jz      joy_sample_loop
		mov     dos_joystick_axis2, cx
		and     bl, 1
		jnz     joy_sample_loop
		jmp     joy_sample_complete
	joy_sample_axis1_done:
		mov     dos_joystick_axis1, cx
		and     bl, 2
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
		dos_joystick_axis1_scale = (legacy_u16)(0x4000U / range);
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
		dos_joystick_axis2_scale = (legacy_u16)(0x4000U / range);
	half = range >> 1;
	quarter = half >> 1;
	dos_joystick_axis2_high_threshold = LEGACY_U16_WRAP_ADD(
		LEGACY_U16_WRAP_ADD(dos_joystick_axis2_min, half), quarter);
	dos_joystick_axis2_low_threshold = LEGACY_U16_WRAP_SUB(
		LEGACY_U16_WRAP_SUB(dos_joystick_axis2_high_threshold, quarter), quarter);
}

static void joystick_reset_axis1_candidates(void)
{
	dos_joystick_axis1_candidate_ticks = 0x14U;
	dos_joystick_axis1_high_candidate = 0x4E20U;
	dos_joystick_axis1_low_candidate = 0;
}

static void joystick_reset_axis2_candidates(void)
{
	dos_joystick_axis2_candidate_ticks = 0x14U;
	dos_joystick_axis2_high_candidate = 0x4E20U;
	dos_joystick_axis2_low_candidate = 0;
}

legacy_s16 dos_get_joy_flags(void)
{
	legacy_u16 axis;
	legacy_u8 buttons;

	if ((dos_joystick_enabled & 1U) == 0)
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
		dos_joystick_input |= 8U;
	else if (LEGACY_S16_FROM_BITS(axis) >=
		LEGACY_S16_FROM_BITS(dos_joystick_axis1_high_threshold))
		dos_joystick_input |= 4U;

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
		dos_joystick_input |= 1U;
	else if (axis >= dos_joystick_axis2_high_threshold)
		dos_joystick_input |= 2U;

	buttons = (legacy_u8)inp(0x201U);
	buttons &= dos_joystick_button_mask;
	buttons &= 0x30U;
	dos_joystick_input |= buttons ^ 0x30U;
	return dos_joystick_input;
}
