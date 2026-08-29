#include <dos.h>
#include "../../c/legacy.h"

#define int86 _int86
extern legacy_s16 _Cdecl _int86(legacy_s16 interrupt_number,
	union REGS far* input, union REGS far* output);

extern legacy_u8 mousehorscale;
extern legacy_u16 word_45D7C;
extern legacy_u16 word_40318;
extern legacy_u16 word_44D3C;
extern legacy_u16 word_44D62;

static void dos_mouse_set_pixel_ratio(legacy_u16 horizontal,
	legacy_u16 vertical)
{
	union REGS registers;

	registers.x.ax = 0x000FU;
	registers.x.cx = horizontal;
	registers.x.dx = vertical;
	int86(0x33, &registers, &registers);
}

void dos_mouse_set_minmax(legacy_s16 minimum_x, legacy_s16 minimum_y,
	legacy_s16 maximum_x, legacy_s16 maximum_y)
{
	union REGS registers;
	legacy_u16 scale;

	scale = mousehorscale;
	registers.x.ax = 7;
	registers.x.cx = (legacy_u16)minimum_x << scale;
	registers.x.dx = (legacy_u16)maximum_x << scale;
	int86(0x33, &registers, &registers);

	registers.x.ax = 8;
	registers.x.cx = (legacy_u16)minimum_y;
	registers.x.dx = (legacy_u16)maximum_y;
	int86(0x33, &registers, &registers);
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
	registers.x.flags = 0;
	registers.x.ax = 0xC201U;
	int86(0x15, &registers, &registers);

	registers.x.ax = 0;
	int86(0x33, &registers, &registers);
	installed = (legacy_s16)registers.x.ax;
	word_45D7C = registers.x.bx;
	if (installed != 0) {
		mousehorscale = width == 0x140 ? 1U : 0U;
		dos_mouse_set_minmax(0, 0,
			LEGACY_S16_WRAP_SUB(width, 1),
			LEGACY_S16_WRAP_SUB(height, 1));
		dos_mouse_set_pixel_ratio(0x10U, 0x10U);
		word_40318 = 0xFFFFU;
	}
	return installed;
}

void dos_mouse_set_position(legacy_s16 x, legacy_s16 y)
{
	union REGS registers;

	registers.x.ax = 4;
	word_44D3C = (legacy_u16)x;
	registers.x.cx = (legacy_u16)x << mousehorscale;
	registers.x.dx = (legacy_u16)y;
	word_44D62 = (legacy_u16)y;
	int86(0x33, &registers, &registers);
}

void dos_mouse_get_state(legacy_s16* buttons, legacy_s16* x, legacy_s16* y)
{
	union REGS registers;

	registers.x.ax = 3;
	int86(0x33, &registers, &registers);
	*buttons = (legacy_s16)registers.x.bx;
	*x = (legacy_s16)(registers.x.cx >> mousehorscale);
	*y = (legacy_s16)registers.x.dx;
}
