/* Standalone Borland DOS regression for the offline dump timer helper.
 * Compile with bcc -c -u- -mm, then link with WLINK option start=test_start
 * plus platform/dos/build/dosstack.obj. Run DOSBox with core=dynamic and cycles=max. */
#include "../c/legacy.h"
#include "../platform/dos/dump_timer.h"
#include <dos.h>

#define TEST_PIC_MASK_PORT 33U
#define TEST_TIMER_IRQ_MASK 1U
#define TEST_OTHER_IRQ_MASK 164U
#define TEST_INTERRUPT_FLAG 512U

static const legacy_s8 passed_message[] = "Offline dump timer IRQ checks passed\r\n$";

static void test_message(const legacy_s8 *message)
{
	__asm {
		mov dx, message
		mov ah, 9
		int 21h
	}
}

#define CHECK(condition, message)                                                                  \
	do {                                                                                           \
		if (!(condition)) {                                                                        \
			test_message("FAIL: " message "\r\n$");                                                \
			return 1;                                                                              \
		}                                                                                          \
	} while (0)

static legacy_s16 test_dump_timer_irq(void)
{
	legacy_u16 original_flags;
	legacy_u16 disabled_flags;
	legacy_u16 repeated_flags;
	legacy_u16 enabled_flags;
	legacy_u8 original_mask;
	legacy_u8 disabled_mask;
	legacy_u8 repeated_mask;
	legacy_u8 enabled_mask;

	__asm {
		pushf
		pop ax
		mov original_flags, ax
		cli
	}
	original_mask = (legacy_u8)inp(TEST_PIC_MASK_PORT);
	/* Exercise mixed mask bits with IF clear, then the live DOS mask with IF set.
	 * Restore both before checking results so failures cannot leave IRQs masked. */
	outp(TEST_PIC_MASK_PORT, TEST_OTHER_IRQ_MASK);
	dump_disable_timer_irq();
	disabled_mask = (legacy_u8)inp(TEST_PIC_MASK_PORT);
	__asm {
		pushf
		pop ax
		mov disabled_flags, ax
	}
	dump_disable_timer_irq();
	repeated_mask = (legacy_u8)inp(TEST_PIC_MASK_PORT);
	__asm {
		pushf
		pop ax
		mov repeated_flags, ax
	}
	outp(TEST_PIC_MASK_PORT, original_mask & ~TEST_TIMER_IRQ_MASK);
	__asm {
		sti
	}
	dump_disable_timer_irq();
	enabled_mask = (legacy_u8)inp(TEST_PIC_MASK_PORT);
	__asm {
		pushf
		pop ax
		mov enabled_flags, ax
		cli
	}
	outp(TEST_PIC_MASK_PORT, original_mask);
	__asm {
		push original_flags
		popf
	}
	CHECK(disabled_mask == (TEST_OTHER_IRQ_MASK | TEST_TIMER_IRQ_MASK),
		  "offline capture masks only IRQ0");
	CHECK((disabled_flags & TEST_INTERRUPT_FLAG) == 0, "offline capture preserves cleared IF");
	CHECK(repeated_mask == disabled_mask && (repeated_flags & TEST_INTERRUPT_FLAG) == 0,
		  "offline capture timer mask is idempotent");
	CHECK(enabled_mask == (original_mask | TEST_TIMER_IRQ_MASK),
		  "offline capture preserves live PIC mask");
	CHECK((enabled_flags & TEST_INTERRUPT_FLAG) != 0, "offline capture preserves set IF");
	return 0;
}

void test_start(void)
{
	legacy_s16 result;

	/* DOS supplies DS as the PSP. Establish the data segment before using
	 * C globals; this test does not need the game's startup or runtime. */
	__asm {
		mov ax, seg passed_message
		mov ds, ax
	}
	result = test_dump_timer_irq();
	if (result == 0) {
		test_message(passed_message);
	}
	__asm {
		mov ax, result
		mov ah, 4ch
		int 21h
	}
	for (;;)
	{
	}
}
