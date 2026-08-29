#ifdef RESTUNTS_DOS
#include <dos.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include "externs.h"
#include "memmgr.h"

#ifdef RESTUNTS_HEADLESS
extern void _abort(void);
extern legacy_s16 __stbuf(FILE* stream);
extern void __ftbuf(legacy_s16 buffer_state, FILE* stream);
extern legacy_s16 __output(FILE* stream, const legacy_s8* format,
	void* arguments);
extern legacy_u16 word_3BE30;
extern legacy_u16 word_3BE32;

void add_exit_handler(void (far* exit_handler)(void))
{
	(void)exit_handler;
}

void fatal_error(const legacy_s8* format, ...)
{
	legacy_s16 buffer_state;
	va_list arguments;

	buffer_state = __stbuf(stdout);
	va_start(arguments, format);
	__output(stdout, format, (void*)FP_OFF(arguments));
	va_end(arguments);
	__ftbuf(buffer_state, stdout);
	_abort();
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
