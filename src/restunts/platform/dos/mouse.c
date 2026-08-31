#include "../../c/platform.h"
#include "doscompat.h"

static legacy_u16 dos_mouse_button_count;

static legacy_u8 dos_mouse_horizontal_scale;
static legacy_u16 dos_mouse_available;
static legacy_u16 dos_mouse_last_x;
static legacy_u16 dos_mouse_last_y;

static void dos_mouse_set_pixel_ratio(legacy_u16 horizontal,
	legacy_u16 vertical)
{
	union REGS registers;

	registers.x.ax = 0x000FU;
	registers.x.cx = horizontal;
	registers.x.dx = vertical;
	dos_int86(0x33, &registers, &registers);
}

void dos_mouse_set_minmax(legacy_s16 minimum_x, legacy_s16 minimum_y,
	legacy_s16 maximum_x, legacy_s16 maximum_y)
{
	union REGS registers;
	legacy_u16 scale;

	scale = dos_mouse_horizontal_scale;
	registers.x.ax = 7;
	registers.x.cx = (legacy_u16)minimum_x << scale;
	registers.x.dx = (legacy_u16)maximum_x << scale;
	dos_int86(0x33, &registers, &registers);

	registers.x.ax = 8;
	registers.x.cx = (legacy_u16)minimum_y;
	registers.x.dx = (legacy_u16)maximum_y;
	dos_int86(0x33, &registers, &registers);
}

legacy_s16 dos_mouse_init(legacy_s16 width, legacy_s16 height)
{
	union REGS registers;
	legacy_s16 installed;

	registers.x.ax = 0;
	registers.x.bx = 0;
	registers.x.cx = 0;
	registers.x.dx = 0;
	registers.x.si = 0;
	registers.x.di = 0;
	registers.x.cflag = 0;
#if defined(__BORLANDC__)
	registers.x.flags = 0;
#endif
	registers.x.ax = 0xC201U;
	dos_int86(0x15, &registers, &registers);

	registers.x.ax = 0;
	dos_int86(0x33, &registers, &registers);
	installed = (legacy_s16)registers.x.ax;
	dos_mouse_button_count = registers.x.bx;
	if (installed != 0) {
		dos_mouse_horizontal_scale = width == 0x140 ? 1U : 0U;
		dos_mouse_set_minmax(0, 0,
			LEGACY_S16_WRAP_SUB(width, 1),
			LEGACY_S16_WRAP_SUB(height, 1));
		dos_mouse_set_pixel_ratio(0x10U, 0x10U);
		dos_mouse_available = 0xFFFFU;
	}
	return installed;
}

void dos_mouse_set_position(legacy_s16 x, legacy_s16 y)
{
	union REGS registers;

	registers.x.ax = 4;
	dos_mouse_last_x = (legacy_u16)x;
	registers.x.cx = (legacy_u16)x << dos_mouse_horizontal_scale;
	registers.x.dx = (legacy_u16)y;
	dos_mouse_last_y = (legacy_u16)y;
	dos_int86(0x33, &registers, &registers);
}

void dos_mouse_get_state(legacy_s16* buttons, legacy_s16* x, legacy_s16* y)
{
	union REGS registers;

	registers.x.ax = 3;
	dos_int86(0x33, &registers, &registers);
	*buttons = (legacy_s16)registers.x.bx;
	*x = (legacy_s16)(registers.x.cx >> dos_mouse_horizontal_scale);
	*y = (legacy_s16)registers.x.dx;
}

legacy_u16 dos_mouse_get_button_count(void)
{
	return dos_mouse_button_count;
}
