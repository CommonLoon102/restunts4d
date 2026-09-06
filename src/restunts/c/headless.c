#include <stdarg.h>
#include "externs.h"
#include "memmgr.h"
#include "platform.h"

#define HEADLESS_STRING_INDEX_FIRST 0U
#define HEADLESS_STRING_TERMINATOR 0
#define HEADLESS_FORMAT_NEXT_CHARACTER_OFFSET 1U
#define HEADLESS_FATAL_EXIT_STATUS 1
#define HEADLESS_VIDEO_DISABLED 0
#define HEADLESS_VIDEO_UNIT_SCALE 1
#define HEADLESS_KEY_NONE 0

extern void headless_exit(legacy_s16 result);

void add_exit_handler(void (far* exit_handler)(void))
{
	(void)exit_handler;
}

static void headless_write_text(const legacy_s8* text)
{
	legacy_u16 length;

	length = HEADLESS_STRING_INDEX_FIRST;
	while (text[length] != HEADLESS_STRING_TERMINATOR)
		length++;
	(void)dos_write_stdout(text, length);
}

void fatal_error(const legacy_s8* format, ...)
{
	legacy_u16 index;
	va_list arguments;
	const legacy_s8* detail;

	headless_write_text(format);
	for (index = HEADLESS_STRING_INDEX_FIRST;
		format[index] != HEADLESS_STRING_TERMINATOR; index++) {
		if (format[index] == '%' &&
			format[index + HEADLESS_FORMAT_NEXT_CHARACTER_OFFSET] == 's') {
			va_start(arguments, format);
			detail = va_arg(arguments, const legacy_s8*);
			va_end(arguments);
			headless_write_text(" ");
			headless_write_text(detail);
			break;
		}
	}
	headless_write_text("\r\n");
	headless_exit(HEADLESS_FATAL_EXIT_STATUS);
}

void init_div0(void)
{
	dos_install_divide_error_handler();
}

void init_main(legacy_s16 argc, legacy_s8* argv[])
{
	(void)argc;
	(void)argv;
	init_video_geometry_flags();
	video_flag5_is0 = HEADLESS_VIDEO_DISABLED;
	video_flag6_is1 = HEADLESS_VIDEO_UNIT_SCALE;
	textresprefix = 'e';
	framespersec = GAME_FRAME_RATE_NORMAL;
	mmgr_alloc_a000();
	himem_init();
}

legacy_s16 kb_read_char(void)
{
	return HEADLESS_KEY_NONE;
}
