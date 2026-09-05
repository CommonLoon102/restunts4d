#define int86 _int86
#include <dos.h>
#include "../../c/legacy.h"

extern void add_exit_handler(void (far* exit_handler)(void));

#define DOS_VIDEO_BIOS_INTERRUPT 16
#define DOS_VIDEO_BIOS_SET_MODE_FUNCTION 0
#define DOS_VIDEO_BIOS_SET_BACKGROUND_FUNCTION 11
#define DOS_VIDEO_BIOS_GET_MODE_FUNCTION 15
#define DOS_VIDEO_BIOS_SET_DAC_BLOCK_FUNCTION 4114
#define DOS_VIDEO_BIOS_DATA_SEGMENT 64U
#define DOS_VIDEO_BIOS_EQUIPMENT_OFFSET 16U
#define DOS_VIDEO_EQUIPMENT_DISPLAY_CLEAR_MASK 65487U
#define DOS_VIDEO_EQUIPMENT_COLOR_BITS 16U
#define DOS_VIDEO_EQUIPMENT_MODE4_BITS 32U
#define DOS_VIDEO_EQUIPMENT_MONOCHROME_BITS 48U
#define DOS_VIDEO_CRTC_REGISTER_COUNT 12U
#define DOS_VIDEO_CRTC_INDEX_PORT 948U
#define DOS_VIDEO_CRTC_DATA_PORT 949U
#define DOS_VIDEO_STATUS_PORT 986U
#define DOS_VIDEO_RETRACE_STATUS_BIT 8U
#define DOS_VIDEO_GRAPHICS_SEGMENT 40960U
#define DOS_VIDEO_GRAPHICS_CLEAR_WORDS 64000U
#define DOS_VIDEO_MONOCHROME_SEGMENT 47104U
#define DOS_VIDEO_MONOCHROME_CLEAR_WORDS 16384U
#define DOS_VIDEO_HERCULES_CONFIG_PORT 959U
#define DOS_VIDEO_HERCULES_CONTROL_PORT 952U
#define DOS_VIDEO_HERCULES_CONFIG_ENABLE 3U
#define DOS_VIDEO_HERCULES_MODE4_CONTROL 2U
#define DOS_VIDEO_HERCULES_MODE4_ACTIVE_CONTROL 138U
#define DOS_VIDEO_HERCULES_MODE7_CONTROL 32U
#define DOS_VIDEO_HERCULES_MODE7_ACTIVE_CONTROL 40U
#define DOS_VIDEO_BIOS_MODE3 3U
#define DOS_VIDEO_BIOS_MODE4 4U
#define DOS_VIDEO_BIOS_MODE7 7U
#define DOS_VIDEO_BIOS_MODE13 19U

static legacy_u8 saved_video_mode;
static legacy_u8 saved_equipment_byte;
static legacy_u8 mode4_active;

static const legacy_u8 mode4_crtc_registers[DOS_VIDEO_CRTC_REGISTER_COUNT] = {
	53U, 40U, 44U, 7U, 121U, 2U,
	100U, 110U, 2U, 2U, 0U, 0U
};

static const legacy_u8 mode7_crtc_registers[DOS_VIDEO_CRTC_REGISTER_COUNT] = {
	97U, 80U, 82U, 15U, 25U, 6U,
	25U, 25U, 2U, 13U, 11U, 12U
};

static void dos_video_set_equipment_bits(legacy_u16 display_bits)
{
	legacy_u16 equipment;

	equipment = (legacy_u16)peek(DOS_VIDEO_BIOS_DATA_SEGMENT,
		DOS_VIDEO_BIOS_EQUIPMENT_OFFSET);
	equipment = (legacy_u16)((equipment &
		DOS_VIDEO_EQUIPMENT_DISPLAY_CLEAR_MASK) | display_bits);
	poke(DOS_VIDEO_BIOS_DATA_SEGMENT, DOS_VIDEO_BIOS_EQUIPMENT_OFFSET,
		equipment);
}

static void dos_video_set_bios_mode(legacy_u8 mode)
{
	union REGS registers;

	registers.h.ah = DOS_VIDEO_BIOS_SET_MODE_FUNCTION;
	registers.h.al = mode;
	int86(DOS_VIDEO_BIOS_INTERRUPT, &registers, &registers);
}

static void dos_video_reset_palette(void)
{
	union REGS registers;

	registers.h.ah = DOS_VIDEO_BIOS_SET_BACKGROUND_FUNCTION;
	registers.x.bx = 0;
	int86(DOS_VIDEO_BIOS_INTERRUPT, &registers, &registers);
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

	for (index = 0; index < DOS_VIDEO_CRTC_REGISTER_COUNT; ++index) {
		outp(DOS_VIDEO_CRTC_INDEX_PORT, index);
		outp(DOS_VIDEO_CRTC_DATA_PORT, values[index]);
	}
}

