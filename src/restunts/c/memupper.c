#ifdef RESTUNTS_DOS
#include <dos.h>
#endif

#include "externs.h"
#include "memmgr.h"

#ifdef RESTUNTS_DOS

static unsigned short upper_memory_segment;
static unsigned short upper_memory_paras;
static int upper_memory_reserved;
static unsigned short borrowed_video_limit;

static unsigned short dos_get_alloc_strategy(void) {
	unsigned short result = 0xFFFF;

	__asm {
		mov ax, 5800h
		int 21h
		jc short get_strategy_done
		xor ah, ah
		mov result, ax
	get_strategy_done:
	}
	return result;
}

static int dos_set_alloc_strategy(unsigned short strategy) {
	int result = 0;

	__asm {
		mov ax, 5801h
		mov bx, strategy
		int 21h
		jc short set_strategy_done
		mov result, 1
	set_strategy_done:
	}
	return result;
}

static unsigned short dos_get_umb_link_state(void) {
	unsigned short result = 0xFFFF;

	__asm {
		mov ax, 5802h
		int 21h
		jc short get_umb_link_done
		xor ah, ah
		mov result, ax
	get_umb_link_done:
	}
	return result;
}

static int dos_set_umb_link_state(unsigned short state) {
	int result = 0;

	__asm {
		mov ax, 5803h
		mov bx, state
		int 21h
		jc short set_umb_link_done
		mov result, 1
	set_umb_link_done:
	}
	return result;
}

static unsigned short dos_alloc_checked(unsigned short paras) {
	unsigned short result = 0;

	__asm {
		mov bx, paras
		mov ah, 48h
		int 21h
		jc short alloc_checked_done
		mov result, ax
	alloc_checked_done:
	}
	return result;
}

static void dos_free(unsigned short blockseg) {
	__asm {
		mov es, blockseg
		mov ah, 49h
		int 21h
	}
}

static unsigned short dos_alloc_upper(unsigned short paras) {
	unsigned short oldstrategy;
	unsigned short oldlinkstate;
	unsigned short result = 0;

	oldstrategy = dos_get_alloc_strategy();
	oldlinkstate = dos_get_umb_link_state();
	if (oldstrategy == 0xFFFF || oldlinkstate == 0xFFFF) {
		return 0;
	}

	/* 80h is upper-memory-only, first fit. */
	if (dos_set_umb_link_state(1) && dos_set_alloc_strategy(0x80)) {
		result = dos_alloc_checked(paras);
	}

	dos_set_alloc_strategy(oldstrategy);
	dos_set_umb_link_state(oldlinkstate);
	return result;
}

static int reserve_upper_memory(unsigned short paras) {
	if (upper_memory_segment != 0) {
		return 0;
	}

	upper_memory_segment = dos_alloc_upper(paras);
	if (upper_memory_segment == 0) {
		return 0;
	}
	upper_memory_paras = paras;
	upper_memory_reserved = 1;
	return 1;
}

static void far* claim_upper_memory(unsigned short paras) {
	if (!upper_memory_reserved || paras > upper_memory_paras) {
		return MK_FP(0, 0);
	}
	upper_memory_reserved = 0;
	return MK_FP(upper_memory_segment, 0);
}

static int is_upper_memory(void far* ptr) {
	return upper_memory_segment != 0 &&
		!upper_memory_reserved && FP_SEG(ptr) == upper_memory_segment;
}

static void free_upper_memory(void far* ptr) {
	if (!is_upper_memory(ptr)) {
		return;
	}
	dos_free(upper_memory_segment);
	upper_memory_segment = 0;
	upper_memory_paras = 0;
	upper_memory_reserved = 0;
}

static unsigned short borrow_video_memory(void) {
	unsigned short oldlimit = resendptr2->resofs;

	if (oldlimit < 0xB000) {
		resendptr2->resofs = 0xB000;
	}
	return oldlimit;
}

static void restore_video_memory(unsigned short oldlimit) {
	unsigned short usedend = resptr2->resofs + resptr2->ressize;

	if (usedend > oldlimit) {
		fatal_error("memory manager - VGA SCRATCH BLOCK DID NOT SHRINK END=%x LIMIT=%x",
			usedend, oldlimit);
	}
	resendptr2->resofs = oldlimit;
}

#endif

unsigned long mmgr_prepare_fullscreen_window(void) {
	unsigned long available = mmgr_get_res_ofs_diff_scaled();

#ifdef RESTUNTS_DOS
	if (available <= 0xFA00L && reserve_upper_memory(0xFA2)) {
		return 0xFA01L;
	}
#endif
	return available;
}

void far* mmgr_alloc_window_pages(const char* name, unsigned short pages) {
#ifdef RESTUNTS_DOS
	void far* ptr = MK_FP(0, 0);

	if (pages > mmgr_get_ofs_diff()) {
		ptr = claim_upper_memory(pages);
	}
	if (ptr) {
		return ptr;
	}
#endif
	return mmgr_alloc_pages(name, pages);
}

void mmgr_release_window(void far* ptr) {
#ifdef RESTUNTS_DOS
	if (is_upper_memory(ptr)) {
		free_upper_memory(ptr);
		return;
	}
#endif
	mmgr_release(ptr);
}

void far* mmgr_alloc_shape2d_pages(const char* name, unsigned short pages) {
#ifdef RESTUNTS_DOS
	borrowed_video_limit = 0;
	if (pages > mmgr_get_ofs_diff()) {
		borrowed_video_limit = borrow_video_memory();
	}
#endif
	return mmgr_alloc_pages(name, pages);
}

void far* mmgr_finish_shape2d_pages(void far* ptr) {
	ptr = mmgr_op_unk(ptr);
#ifdef RESTUNTS_DOS
	if (borrowed_video_limit != 0) {
		restore_video_memory(borrowed_video_limit);
		borrowed_video_limit = 0;
	}
#endif
	return ptr;
}
