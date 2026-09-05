#include <stdarg.h>
#include "externs.h"
#include "fatal.h"
#include "platform.h"
#include "shape2d.h"

#define FATAL_OUTPUT_BUFFER_SIZE 96U
#define FATAL_NUMBER_SCRATCH_SIZE 12U
#define FATAL_FORMAT_DECIMAL_RADIX 10U
#define FATAL_FORMAT_HEXADECIMAL_RADIX 16U

struct FATAL_OUTPUT_STATE {
	legacy_s8 buffer[FATAL_OUTPUT_BUFFER_SIZE];
	legacy_u16 length;
};

void flush_stdin(void);

void add_exit_handler(void (far* exit_handler)(void))
{
	legacy_s16 index;

	for (index = 0; index < EXIT_HANDLER_MAX_COUNT; index++) {
		if (exitlistfuncs[index] == exit_handler)
			return;
		if (exitlistfuncs[index] == 0) {
			exitlistfuncs[index] = exit_handler;
			exitlistfuncs[index + 1] = 0;
			return;
		}
	}
	fatal_error(aExitListOverflow);
}

void call_exitlist(void)
{
	legacy_s16 index;

	for (index = EXIT_HANDLER_MAX_COUNT; index >= 0; index--)
		if (exitlistfuncs[index] != 0)
			exitlistfuncs[index]();
}

void call_exitlist2(void)
{
	call_exitlist();
	dos_process_exit(0);
}

static void fatal_flush_output(struct FATAL_OUTPUT_STATE* output)
{
	if (output->length != 0) {
		dos_write_stderr(output->buffer, output->length);
		output->length = 0;
	}
}

static void fatal_emit_character(struct FATAL_OUTPUT_STATE* output, legacy_s8 character)
{
	if (output->length == FATAL_OUTPUT_BUFFER_SIZE)
		fatal_flush_output(output);
	output->buffer[output->length++] = character;
}

static void fatal_emit_padding(struct FATAL_OUTPUT_STATE* output, legacy_s8 character,
	legacy_s16 count)
{
	while (count-- > 0)
		fatal_emit_character(output, character);
}

static void fatal_emit_text(struct FATAL_OUTPUT_STATE* output, const legacy_s8* text,
	legacy_s16 width, legacy_s16 precision, legacy_s16 left_aligned)
{
	legacy_s16 length;
	legacy_s16 emitted_length;

	if (text == 0)
		text = "(null)";
	for (length = 0; text[length] != 0 &&
		(precision < 0 || length < precision); length++) {
	}
	emitted_length = length;
	if (!left_aligned)
		fatal_emit_padding(output, ' ', (legacy_s16)(width - length));
	while (length-- > 0)
		fatal_emit_character(output, *text++);
	if (left_aligned)
		fatal_emit_padding(output, ' ', (legacy_s16)(width - emitted_length));
}

static void fatal_emit_number(struct FATAL_OUTPUT_STATE* output, legacy_u32 value,
	legacy_s16 negative, legacy_u16 radix, legacy_s16 uppercase,
	legacy_s16 width, legacy_s16 precision, legacy_s16 left_aligned,
	legacy_s16 zero_padded)
{
	legacy_s8 digits[FATAL_NUMBER_SCRATCH_SIZE];
	legacy_s16 digit_count;
	legacy_s16 zero_count;
	legacy_s16 space_count;
	legacy_s16 index;
	const legacy_s8* alphabet;

	alphabet = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
	digit_count = 0;
	do {
		if (radix == 0U) {
			digits[digit_count++] = alphabet[0];
			value = 0UL;
		} else {
			digits[digit_count++] = alphabet[(legacy_u16)(value % radix)];
			value = LEGACY_U32_DIV_OR_ZERO(value, radix);
		}
	} while (value != 0);
	zero_count = precision > digit_count ? precision - digit_count : 0;
	if (zero_padded && precision < 0 && !left_aligned) {
		zero_count = width - digit_count - (negative != 0);
		if (zero_count < 0)
			zero_count = 0;
	}
	space_count = width - digit_count - zero_count - (negative != 0);
	if (!left_aligned)
		fatal_emit_padding(output, ' ', space_count);
	if (negative)
		fatal_emit_character(output, '-');
	fatal_emit_padding(output, '0', zero_count);
	for (index = digit_count - 1; index >= 0; index--)
		fatal_emit_character(output, digits[index]);
	if (left_aligned)
		fatal_emit_padding(output, ' ', space_count);
}