static void far dos_video_on_exit(void)
{
	legacy_u8 equipment;

	pokeb(DOS_VIDEO_BIOS_DATA_SEGMENT, DOS_VIDEO_BIOS_EQUIPMENT_OFFSET,
		saved_equipment_byte);
	dos_video_set_bios_mode(saved_video_mode);
	pokeb(DOS_VIDEO_BIOS_DATA_SEGMENT, DOS_VIDEO_BIOS_EQUIPMENT_OFFSET,
		saved_equipment_byte);
	equipment = saved_equipment_byte;
	if ((equipment & DOS_VIDEO_EQUIPMENT_MONOCHROME_BITS) ==
		DOS_VIDEO_EQUIPMENT_MONOCHROME_BITS)
		dos_video_fill(DOS_VIDEO_GRAPHICS_SEGMENT, 0,
			DOS_VIDEO_GRAPHICS_CLEAR_WORDS);
	dos_video_reset_palette();
}

static void dos_video_add_exit_handler(void)
{
	union REGS registers;

	if (saved_video_mode != 0)
		return;
	registers.h.ah = DOS_VIDEO_BIOS_GET_MODE_FUNCTION;
	int86(DOS_VIDEO_BIOS_INTERRUPT, &registers, &registers);
	saved_video_mode = registers.h.al;
	saved_equipment_byte = (legacy_u8)peekb(DOS_VIDEO_BIOS_DATA_SEGMENT,
		DOS_VIDEO_BIOS_EQUIPMENT_OFFSET);
	add_exit_handler(dos_video_on_exit);
}

static void dos_video_set_mode3(void)
{
	dos_video_fill(DOS_VIDEO_GRAPHICS_SEGMENT, 0,
		DOS_VIDEO_GRAPHICS_CLEAR_WORDS);
	dos_video_set_equipment_bits(DOS_VIDEO_EQUIPMENT_COLOR_BITS);
	dos_video_set_bios_mode(DOS_VIDEO_BIOS_MODE3);
	dos_video_reset_palette();
}

legacy_s16 dos_video_get_status(void)
{
	return (legacy_s16)(inport(DOS_VIDEO_STATUS_PORT) &
		DOS_VIDEO_RETRACE_STATUS_BIT);
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
		mov     ax, DOS_VIDEO_BIOS_SET_DAC_BLOCK_FUNCTION
		int     DOS_VIDEO_BIOS_INTERRUPT
		pop     es
	}
}

void dos_video_set_mode_13h(void)
{
	dos_video_add_exit_handler();
	dos_video_set_equipment_bits(DOS_VIDEO_EQUIPMENT_COLOR_BITS);
	dos_video_reset_palette();
	dos_video_set_bios_mode(DOS_VIDEO_BIOS_MODE13);
	dos_video_fill(DOS_VIDEO_GRAPHICS_SEGMENT, 0,
		DOS_VIDEO_GRAPHICS_CLEAR_WORDS);
}

void dos_video_set_mode4(void)
{
	mode4_active = 1U;
	dos_video_set_equipment_bits(DOS_VIDEO_EQUIPMENT_MODE4_BITS);
	dos_video_set_bios_mode(DOS_VIDEO_BIOS_MODE4);
	outp(DOS_VIDEO_HERCULES_CONFIG_PORT,
		DOS_VIDEO_HERCULES_CONFIG_ENABLE);
	outp(DOS_VIDEO_HERCULES_CONTROL_PORT,
		DOS_VIDEO_HERCULES_MODE4_CONTROL);
	dos_video_program_crtc(mode4_crtc_registers);
	dos_video_fill(DOS_VIDEO_MONOCHROME_SEGMENT, 0,
		DOS_VIDEO_MONOCHROME_CLEAR_WORDS);
	outp(DOS_VIDEO_HERCULES_CONTROL_PORT,
		DOS_VIDEO_HERCULES_MODE4_ACTIVE_CONTROL);
}

void dos_video_set_mode7(void)
{
	if (mode4_active == 0) {
		dos_video_set_mode3();
		return;
	}

	dos_video_set_equipment_bits(DOS_VIDEO_EQUIPMENT_MONOCHROME_BITS);
	outp(DOS_VIDEO_HERCULES_CONFIG_PORT,
		DOS_VIDEO_HERCULES_CONFIG_ENABLE);
	outp(DOS_VIDEO_HERCULES_CONTROL_PORT,
		DOS_VIDEO_HERCULES_MODE7_CONTROL);
	dos_video_program_crtc(mode7_crtc_registers);
	dos_video_fill(DOS_VIDEO_MONOCHROME_SEGMENT, 0,
		DOS_VIDEO_MONOCHROME_CLEAR_WORDS);
	outp(DOS_VIDEO_HERCULES_CONTROL_PORT,
		DOS_VIDEO_HERCULES_MODE7_ACTIVE_CONTROL);
	dos_video_set_bios_mode(DOS_VIDEO_BIOS_MODE7);
}
