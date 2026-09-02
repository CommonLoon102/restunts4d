#include "legacy.h"
#include "platform.h"

#include <dos.h>

#define HEADLESS_MAX_ARGS 6
#define HEADLESS_COMMAND_LINE_SIZE 128
#define HEADLESS_STACK_PARAGRAPHS 0x200

#if defined(RESTUNTS_FULL) || defined(RESTUNTS_PIXLDUMP)
extern void full_data_initialize(void);
#endif

#ifdef RESTUNTS_FULL
extern legacy_s16 stuntsmainimpl(legacy_s16 argc, legacy_s8* argv[]);
#define dos_program_main stuntsmainimpl
#else
extern legacy_s16 stuntsmain(legacy_s16 argc, legacy_s8* argv[]);
#define dos_program_main stuntsmain
#endif
extern legacy_u8 headless_bss_start;
extern legacy_u8 headless_bss_end;
extern legacy_u8 headless_stack_top;

static legacy_u16 headless_psp_segment;
static legacy_u16 headless_program_paragraphs;
#ifdef RESTUNTS_PIXLDUMP
static legacy_s8 headless_program_name[] = "PIXLDUMP";
#else
static legacy_s8 headless_program_name[] = "REPLDUMP";
#endif
static legacy_s8 headless_command_line[HEADLESS_COMMAND_LINE_SIZE];
static legacy_s8* headless_argv[HEADLESS_MAX_ARGS];

void headless_exit(legacy_s16 result)
{
	dos_process_exit(result);
}

static void headless_release_extra_memory(void)
{
	legacy_u16 psp_segment = headless_psp_segment;
	legacy_u16 program_paragraphs = headless_program_paragraphs;

	__asm {
		mov bx, program_paragraphs
		mov es, psp_segment
		mov ah, 4Ah
		int 21h
		push ds
		pop es
	}
}

static legacy_s16 headless_parse_command_line(void)
{
	legacy_u8 far* source;
	legacy_u16 source_length;
	legacy_u16 source_index;
	legacy_u16 destination_index;
	legacy_s16 argc;
	legacy_u8 character;
	legacy_u8 quoted;

	source = (legacy_u8 far*)MK_FP(headless_psp_segment, 0x80);
	source_length = source[0];
	source_index = 1;
	destination_index = 0;
	argc = 1;
	headless_argv[0] = headless_program_name;

	while (source_index <= source_length && argc < HEADLESS_MAX_ARGS &&
		destination_index + 1U < HEADLESS_COMMAND_LINE_SIZE) {
		while (source_index <= source_length &&
			(source[source_index] == ' ' || source[source_index] == '\t'))
			source_index++;
		if (source_index > source_length)
			break;

		headless_argv[argc++] = &headless_command_line[destination_index];
		quoted = 0;
		while (source_index <= source_length) {
			character = source[source_index++];
			if (character == '"') {
				quoted ^= 1U;
				continue;
			}
			if (!quoted && (character == ' ' || character == '\t'))
				break;
			if (destination_index + 1U < HEADLESS_COMMAND_LINE_SIZE)
				headless_command_line[destination_index++] = character;
		}
		headless_command_line[destination_index++] = 0;
	}

	return argc;
}

static void headless_run(void)
{
	legacy_s16 argc;
	legacy_s16 result;

	headless_release_extra_memory();
#if defined(RESTUNTS_FULL) || defined(RESTUNTS_PIXLDUMP)
	full_data_initialize();
#endif
	argc = headless_parse_command_line();
	result = dos_program_main(argc, headless_argv);
	headless_exit(result);
}

void headless_start(void)
{
	legacy_u16 bss_length;
	legacy_u16 bss_offset;
	legacy_u16 stack_pointer;

	/* DOS enters an EXE with ES pointing at its PSP.  Borland's medium model
	 * requires SS == DS whenever a pointer to an automatic object is passed as
	 * an ordinary near pointer, so normalize both registers to DGROUP before
	 * calling any C routine.  The linker-visible STACK remains last and gives
	 * us both the initial entry stack and the final near offset. */
	bss_offset = FP_OFF(&headless_bss_start);
	bss_length = (legacy_u16)(FP_OFF(&headless_bss_end) - bss_offset);
	stack_pointer = FP_OFF(&headless_stack_top);
	__asm {
		cld
		mov bx, es
		mov cx, ss
		sub cx, bx
		add cx, HEADLESS_STACK_PARAGRAPHS
		mov dx, stack_pointer
		mov ax, seg headless_psp_segment
		mov ds, ax
		push es
		push ds
		pop es
		mov di, bss_offset
		push cx
		mov cx, bss_length
		xor ax, ax
		rep stosb
		pop cx
		pop es
		mov ax, seg headless_psp_segment
		mov headless_psp_segment, bx
		mov headless_program_paragraphs, cx
		cli
		mov ss, ax
		mov sp, dx
		sti
	}

	headless_run();
}
