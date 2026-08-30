#define int86 _int86
#include <dos.h>
#include "../../c/legacy.h"

extern void add_exit_handler(void (far* exit_handler)(void));

static legacy_u8 saved_video_mode;
static legacy_u8 saved_equipment_byte;
static legacy_u8 mode4_active;

static const legacy_u8 mode4_crtc_registers[12] = {
	53U, 40U, 44U, 7U, 121U, 2U,
	100U, 110U, 2U, 2U, 0U, 0U
};

static const legacy_u8 mode7_crtc_registers[12] = {
	97U, 80U, 82U, 15U, 25U, 6U,
	25U, 25U, 2U, 13U, 11U, 12U
};

static void dos_video_set_equipment_bits(legacy_u16 display_bits)
{
	legacy_u16 equipment;

	equipment = (legacy_u16)peek(0x40U, 0x10U);
	equipment = (legacy_u16)((equipment & 0xFFCFU) | display_bits);
	poke(0x40U, 0x10U, equipment);
}

static void dos_video_set_bios_mode(legacy_u8 mode)
{
	union REGS registers;

	registers.h.ah = 0;
	registers.h.al = mode;
	int86(0x10, &registers, &registers);
}

static void dos_video_reset_palette(void)
{
	union REGS registers;

	registers.h.ah = 0x0BU;
	registers.x.bx = 0;
	int86(0x10, &registers, &registers);
}

static void dos_video_fill(legacy_u16 segment, legacy_u16 value,
	legacy_u16 word_count)
{
	legacy_u16 far* destination;
	legacy_u16 index;

	destination = (legacy_u16 far*)MK_FP(segment, 0);
	for (index = 0; index < word_count; ++index)
		destination[index] = value;
}

static void dos_video_program_crtc(const legacy_u8* values)
{
	legacy_u16 index;

	for (index = 0; index < 12U; ++index) {
		outp(0x3B4U, index);
		outp(0x3B5U, values[index]);
	}
}

static void far dos_video_on_exit(void)
{
	legacy_u8 equipment;

	pokeb(0x40U, 0x10U, saved_equipment_byte);
	dos_video_set_bios_mode(saved_video_mode);
	pokeb(0x40U, 0x10U, saved_equipment_byte);
	equipment = saved_equipment_byte;
	if ((equipment & 0x30U) == 0x30U)
		dos_video_fill(0xA000U, 0, 0xFA00U);
	dos_video_reset_palette();
}

static void dos_video_add_exit_handler(void)
{
	union REGS registers;

	if (saved_video_mode != 0)
		return;
	registers.h.ah = 0x0FU;
	int86(0x10, &registers, &registers);
	saved_video_mode = registers.h.al;
	saved_equipment_byte = (legacy_u8)peekb(0x40U, 0x10U);
	add_exit_handler(dos_video_on_exit);
}

static void dos_video_set_mode3(void)
{
	dos_video_fill(0xA000U, 0, 0xFA00U);
	dos_video_set_equipment_bits(0x10U);
	dos_video_set_bios_mode(3U);
	dos_video_reset_palette();
}

legacy_s16 dos_video_get_status(void)
{
	return (legacy_s16)(inport(0x3DAU) & 8U);
}

/* A dormant translated-assembly fallback still imports this legacy name. */
legacy_s16 video_get_status(void)
{
	return dos_video_get_status();
}

void dos_video_set_palette(legacy_u16 start, legacy_u16 count,
	legacy_u8* palette)
{
	/* The source palette is a near pointer in the game's data segment. */
	__asm {
		push    es
		mov     ax, ds
		mov     es, ax
		mov     bx, start
		mov     cx, count
		mov     dx, palette
		mov     ax, 1012h
		int     10h
		pop     es
	}
}

void dos_video_set_mode_13h(void)
{
	dos_video_add_exit_handler();
	dos_video_set_equipment_bits(0x10U);
	dos_video_reset_palette();
	dos_video_set_bios_mode(0x13U);
	dos_video_fill(0xA000U, 0, 0xFA00U);
}

void dos_video_set_mode4(void)
{
	mode4_active = 1U;
	dos_video_set_equipment_bits(0x20U);
	dos_video_set_bios_mode(4U);
	outp(0x3BFU, 3U);
	outp(0x3B8U, 2U);
	dos_video_program_crtc(mode4_crtc_registers);
	dos_video_fill(0xB800U, 0, 0x4000U);
	outp(0x3B8U, 0x8AU);
}

void dos_video_set_mode7(void)
{
	if (mode4_active == 0) {
		dos_video_set_mode3();
		return;
	}

	dos_video_set_equipment_bits(0x30U);
	outp(0x3BFU, 3U);
	outp(0x3B8U, 0x20U);
	dos_video_program_crtc(mode7_crtc_registers);
	dos_video_fill(0xB800U, 0, 0x4000U);
	outp(0x3B8U, 0x28U);
	dos_video_set_bios_mode(7U);
}