static void fatal_vprintf(const legacy_s8* format, va_list arguments)
{
	struct FATAL_OUTPUT_STATE output;
	legacy_s16 left_aligned;
	legacy_s16 zero_padded;
	legacy_s16 width;
	legacy_s16 precision;
	legacy_s16 long_value;
	legacy_s16 value;
	legacy_s32 long_signed_value;
	legacy_u32 unsigned_value;
	legacy_s8 conversion;

	output.length = 0;
	while (*format != 0) {
		if (*format != '%') {
			fatal_emit_character(&output, *format++);
			continue;
		}
		format++;
		left_aligned = 0;
		zero_padded = 0;
		while (*format == '-' || *format == '0') {
			if (*format == '-')
				left_aligned = 1;
			else
				zero_padded = 1;
			format++;
		}
		width = 0;
		while (*format >= '0' && *format <= '9')
			width = (legacy_s16)(width * FATAL_FORMAT_DECIMAL_RADIX +
				*format++ - '0');
		precision = -1;
		if (*format == '.') {
			format++;
			precision = 0;
			while (*format >= '0' && *format <= '9')
				precision = (legacy_s16)(precision *
					FATAL_FORMAT_DECIMAL_RADIX + *format++ - '0');
		}
		long_value = 0;
		if (*format == 'l') {
			long_value = 1;
			format++;
		}
		conversion = *format;
		if (conversion != 0)
			format++;
		switch (conversion) {
		case '%':
			fatal_emit_character(&output, '%');
			break;
		case 'c':
			value = va_arg(arguments, legacy_s16);
			if (!left_aligned)
				fatal_emit_padding(&output, ' ', (legacy_s16)(width - 1));
			fatal_emit_character(&output, (legacy_s8)value);
			if (left_aligned)
				fatal_emit_padding(&output, ' ', (legacy_s16)(width - 1));
			break;
		case 's':
			fatal_emit_text(&output, va_arg(arguments, legacy_s8*), width,
				precision, left_aligned);
			break;
		case 'd':
		case 'i':
			if (long_value) {
				long_signed_value = va_arg(arguments, legacy_s32);
				unsigned_value = long_signed_value < 0 ?
					(legacy_u32)(0UL - (legacy_u32)long_signed_value) :
					(legacy_u32)long_signed_value;
				fatal_emit_number(&output, unsigned_value, long_signed_value < 0,
					FATAL_FORMAT_DECIMAL_RADIX, 0, width, precision,
					left_aligned, zero_padded);
			} else {
				value = va_arg(arguments, legacy_s16);
				unsigned_value = value < 0 ?
					(legacy_u16)(0U - (legacy_u16)value) : (legacy_u16)value;
				fatal_emit_number(&output, unsigned_value, value < 0,
					FATAL_FORMAT_DECIMAL_RADIX, 0, width, precision,
					left_aligned, zero_padded);
			}
			break;
		case 'u':
		case 'x':
		case 'X':
			unsigned_value = long_value ? va_arg(arguments, legacy_u32) :
				(legacy_u16)va_arg(arguments, legacy_u16);
			fatal_emit_number(&output, unsigned_value, 0,
				conversion == 'u' ? FATAL_FORMAT_DECIMAL_RADIX :
					FATAL_FORMAT_HEXADECIMAL_RADIX,
				conversion == 'X', width,
				precision, left_aligned, zero_padded);
			break;
		default:
			fatal_emit_character(&output, '%');
			if (conversion != 0)
				fatal_emit_character(&output, conversion);
			break;
		}
	}
	fatal_flush_output(&output);
}

void fatal_error(const legacy_s8* format, ...)
{
	va_list arguments;

	sprite_copy_2_to_1();
	va_start(arguments, format);
	fatal_vprintf(format, arguments);
	va_end(arguments);
	flush_stdin();
	call_exitlist();
	va_start(arguments, format);
	fatal_vprintf(format, arguments);
	va_end(arguments);
	dos_process_exit(1);
}
