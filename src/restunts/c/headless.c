#ifdef RESTUNTS_DOS
#include <dos.h>
#endif
#include <stdarg.h>
#include "externs.h"
#include "memmgr.h"
#include "platform.h"

#ifdef RESTUNTS_HEADLESS
extern void headless_exit(legacy_s16 result);

void add_exit_handler(void (far* exit_handler)(void))
{
	(void)exit_handler;
}

static void headless_write_text(const legacy_s8* text)
{
	legacy_u16 length;

	length = 0;
	while (text[length] != 0)
		length++;
#ifdef RESTUNTS_DOS
	(void)dos_write_stdout(text, length);
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

void init_div0(void)
{
#ifdef RESTUNTS_DOS
	dos_install_divide_error_handler();
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
