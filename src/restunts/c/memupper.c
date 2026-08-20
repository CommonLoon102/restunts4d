#ifdef RESTUNTS_DOS
#include <dos.h>

/* The DOS build disables automatic C symbol underscores with /u-. */
#define intdos _intdos
#define _dos_allocmem __dos_allocmem
#define _dos_freemem __dos_freemem
int _Cdecl _intdos(union REGS* inregs, union REGS* outregs);
unsigned _Cdecl __dos_allocmem(unsigned size, unsigned* segment);
unsigned _Cdecl __dos_freemem(unsigned segment);

/* Minimal error-state globals used by the selected DOS runtime modules. */
int _errno;
int __sys_nerr = 0x30;
#endif

#include "externs.h"
#include "memmgr.h"

#ifdef RESTUNTS_DOS

static unsigned short upper_memory_segment;
static unsigned short upper_memory_paras;
static int upper_memory_reserved;
static unsigned short borrowed_video_limit;
static unsigned dos_alloc_segment_result;

static unsigned short dos_get_alloc_strategy(void) {
	union REGS inregs;
	union REGS outregs;

	inregs.x.ax = 0x5800;
	intdos(&inregs, &outregs);
	if (outregs.x.cflag != 0) {
		return 0xFFFF;
	}
	return outregs.h.al;
}

static int dos_set_alloc_strategy(unsigned short strategy) {
	union REGS inregs;
	union REGS outregs;

	inregs.x.ax = 0x5801;
	inregs.x.bx = strategy;
	intdos(&inregs, &outregs);
	return outregs.x.cflag == 0;
}

static unsigned short dos_get_umb_link_state(void) {
	union REGS inregs;
	union REGS outregs;

	inregs.x.ax = 0x5802;
	intdos(&inregs, &outregs);
	if (outregs.x.cflag != 0) {
		return 0xFFFF;
	}
	return outregs.h.al;
}

static int dos_set_umb_link_state(unsigned short state) {
	union REGS inregs;
	union REGS outregs;

	inregs.x.ax = 0x5803;
	inregs.x.bx = state;
	intdos(&inregs, &outregs);
	return outregs.x.cflag == 0;
}

static unsigned short dos_alloc_checked(unsigned short paras) {
	if (_dos_allocmem(paras, &dos_alloc_segment_result) != 0) {
		return 0;
	}
	return dos_alloc_segment_result;
}

static void dos_free(unsigned short blockseg) {
	_dos_freemem(blockseg);
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
