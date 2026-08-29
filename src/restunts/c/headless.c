#ifdef RESTUNTS_DOS
#include <dos.h>
#endif
#include <stdarg.h>
#include "externs.h"
#include "memmgr.h"

#ifdef RESTUNTS_HEADLESS
extern legacy_u16 word_3BE30;
extern legacy_u16 word_3BE32;
extern void headless_exit(legacy_s16 result);

void add_exit_handler(void (far* exit_handler)(void))
{
	(void)exit_handler;
}

static void headless_write_text(const legacy_s8* text)
{
	legacy_u16 length;
	legacy_u16 message_offset;

	length = 0;
	while (text[length] != 0)
		length++;
	message_offset = FP_OFF(text);
#ifdef RESTUNTS_DOS
	__asm {
		mov ah, 40h
		mov bx, 1
		mov cx, length
		mov dx, message_offset
		int 21h
	}
#endif
}

void fatal_error(const legacy_s8* format, ...)
{
	legacy_u16 index;
	va_list arguments;
	const legacy_s8* detail;

	headless_write_text(format);
	for (index = 0; format[index] != 0; index++) {
		if (format[index] == '%' && format[index + 1] == 's') {
			va_start(arguments, format);
			detail = va_arg(arguments, const legacy_s8*);
			va_end(arguments);
			headless_write_text(" ");
			headless_write_text(detail);
			break;
		}
	}
	headless_write_text("\r\n");
	headless_exit(1);
}

#ifdef RESTUNTS_DOS
extern void _CType _setvect(legacy_s16 interrupt_number,
	void interrupt (far* handler)());
extern void interrupt (far* _CType _getvect(
	legacy_s16 interrupt_number))();
static void interrupt (far* old_intr0_handler)();

#pragma argsused
static void interrupt headless_intr0_handler(legacy_u16 bp, legacy_u16 di,
	legacy_u16 si, legacy_u16 ds, legacy_u16 es, legacy_u16 dx,
	legacy_u16 cx, legacy_u16 bx, legacy_u16 ax, legacy_u16 ip,
	legacy_u16 cs, legacy_u16 flags)
{
	word_3BE30 = cs;
	word_3BE32 = ip;
	ip = LEGACY_U16_WRAP_ADD(ip, 2U);
	ax = 0;
}
#endif

void init_div0(void)
{
#ifdef RESTUNTS_DOS
	old_intr0_handler = _getvect(0);
	_setvect(0, headless_intr0_handler);
#endif
}

void init_main(legacy_s16 argc, legacy_s8* argv[])
{
	(void)argc;
	(void)argv;
	video_flag1_is1 = 1;
	video_flag2_is1 = 1;
	video_flag3_isFFFF = -1;
	video_flag4_is1 = 1;
	video_flag5_is0 = 0;
	video_flag6_is1 = 1;
	textresprefix = 'e';
	framespersec = 20;
	mmgr_alloc_a000();
	himem_init();
}

legacy_s16 kb_read_char(void)
{
	return 0;
}
#endif
