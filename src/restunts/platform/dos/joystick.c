#include <dos.h>
#include "../../c/legacy.h"

extern legacy_u8 byte_3FE00;
extern legacy_u16 joyflag1;
extern legacy_u16 joyflag2;
extern legacy_u8 joybutton;
extern legacy_u8 joyinput;
extern legacy_u16 word_3FB18;
extern legacy_u16 word_3FB1A;
extern legacy_u16 word_3FB1C;
extern legacy_u16 word_3FB1E;
extern legacy_u16 word_3FB20;
extern legacy_u16 word_3FB22;
extern legacy_u16 word_3FB24;
extern legacy_u16 word_3FB26;
extern legacy_u16 word_3FB28;
extern legacy_u16 word_3FB2A;
extern legacy_u16 word_3FB2C;
extern legacy_u16 word_3FB2E;
extern legacy_u16 word_3FB30;
extern legacy_u16 word_3FB32;
extern legacy_u16 word_3FB34;
extern legacy_u16 word_3FB36;

static void dos_sample_joystick_axes(void)
{
	/* The discharge loop is deliberately kept as one instruction-timed DOS
	 * block.  Moving port reads into an ordinary C loop changes calibration. */
	__asm {
		mov     joyinput, 0
		mov     dx, 201h
		in      al, dx
		mov     joybutton, al
		mov     bl, 3
		mov     joyflag1, 50h
		mov     joyflag2, 50h
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
		mov     joyflag2, cx
		and     bl, 1
		jnz     joy_sample_loop
		jmp     joy_sample_complete
	joy_sample_axis1_done:
		mov     joyflag1, cx
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

	range = LEGACY_U16_WRAP_SUB(word_3FB1C, word_3FB18);
	if (LEGACY_S16_FROM_BITS(range) > 0)
		word_3FB34 = (legacy_u16)(0x4000U / range);
	half = range >> 1;
	quarter = half >> 1;
	word_3FB24 = LEGACY_U16_WRAP_ADD(
		LEGACY_U16_WRAP_ADD(word_3FB18, half), quarter);
	word_3FB22 = LEGACY_U16_WRAP_SUB(
		LEGACY_U16_WRAP_SUB(word_3FB24, quarter), quarter);
}

static void joystick_recalculate_axis2(void)
{
	legacy_u16 range;
	legacy_u16 half;
	legacy_u16 quarter;

	range = LEGACY_U16_WRAP_SUB(word_3FB2A, word_3FB26);
	if (LEGACY_S16_FROM_BITS(range) > 0)
		word_3FB36 = (legacy_u16)(0x4000U / range);
	half = range >> 1;
	quarter = half >> 1;
	word_3FB32 = LEGACY_U16_WRAP_ADD(
		LEGACY_U16_WRAP_ADD(word_3FB26, half), quarter);
	word_3FB30 = LEGACY_U16_WRAP_SUB(
		LEGACY_U16_WRAP_SUB(word_3FB32, quarter), quarter);
}

static void joystick_reset_axis1_candidates(void)
{
	word_3FB20 = 0x14U;
	word_3FB1E = 0x4E20U;
	word_3FB1A = 0;
}

static void joystick_reset_axis2_candidates(void)
{
	word_3FB2E = 0x14U;
	word_3FB2C = 0x4E20U;
	word_3FB28 = 0;
}

legacy_s16 dos_get_joy_flags(void)
{
	legacy_u16 axis;
	legacy_u8 buttons;

	if ((byte_3FE00 & 1U) == 0)
		return 0;

	dos_sample_joystick_axes();

	axis = joyflag1;
	if (LEGACY_S16_FROM_BITS(axis) < LEGACY_S16_FROM_BITS(word_3FB18)) {
		word_3FB20 = LEGACY_U16_WRAP_SUB(word_3FB20, 1U);
		if (LEGACY_S16_FROM_BITS(word_3FB20) <= 0) {
			word_3FB18 = word_3FB1A;
			joystick_recalculate_axis1();
			joystick_reset_axis1_candidates();
		} else if (LEGACY_S16_FROM_BITS(axis) >=
			LEGACY_S16_FROM_BITS(word_3FB1A)) {
			word_3FB1A = axis;
		}
	} else if (LEGACY_S16_FROM_BITS(axis) >
		LEGACY_S16_FROM_BITS(word_3FB1C)) {
		word_3FB20 = LEGACY_U16_WRAP_SUB(word_3FB20, 1U);
		if (LEGACY_S16_FROM_BITS(word_3FB20) <= 0) {
			word_3FB1C = word_3FB1E;
			joystick_recalculate_axis1();
			joystick_reset_axis1_candidates();
		} else if (LEGACY_S16_FROM_BITS(axis) <
			LEGACY_S16_FROM_BITS(word_3FB1E)) {
			word_3FB1E = axis;
		}
	} else {
		joystick_reset_axis1_candidates();
	}

	if (LEGACY_S16_FROM_BITS(axis) < LEGACY_S16_FROM_BITS(word_3FB22))
		joyinput |= 8U;
	else if (LEGACY_S16_FROM_BITS(axis) >=
		LEGACY_S16_FROM_BITS(word_3FB24))
		joyinput |= 4U;

	axis = joyflag2;
	if (axis < word_3FB26) {
		word_3FB2E = LEGACY_U16_WRAP_SUB(word_3FB2E, 1U);
		if (LEGACY_S16_FROM_BITS(word_3FB2E) <= 0) {
			word_3FB26 = word_3FB28;
			joystick_recalculate_axis2();
			joystick_reset_axis2_candidates();
		} else if (LEGACY_S16_FROM_BITS(axis) >=
			LEGACY_S16_FROM_BITS(word_3FB28)) {
			word_3FB28 = axis;
		}
	} else if (LEGACY_S16_FROM_BITS(axis) >
		LEGACY_S16_FROM_BITS(word_3FB2A)) {
		word_3FB2E = LEGACY_U16_WRAP_SUB(word_3FB2E, 1U);
		if (word_3FB2E == 0) {
			word_3FB2A = word_3FB2C;
			joystick_recalculate_axis2();
			joystick_reset_axis2_candidates();
		} else if (LEGACY_S16_FROM_BITS(axis) <
			LEGACY_S16_FROM_BITS(word_3FB2C)) {
			word_3FB2C = axis;
		}
	} else {
		joystick_reset_axis2_candidates();
	}

	if (axis < word_3FB30)
		joyinput |= 1U;
	else if (axis >= word_3FB32)
		joyinput |= 2U;

	buttons = (legacy_u8)inp(0x201U);
	buttons &= joybutton;
	buttons &= 0x30U;
	joyinput |= buttons ^ 0x30U;
	return joyinput;
}
