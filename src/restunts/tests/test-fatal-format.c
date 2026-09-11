#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include <stdarg.h>
#include "../c/legacy.h"
/* DOS int is 16 bits. Promote short va_arg reads in this host-only fixture
 * so the unchanged DOS implementation can be compared with default-promoted
 * host arguments. Both baseline and refactored implementations use this shim. */
#undef va_arg
#define va_arg(arguments, type)                                                                    \
	((type) __builtin_va_arg(arguments, __typeof__(__builtin_choose_expr(                          \
											__builtin_types_compatible_p(type, legacy_s16) ||      \
												__builtin_types_compatible_p(type, legacy_u16),    \
											(int)0, (type)0))))
#include "../c/fatal.c"
#undef printf
#undef memcpy
#undef strlen

static uint32_t trace_hash = UINT32_C(2166136261);
static legacy_s8 captured[20000];
static unsigned captured_length, write_count;
static jmp_buf exit_jump;
static void trace(legacy_u32 value)
{
	unsigned i;
	for (i = 0; i < 4; i++) {
		trace_hash = (trace_hash ^ (value & 255U)) * UINT32_C(16777619);
		value >>= 8;
	}
}
legacy_s16 dos_write_stderr(const legacy_s8 *data, legacy_u16 length)
{
	unsigned i;
	assert(length <= 96);
	assert(captured_length + length < sizeof(captured));
	trace(1);
	trace(length);
	write_count++;
	for (i = 0; i < length; i++) {
		trace((legacy_u8)data[i]);
	}
	memcpy(captured + captured_length, data, length);
	captured_length += length;
	return length;
}
void sprite_select_screen(void)
{
	trace(2);
}
void flush_stdin(void)
{
	trace(3);
}
void dos_process_exit(legacy_s16 status)
{
	trace(4);
	trace(status);
	longjmp(exit_jump, 1);
}
static void exit_first(void)
{
	trace(5);
}
static void exit_second(void)
{
	trace(6);
}
static void check_format(const legacy_s8 *format, ...)
{
	va_list arguments;
	captured_length = write_count = 0;
	va_start(arguments, format);
	fatal_vprintf(format, arguments);
	va_end(arguments);
	trace(captured_length);
	trace(write_count);
}
int main(void)
{
	static const legacy_s16 signed_values[] = {0, 1, -1, 32767, -32768};
	static const legacy_u32 long_values[] = {0, 1, 0x7fffffffUL, 0x80000000UL, 0xffffffffUL};
	static const legacy_s8 *formats[] = {
		"%d/%i/%u/%x/%X/%ld/%li/%lu/%lx/%lX|%c|%s",
		"%08d/%-8i/%8u/%08x/%8.4X/%015ld/%-15li/%15lu/%015lx/%15.4lX|%4c|%10s",
		"%.0d/%.5i/%.0u/%.6x/%-8.3X/%.0ld/%.12li/%.0lu/%.12lx/%-15.3lX|%-4c|%-10.3s",
		"%0008d/%--8i/%65536u/%65537x/%.65536X/%ld/%li/%lu/%lx/%lX|%c|%.0s",
	};
	legacy_s8 long_text[195];
	unsigned a, b, f, length;
	for (a = 0; a < 5; a++) {
		for (b = 0; b < 5; b++) {
			for (f = 0; f < 4; f++) {
				check_format(formats[f], signed_values[a], signed_values[a],
							 (legacy_u16)signed_values[a], (legacy_u16)signed_values[a],
							 (legacy_u16)signed_values[a], (legacy_s32)long_values[b],
							 (legacy_s32)long_values[b], long_values[b], long_values[b],
							 long_values[b], (legacy_s16)('A' + a),
							 a & 1 ? (legacy_s8 *)0 : (legacy_s8 *)"hello world");
			}
		}
	}
	check_format((legacy_s8 *)"literal %% %q %lq trailing%");
	memset(long_text, 'A', sizeof(long_text));
	for (length = 94; length <= 194; length++) {
		long_text[length] = 0;
		check_format((legacy_s8 *)"%s", long_text);
		long_text[length] = 'A';
	}
	captured_length = write_count = 0;
	memset(exitlistfuncs, 0, sizeof(void (*)(void)) * (EXIT_HANDLER_MAX_COUNT + 1));
	add_exit_handler(exit_first);
	add_exit_handler(exit_second);
	add_exit_handler(exit_first);
	if (setjmp(exit_jump) == 0) {
		fatal_error((legacy_s8 *)"fatal %d %s", (legacy_s16)-32768, (legacy_s8 *)"fixture");
	}
	assert(captured_length == 40);
#ifdef FATAL_RECORD_BASELINE
	printf("Fatal formatting fingerprint: %08x\n", (unsigned)trace_hash);
#else
	assert(trace_hash == UINT32_C(0x6ec7964a));
#endif
	return 0;
}
