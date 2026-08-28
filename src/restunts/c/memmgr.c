#ifdef RESTUNTS_DOS
#include <dos.h>
#endif
#include <stdlib.h>
#include "externs.h"
#include "memmgr.h"

#ifdef RESTUNTS_DOS

#define pushregs()\
	_asm {\
		push dx\
	}\


#define popregs()\
	_asm {\
		pop dx\
	}

void far* dos_get_psp(void) {
	legacy_u16 resseg, resofs;
	__asm {
		push ds
		mov ah, 62h
		int 21h
		mov resseg, ds
		mov resofs, bx
		pop ds
	}
	return MK_FP(resseg, resofs);
}

legacy_u16 dos_alloc(legacy_u16 paras) {
	legacy_u16 resseg;
	__asm {
		mov bx, paras
		mov ah, 48h
		int 21h
		mov resseg, ax
	}
	return resseg;
}

legacy_u16 dos_setblock(legacy_u16 blockseg, legacy_u16 newsize) {
	legacy_u16 res;
	__asm {
		mov bx, newsize
		mov es, blockseg
		mov ah, 4ah
		int 21h
		mov res, bx	// bx = max blocks
	}
	return res;
}

void dos_free(legacy_u16 blockseg) {
	__asm {
		mov es, blockseg
		mov ah, 49h
		int 21h
	}
}

// High-memory pool. A few render-only chunks are served from upper memory
// (the fixed-mapped 64k EMS page frame and/or a DOS upper memory block) so
// they stop competing with the car resources for the conventional arena
// below A000. Both regions are plain addressable RAM once set up, so far
// pointers into them behave like ordinary memory everywhere, including in
// the original asm consumers. See highpool_names for what may go here.
legacy_u16 ems_handle = 0;
legacy_u8 ems_present = 0;

typedef void (far* emsvoidfunctype)();
extern void add_exit_handler(emsvoidfunctype exitfunc);

void ems_shutdown(void) {
	legacy_u16 handle;

	if (ems_present == 0)
		return;
	ems_present = 0;
	handle = ems_handle;
	ems_handle = 0;
	__asm {
		mov ah, 45h
		mov dx, handle
		int 67h
	}
}

static void ems_init(void) {
	legacy_u16 vecseg, vecofs, frameseg, handle, stat;
	legacy_u8 pageno;
	legacy_s16 i;
	legacy_s8 far* devname;
	static legacy_s8 emmname[8] = { 'E', 'M', 'M', 'X', 'X', 'X', 'X', '0' };

	ems_present = 0;

	__asm {
		push es
		mov ax, 3567h
		int 21h
		mov vecseg, es
		mov vecofs, bx
		pop es
	}
	if (vecseg == 0 && vecofs == 0)
		return;

	// The int 67h vector points into the EMM device driver; its header
	// carries the device name at offset 0Ah.
	devname = MK_FP(vecseg, 0x0A);
	for (i = 0; i < 8; i++) {
		if (devname[i] != emmname[i])
			return;
	}

	__asm {
		mov ah, 40h
		int 67h
		mov stat, ax
	}
	if (stat & 0xFF00)
		return;

	__asm {
		mov ah, 41h
		int 67h
		mov stat, ax
		mov frameseg, bx
	}
	if (stat & 0xFF00)
		return;

	__asm {
		mov ah, 43h
		mov bx, 4
		int 67h
		mov stat, ax
		mov handle, dx
	}
	if (stat & 0xFF00)
		return;

	for (i = 0; i < 4; i++) {
		pageno = (legacy_u8)i;
		__asm {
			mov ah, 44h
			mov al, pageno
			mov bl, pageno
			xor bh, bh
			mov dx, handle
			int 67h
			mov stat, ax
		}
		if (stat & 0xFF00) {
			__asm {
				mov ah, 45h
				mov dx, handle
				int 67h
			}
			return;
		}
	}

	ems_handle = handle;
	ems_present = 1;
	add_exit_handler(ems_shutdown);
	highpool_add_block(frameseg, 0x1000, 0);
}

static void umb_init(void) {
	legacy_u16 oldstrat, oldlink, umbsize, umbseg;

	__asm {
		mov ax, 5800h
		int 21h
		mov oldstrat, ax
	}
	__asm {
		mov ax, 5802h
		int 21h
		xor ah, ah
		mov oldlink, ax
	}
	__asm {
		mov ax, 5803h
		mov bx, 1
		int 21h
	}
	__asm {
		mov ax, 5801h
		mov bx, 40h		// first fit, high memory only
		int 21h
	}

	// Probing with an impossible size makes DOS report the largest free
	// block, which under the high-only strategy is the largest UMB.
	umbsize = 0;
	__asm {
		mov ah, 48h
		mov bx, 0FFFFh
		int 21h
		mov umbsize, bx
	}

	umbseg = 0;
	if (umbsize >= 0x100)
		umbseg = dos_alloc(umbsize);

	__asm {
		mov ax, 5801h
		mov bx, oldstrat
		int 21h
	}
	__asm {
		mov ax, 5803h
		mov bx, oldlink
		int 21h
	}

	// Only accept real upper memory that cannot wrap the pool's segment
	// arithmetic. This is the one region large enough for the full-screen
	// window, so it is kept clear of small chunks.
	if (umbseg >= 0xA000 && umbseg < 0xF000 &&
	    (legacy_u32)umbseg + umbsize <= 0xF000) {
		highpool_add_block(umbseg, umbsize, 0);
	} else if (umbseg > 0x10) {
		dos_free(umbseg);
	}
}

void himem_init(void) {
	ems_init();
	umb_init();
	{ extern void highpool_reserve_window(void); highpool_reserve_window(); }
}

// The colour text page is 32k of real memory that nothing reads while the
// game is in mode 13h: the framebuffer is at A000 and BIOS teletype output
// follows the active mode, so B800 just sits there. Claim it only once the
// video mode has actually been set, and only for mode 13h - in a text or
// CGA mode those same bytes are the visible screen. Tools that keep writing
// to the console (repldump) therefore never gain this block, which is why
// the replay harness cannot exercise it.
#else
void pushregs() {}
void popregs() {}
	
size_t word_3FF82 = 0; // last para reserved by memmgr
size_t word_3FF84 = 0; // first para reserved by memmgr
legacy_u16 resmaxsize = 0; // size of largest chunk?

struct MEMCHUNK resources[] = {
	{ 0, 0, 0, 2 },
	{ 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, 
	{ 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, 
	{ 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, 
	{ 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, 

	{ 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, 
	{ 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, 
	{ 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, 
	{ 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, 

	{ 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, 
	{ 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, 
	{ 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, 
	{ 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, 
	{ 0, 0, 0, 1 },
};
struct MEMCHUNK* resendptr1 = &resources[49]; // eller 49?
struct MEMCHUNK* resendptr2 = &resources[49]; // ditto
struct MEMCHUNK* resptr1 = resources;
struct MEMCHUNK* resptr2 = resources;

// No upper memory outside DOS; every chunk stays in the regular arena.
void ems_shutdown(void) {}
void himem_init(void) {}

#endif

#ifdef RESTUNTS_DOS

// High-memory pool core. The pool is a set of fixed upper-memory blocks with
// a small chunk table, kept fully separate from the resources[] arena table
// whose invariants assume one contiguous range. Chunks are diverted into the
// pool by name in mmgr_alloc_pages; every other memory manager entry point
// recognizes pool chunks by segment and handles them in place (no cache
// moves and no compaction - all pool consumers hold position-independent far
// pointers). With no blocks added the pool is inert and every call falls
// through to the original arena behavior.
#define HIGHPOOL_MAXBLOCKS 6
#define HIGHPOOL_MAXCHUNKS 32

struct HIGHBLOCK {
	legacy_u16 blockseg;
	legacy_u16 blockparas;
	legacy_u16 blocklarge; // 1 = video memory, full-screen window only
};

struct HIGHCHUNK {
	legacy_s8 resname[12];
	legacy_u16 resseg;
	legacy_u16 resparas;
	legacy_u16 resstate; // 0 = free, 1 = cached, 2 = live, 3 = reserved
};

// The full-screen window is the largest single allocation the game makes and
// the last one it asks for, by which time the pool is full of smaller
// resources and the arena has been whittled down - so it is the one that
// fails. Set its space aside before anything else can take it. 0xFA2 paras
// is 320x200 plus the bitmap header.
#define HIGHPOOL_WINDOW_PARAS 0xFA2
#define HIGHPOOL_WINDOW_NAME "MCGA WINDOW"

static struct HIGHCHUNK* highpool_reserved_window(void);

static struct HIGHBLOCK highblocks[HIGHPOOL_MAXBLOCKS];
static struct HIGHCHUNK highchunks[HIGHPOOL_MAXCHUNKS];
static legacy_s16 highblockcount = 0;

// Chunks allowed in upper memory. The list is deliberately short: it was
// established by replay regression, not by reasoning about what "looks"
// render-only. Anything whose bytes can be observed before being written
// must stay in the conventional arena, because the arena position it would
// have taken holds remnants of earlier chunks and the original executable
// observes exactly those remnants.
//
// Known to break replays if moved here, do not add them back:
//  - "trakdata": its address participates in legacy stack residue, and its
//    23 sub-blocks must also stay one contiguous allocation.
//  - "cvx": init_game_state only clears one field per entry, so
//    restore_gamestate copies bytes that were never written.
//  - "*.vce"/"*.sfx"/"*.drv" and the car "st????.3sh"/".p3s" containers.
static const legacy_s8* highpool_names[] = {
	"MCGA WINDOW",
	"polyinfo",
	"sdgame",
	"main.res",
	"fontdef.fnt",
	"fontn.fnt",
	"fontled.fnt",
	"game.pre",
	"game.res",
	"game1.p3s",
	"game2.p3s",
	"game1.3sh",
	"game2.3sh",
	"sdgame2.PVS",
	"city.PVS",
	"desert.PVS",
	"alpine.PVS",
	"country.PVS",
	"tropical.PVS",
	0
};

void highpool_add_block(legacy_u16 seg, legacy_u16 paras, legacy_u16 largeonly) {
	legacy_u16 p;
	legacy_u16 far* wipe;
	legacy_s16 i, k;

	if (paras == 0)
		return;

	// Never hand the same paragraphs out twice: DOS may offer a block that
	// already belongs to the pool.
	for (k = 0; k < highblockcount; k++) {
		if (seg < highblocks[k].blockseg + highblocks[k].blockparas &&
		    highblocks[k].blockseg < seg + paras)
			return;
	}

	if (highblockcount >= HIGHPOOL_MAXBLOCKS)
		return;

	// Fresh conventional memory is zero-filled at boot; give the pool the
	// same starting content so reads of never-written chunk bytes see the
	// same values as they would in the regular arena. Video memory is left
	// alone: it holds the visible screen, and its only tenant overwrites it
	// completely anyway.
	if (!largeonly) {
		for (p = 0; p < paras; p++) {
			wipe = MK_FP(seg + p, 0);
			for (i = 0; i < 8; i++)
				wipe[i] = 0;
		}
	}

	highblocks[highblockcount].blockseg = seg;
	highblocks[highblockcount].blockparas = paras;
	highblocks[highblockcount].blocklarge = largeonly;
	highblockcount++;
}

legacy_s16 highpool_owns_seg(legacy_u16 seg) {
	legacy_s16 i;
	for (i = 0; i < highblockcount; i++) {
		if (seg >= highblocks[i].blockseg &&
		    seg < highblocks[i].blockseg + highblocks[i].blockparas)
			return 1;
	}
	return 0;
}

static struct HIGHCHUNK* highpool_chunk_by_seg(legacy_u16 seg) {
	legacy_s16 i;
	for (i = 0; i < HIGHPOOL_MAXCHUNKS; i++) {
		if (highchunks[i].resstate != 0 && highchunks[i].resseg == seg)
			return &highchunks[i];
	}
	return 0;
}

legacy_s16 highpool_route(const legacy_s8* name, legacy_u16 paras) {
	legacy_s16 i, j;
	const legacy_s8* entry;

	if (highblockcount == 0)
		return 0;

	// Menus and instruments create many small windows under the same name.
	// Pool space is zero-sum, and giving it to them only displaces bigger
	// chunks, so only the full-screen window is worth diverting.
	if (paras < 0xF00 && name[0] == 'M' && name[1] == 'C')
		return 0;

	for (i = 0; highpool_names[i] != 0; i++) {
		entry = highpool_names[i];
		for (j = 0; ; j++) {
			if (entry[j] != name[j])
				break;
			if (entry[j] == 0)
				return 1;
		}
	}
	return 0;
}

// First fit inside one block: bump the candidate past any conflicting chunk
// until the request fits or the block ends.
static legacy_u16 highpool_find_gap(struct HIGHBLOCK* block, legacy_u16 paras,
                                        legacy_s16 dropcached) {
	legacy_u16 cand, blockend;
	legacy_s16 i, conflict;

	cand = block->blockseg;
	blockend = block->blockseg + block->blockparas;

	for (;;) {
		if (paras > blockend - cand || cand >= blockend)
			return 0;
		conflict = 0;
		for (i = 0; i < HIGHPOOL_MAXCHUNKS; i++) {
			if (highchunks[i].resstate == 0)
				continue;
			if (dropcached && highchunks[i].resstate == 1)
				continue; // discardable, so it does not block the search
			if (highchunks[i].resseg < cand + paras &&
			    cand < highchunks[i].resseg + highchunks[i].resparas) {
				cand = highchunks[i].resseg + highchunks[i].resparas;
				conflict = 1;
				break;
			}
		}
		if (!conflict)
			return cand;
	}
}

// Discard cached chunks overlapping a placement, exactly as the arena
// allocator drops its own cached blocks when it needs the room back.
static void highpool_drop_cached(legacy_u16 seg, legacy_u16 paras) {
	legacy_s16 i;
	for (i = 0; i < HIGHPOOL_MAXCHUNKS; i++) {
		if (highchunks[i].resstate != 1)
			continue;
		if (highchunks[i].resseg < seg + paras &&
		    seg < highchunks[i].resseg + highchunks[i].resparas)
			highchunks[i].resstate = 0;
	}
}

// Does a chunk of this size fit at exactly this segment?
static legacy_s16 highpool_fits_at(legacy_u16 seg, legacy_u16 paras) {
	legacy_s16 i, b, inblock;

	inblock = 0;
	for (b = 0; b < highblockcount; b++) {
		if (seg >= highblocks[b].blockseg &&
		    seg + paras <= highblocks[b].blockseg + highblocks[b].blockparas) {
			inblock = 1;
			break;
		}
	}
	if (!inblock)
		return 0;

	for (i = 0; i < HIGHPOOL_MAXCHUNKS; i++) {
		if (highchunks[i].resstate == 0)
			continue;
		if (highchunks[i].resseg == seg)
			continue;
		if (highchunks[i].resseg < seg + paras &&
		    seg < highchunks[i].resseg + highchunks[i].resparas)
			return 0;
	}
	return 1;
}

legacy_s16 highpool_can_fit(legacy_u16 paras) {
	legacy_s16 i;

	if (paras <= HIGHPOOL_WINDOW_PARAS && highpool_reserved_window() != 0)
		return 1;

	for (i = 0; i < highblockcount; i++) {
		if (highblocks[i].blocklarge && paras < 0xF00)
			continue;
		if (highpool_find_gap(&highblocks[i], paras, 1) != 0)
			return 1;
	}
	return 0;
}

static struct HIGHCHUNK* highpool_reserved_window(void) {
	legacy_s16 i;
	for (i = 0; i < HIGHPOOL_MAXCHUNKS; i++) {
		if (highchunks[i].resstate == 3)
			return &highchunks[i];
	}
	return 0;
}

void highpool_reserve_window(void) {
	struct HIGHCHUNK* slot = 0;
	legacy_u16 seg;
	legacy_s16 i, b;

	for (i = 0; i < HIGHPOOL_MAXCHUNKS; i++) {
		if (highchunks[i].resstate == 0) {
			slot = &highchunks[i];
			break;
		}
	}
	if (slot == 0)
		return;

	for (b = 0; b < highblockcount; b++) {
		seg = highpool_find_gap(&highblocks[b], HIGHPOOL_WINDOW_PARAS, 0);
		if (seg != 0) {
			const legacy_s8* nm = HIGHPOOL_WINDOW_NAME;
			for (i = 0; i < 12; i++) {
				slot->resname[i] = nm[i];
				if (nm[i] == 0)
					break;
			}
			for (; i < 12; i++)
				slot->resname[i] = 0;
			slot->resseg = seg;
			slot->resparas = HIGHPOOL_WINDOW_PARAS;
			slot->resstate = 3;
			return;
		}
	}
}

void far* highpool_alloc(const legacy_s8* name, legacy_u16 paras) {
	legacy_s16 i, b, dropcached;
	legacy_u16 seg;
	struct HIGHCHUNK* slot = 0;

	// Claim the standing reservation rather than hunting for a gap.
	if (paras <= HIGHPOOL_WINDOW_PARAS) {
		struct HIGHCHUNK* res = highpool_reserved_window();
		if (res != 0) {
			const legacy_s8* nm = HIGHPOOL_WINDOW_NAME;
			for (i = 0; ; i++) {
				if (nm[i] == 0 && (name[i] == 0 || i == 12)) {
					res->resstate = 2;
					return MK_FP(res->resseg, 0);
				}
				if (i == 12 || nm[i] != name[i])
					break;
			}
		}
	}

	for (i = 0; i < HIGHPOOL_MAXCHUNKS; i++) {
		if (highchunks[i].resstate == 0) {
			slot = &highchunks[i];
			break;
		}
	}
	if (slot == 0)
		return MK_FP(0, 0);

	// Best fit rather than first fit: put each chunk in the tightest block
	// that still holds it, so one large run of free space stays intact for
	// the chunks that actually need it (above all the 62k window). A first
	// pass leaves cached chunks alone so they can still be revived; only if
	// nothing fits are they discarded to make room.
	for (dropcached = 0; dropcached < 2; dropcached++) {
		legacy_u16 bestseg = 0, bestslack = 0;
		legacy_s16 bestb = -1;
		for (b = 0; b < highblockcount; b++) {
			legacy_u16 cand, slack;
			if (highblocks[b].blocklarge && paras < 0xF00)
				continue;
			cand = highpool_find_gap(&highblocks[b], paras, dropcached);
			if (cand == 0)
				continue;
			slack = highblocks[b].blockseg + highblocks[b].blockparas - cand - paras;
			if (bestb < 0 || slack < bestslack) {
				bestb = b;
				bestseg = cand;
				bestslack = slack;
			}
		}
		if (bestb >= 0) {
			b = bestb;
			seg = bestseg;
			if (dropcached)
				highpool_drop_cached(seg, paras);
			break;
		}
		b = highblockcount;
		seg = 0;
	}

	for (; b < highblockcount; b++) {
		if (seg != 0) {
			for (i = 0; i < 12; i++) {
				slot->resname[i] = name[i];
				if (name[i] == 0)
					break;
			}
			for (; i < 12; i++)
				slot->resname[i] = 0;
			slot->resseg = seg;
			slot->resparas = paras;
			slot->resstate = 2;
			return MK_FP(seg, 0);
		}
	}
	return MK_FP(0, 0);
}

// Cache lookup mirroring mmgr_get_chunk_by_name: a cached pool chunk is
// revived in place instead of being copied back into the arena.
void far* highpool_get_by_name(const legacy_s8* name) {
	legacy_s16 i, j, found;
	struct HIGHCHUNK* chunk;

	for (i = 0; i < HIGHPOOL_MAXCHUNKS; i++) {
		chunk = &highchunks[i];
		if (chunk->resstate != 1)
			continue;
		found = 0;
		for (j = 0; j < 12; j++) {
			if (name[j] == 0) {
				if (chunk->resname[j] == '.' || chunk->resname[j] == 0)
					found = 1;
				break;
			}
			if (name[j] != chunk->resname[j])
				break;
		}
		if (j == 12)
			found = 1;
		if (found) {
			chunk->resstate = 2;
			return MK_FP(chunk->resseg, 0);
		}
	}
	return MK_FP(0, 0);
}

#else

// Upper memory is a DOS notion; elsewhere every request simply falls through
// to the ordinary arena.
void highpool_add_block(legacy_u16 seg, legacy_u16 paras, legacy_u16 largeonly) {
	(void)seg; (void)paras; (void)largeonly;
}

legacy_s16 highpool_owns_seg(legacy_u16 seg) {
	(void)seg;
	return 0;
}

legacy_s16 highpool_route(const legacy_s8* name, legacy_u16 paras) {
	(void)name; (void)paras;
	return 0;
}

legacy_s16 highpool_can_fit(legacy_u16 paras) {
	(void)paras;
	return 0;
}

void far* highpool_alloc(const legacy_s8* name, legacy_u16 paras) {
	(void)name; (void)paras;
	return 0;
}

void far* highpool_get_by_name(const legacy_s8* name) {
	(void)name;
	return 0;
}

#endif

const legacy_s8* mmgr_path_to_name(const legacy_s8* filename) {
	const legacy_s8* c;
	const legacy_s8* result;

	pushregs();
	
	result = filename;
	for (c = filename; *c; c++) {
		if (*c == ':' || *c == '\\') 
			result = c + 1;
	}
	
	popregs();
	return result;
}

extern void far* ported_mmgr_alloc_pages_(const legacy_s8* arg_0, legacy_u16 arg_2);

void far* mmgr_alloc_pages(const legacy_s8* arg_0, legacy_u16 arg_2) {
	legacy_s16 i;
	struct MEMCHUNK* resdi;
	struct MEMCHUNK* ressi;
	const legacy_s8* chunkname;
	legacy_u16 rax, rdx;

#ifdef RESTUNTS_DOS
	if (highpool_route(mmgr_path_to_name(arg_0), arg_2)) {
		void far* highptr = highpool_alloc(mmgr_path_to_name(arg_0), arg_2);
		if (FP_SEG(highptr) != 0)
			return highptr;
		// The pool is full; fall through to the regular arena.
	}
#endif

	resdi = resptr2;
	ressi = resendptr1;
	rdx = resdi->resofs + resdi->ressize;

	resdi++;
	if (ressi <= resdi) {
		if (ressi == resendptr2) 
			fatal_error("reservememory - OUT OF MEMORY SLOTS RESERVING %s", arg_0);

		ressi++;
		resendptr1 = ressi;
	}

	resptr2 = resdi;
	chunkname = mmgr_path_to_name(arg_0);
	for (i = 0; i < 0xC; i++)
		resdi->resname[i] = chunkname[i];

	rax = arg_2;
	resdi->resofs = rdx;
	resdi->ressize = rax;
	resdi->resunk = 2;

	rax += rdx;
	if (rax > resmaxsize) 
		resmaxsize = rax;

	if (rax > ressi->resofs) {
		ressi = resendptr1;
		resdi = resptr2;
		rax = resdi->resofs + resdi->ressize;
	
		while (rax > ressi->resofs) {
			if (ressi == resendptr2) {
				fatal_error("reservememory - OUT OF MEMORY RESERVING %s P=%x HW=%x\r\n", arg_0, resdi->ressize, resmaxsize);
			}

			ressi->resunk = 0;
			ressi++;
			resendptr1 = ressi;
		}
	}

	return MK_FP(rdx, 0);
}

void far* mmgr_alloc_resbytes(const legacy_s8* name, legacy_s32 size) {
	/* The original allocator always reserves one paragraph after division. */
	return mmgr_alloc_pages(name, size / 16 + 1);
}

void mmgr_alloc_resmem(legacy_u16 arg_0) {

	void far* psp;
	legacy_u16 maxblocks;
	struct MEMCHUNK* rp;
	legacy_s8* tempptr;

#ifdef RESTUNTS_DOS
	psp = dos_get_psp();
	pspseg = FP_SEG(psp);
	pspofs = FP_OFF(psp);
	
	if (word_3FF82 == 0) {
		resptr1->resofs = dos_alloc(0x64);
		word_3FF84 = resptr1->resofs;
		maxblocks = dos_setblock(resptr1->resofs, arg_0 - resptr1->resofs);
		maxblocks = dos_setblock(resptr1->resofs, maxblocks);
		resendptr2->resofs = word_3FF84 + maxblocks;
		word_3FF82 = resendptr2->resofs;
		//fatal_error("%u\n", word_3FF82 - word_3FF84);
	}
#else
	if (word_3FF82 == 0) {
		// assume 640k is enough for anybody:
		maxblocks = (640 * 1024) >> 4;
		tempptr = malloc((maxblocks << 4) + 16);
		resptr1->resofs = (((size_t)tempptr) + 16)>>4;
		word_3FF84 = resptr1->resofs;
		resendptr2->resofs = word_3FF84 + maxblocks;
		word_3FF82 = resendptr2->resofs;
		
	}
#endif
	resendptr1 = resendptr2;
	resptr2 = resptr1;
	
	rp = resptr1;
	for (;;) {
		rp++;
		if (rp == resendptr2) break;
		rp->resunk = 0;
	}
}

void mmgr_alloc_a000(void) {
	mmgr_alloc_resmem(0xa000);
}

legacy_u16 mmgr_get_ofs_diff(void) {
	return resendptr2->resofs - resptr2->resofs - resptr2->ressize;
}

legacy_u16 nopsub_31157(void) {
	return mmgr_get_ofs_diff();
}

legacy_u16 nopsub_31169(void) {
	return (legacy_u16)(resptr2->resofs + resptr2->ressize - resptr1->resofs);
}

void far* mmgr_free(legacy_s8 far* ptr) {
	legacy_s16 i;
	legacy_u16 ax, bx, cx, dx, di;
	legacy_u16 ptrseg;
	struct MEMCHUNK* ressi;
	struct MEMCHUNK* resbx;

	ressi = resptr2;
	ptrseg = FP_SEG(ptr);

#ifdef RESTUNTS_DOS
	if (highpool_owns_seg(ptrseg)) {
		struct HIGHCHUNK* highchunk = highpool_chunk_by_seg(ptrseg);
		if (highchunk == 0)
			fatal_error("memory manager - BLOCK NOT FOUND at SEG= %x", ptrseg);
		highchunk->resstate =
			(highchunk->resparas == HIGHPOOL_WINDOW_PARAS) ? 3 : 1;
		return MK_FP(ptrseg, FP_OFF(ptr));
	}
#endif

	while (1) {
		if (ressi == resptr1) 
			fatal_error("memory manager - BLOCK NOT FOUND at SEG= %x", ptrseg);
		if (ressi->resofs == ptrseg) break;
		ressi--;
	}

	ptrseg = 0;
	ressi->resunk = 0;
	if (ressi != resptr2) {
		if (ressi == resendptr1) goto loc_31508;
		ax = resendptr1->resofs - resptr2->resofs - resptr2->ressize;
		if (ax < ressi->ressize) goto loc_31508;
	}

	ptrseg = resendptr1->resofs - ressi->ressize;
	resendptr1--;
	resendptr1->resofs = ptrseg;
	resendptr1->ressize = ressi->ressize;
	resendptr1->resunk = 1;

	for (i = 0; i < 0xC; i++) {
		resendptr1->resname[i] = ressi->resname[i];
	}

	copy_paras_reverse(ressi->resofs, ptrseg, ressi->ressize);

loc_31508:
	if (ressi == resptr2) {
		do {
			ressi--;
		} while (ressi->resunk == 0);
		resptr2 = ressi;
	}

	return MK_FP(ptrseg, FP_OFF(ptr));
}

// `paras` is signed here but the original treats it as unsigned: it steps the
// count down with `sub bx, 1000h` and tests the borrow with jnb. Counts from
// 8000h through 8FFFh wrap into the non-negative range after that subtraction
// and still chunk correctly here; the divergence starts at 9000h (576 KiB),
// where the signed remainder test is taken despite no unsigned borrow. Nothing
// asks either copier for that much at once.
void mmgr_copy_paras(legacy_u16 srcseg, legacy_u16 destseg, legacy_s16 paras) {
	legacy_u16 count; // number of words to copy
	legacy_u16 far * srcptr;
	legacy_u16 far * destptr;
	
	while (paras != 0) {
		count = 0x8000; // 64k in words
		paras -= 0x1000; // 64k in paras
		if (paras < 0) {
			count = (paras + 0x1000) << 3;  // count = remaining paras < 0x1000 in words
			paras = 0;
		}
		srcptr = MK_FP(srcseg, 0);
		destptr = MK_FP(destseg, 0);

		while (count) {
			*destptr = *srcptr;
			srcptr++;
			destptr++;
			count--;
		}

		srcseg += 0x1000;
		destseg += 0x1000;
	}
}


// Same signedness caveat as mmgr_copy_paras above: the original's loop guard
// is `sub bx, 1000h / jnb`, an unsigned count.
void copy_paras_reverse(legacy_u16 srcseg, legacy_u16 destseg, legacy_s16 paras) {
	legacy_u16 count, ofs;
	legacy_u16 far* destptr;
	legacy_u16 far* srcptr;

	pushregs();

	srcseg += paras;
	destseg += paras;

	while (paras != 0) {
		count = 0x1000;
		paras -= 0x1000;
		if (paras < 0) {
			count = paras + 0x1000;
			paras = 0;
		}
		srcseg -= count;
		destseg -= count;
		count <<= 3;
		ofs = (count << 1) - 2;

		srcptr = MK_FP(srcseg, ofs);
		destptr = MK_FP(destseg, ofs);
		while (count) {
			*destptr = *srcptr;
			srcptr--;
			destptr--;
			count--;
		}
	}
	popregs();
}

void mmgr_find_free(void) {
	legacy_s16 i;
	legacy_u16 regax, regdx, resunk;
	struct MEMCHUNK* ressi;
	struct MEMCHUNK* resdi;

	pushregs();

	ressi = resendptr2;
	resdi = ressi;
	regdx = 0;

	do {
		if ((ressi->resunk & 1) == 0) {
			regdx += ressi->ressize;
		} else {
		
			if (regdx != 0) {
				resdi++;
				regax = resdi->resofs - ressi->ressize;
				resdi--;
				resdi->ressize = ressi->ressize;
				resdi->resofs = regax;
				resunk = ressi->resunk;
				ressi->resunk = 0;
				resdi->resunk = resunk;
				for (i = 0; i < 0xC; i++) {
					resdi->resname[i] = ressi->resname[i];
				}
				copy_paras_reverse(ressi->resofs, regax, ressi->ressize);
			}
		
			resdi--;
		}
		ressi--;
	// `cmp si, resendptr1 / jnb` - the entry at resendptr1 is the last one
	// the original visits, not one past the end.
	} while (ressi >= resendptr1);

	resdi++;
	resendptr1 = resdi;

	popregs();
}

void far* ported_mmgr_get_chunk_by_name_(const legacy_s8* name);

void far* mmgr_get_chunk_by_name(const legacy_s8* name) {
	const legacy_s8* pcdi;
	legacy_s16 regbx, regax;
	legacy_u16 srcofs, srcsize, destofs;
	struct MEMCHUNK* ressi;
	struct MEMCHUNK* resdi;
	legacy_s16 found = 0;
	
	pcdi = mmgr_path_to_name(name);

#ifdef RESTUNTS_DOS
	{
		void far* highptr = highpool_get_by_name(pcdi);
		if (FP_SEG(highptr) != 0)
			return highptr;
	}
#endif

	ressi = resendptr1;

	for (; ressi < resendptr2; ressi++) {
		regbx = 0;
		if (ressi->resunk == 0) {
			return MK_FP(0, 0);
		}

		for (; regbx < 0xC; regbx++) {
			if (pcdi[regbx] == 0) {
				if (ressi->resname[regbx] == '.' || ressi->resname[regbx] == 0) {
					found = 1;
				}
				break;
			}
			if (pcdi[regbx] != ressi->resname[regbx])
				break;
		}
		if (regbx == 0xC || found == 1) {
			/* Restore the cached block exactly as the original allocator does. */
			srcofs = ressi->resofs;
			srcsize = ressi->ressize;
			destofs = resptr2->resofs + resptr2->ressize;
			ressi->resunk = 0;
			resdi = resptr2 + 1;
			resptr2 = resdi;
			resdi->resofs = destofs;
			resdi->ressize = srcsize;
			resdi->resunk = 2;
			memcpy(resdi->resname, ressi->resname, sizeof(legacy_s8[12]));
			if (resdi == resendptr1) {
				resendptr1++;
			}
			mmgr_copy_paras(srcofs, destofs, srcsize);
			regax = destofs + srcsize;
			while (regax > resendptr1->resofs) {
				resendptr1->resunk = 0;
				resendptr1++;
			}
			mmgr_find_free();
			return MK_FP(resdi->resofs, 0);
		}

	}

	return MK_FP(0, 0);
}

legacy_u16 nopsub_31429(const legacy_s8* name) {
	const legacy_s8* wanted;
	legacy_s16 i;
	struct MEMCHUNK* chunk;

	wanted = mmgr_path_to_name(name);
	chunk = resendptr1;
	while (chunk < resendptr2) {
		if (chunk->resunk == 0)
			return 0;
		for (i = 0; i < 12; i++) {
			if (wanted[i] == 0) {
				if (chunk->resname[i] == '.' || chunk->resname[i] == 0)
					return 1;
				break;
			}
			if (wanted[i] != chunk->resname[i])
				break;
		}
		if (i == 12)
			return 1;
		chunk++;
	}
	return 0;
}

void mmgr_release(legacy_s8 far* ptr) {
	legacy_s16 i;
	legacy_u16 regax, regbx, regcx, regdx;
	legacy_s8* strdi;
	struct MEMCHUNK* ressi;
	struct MEMCHUNK* resdi;

	pushregs();
	__asm {
		push bx
	}
	
	regax = FP_SEG(ptr);
	ressi = resptr2;

#ifdef RESTUNTS_DOS
	if (highpool_owns_seg(regax)) {
		struct HIGHCHUNK* highchunk = highpool_chunk_by_seg(regax);
		if (highchunk == 0)
			fatal_error("memory manager - BLOCK NOT FOUND at SEG= %x", regax);
		highchunk->resstate =
			(highchunk->resparas == HIGHPOOL_WINDOW_PARAS) ? 3 : 0;
		__asm {
			pop bx
		}
		popregs();
		return;
	}
#endif

	for (;;) {
		if (ressi == resptr1) 
			fatal_error("memory manager - BLOCK NOT FOUND at SEG= %x", regax);
		if (regax == ressi->resofs) break;
		ressi--;
	}
	
	ressi->resunk = 0;
	if (ressi == resptr2) {
		do {
			ressi--;
		} while (ressi->resunk == 0);
		resptr2 = ressi;
	}

	__asm {
		pop bx
	}
	popregs();
}

// Rename a live arena chunk, so a buffer filled under one name can be handed
// on under the name the caller expects.
void mmgr_rename_chunk(legacy_s8 far* ptr, const legacy_s8* name) {
	legacy_s16 i;
	legacy_u16 regax;
	const legacy_s8* chunkname;
	struct MEMCHUNK* ressi;

	regax = FP_SEG(ptr);
	ressi = resptr2;

#ifdef RESTUNTS_DOS
	if (highpool_owns_seg(regax))
		return;
#endif

	for (;;) {
		if (ressi == resptr1)
			fatal_error("memory manager - BLOCK NOT FOUND at SEG= %x", regax);
		if (regax == ressi->resofs) break;
		ressi--;
	}

	chunkname = mmgr_path_to_name(name);
	for (i = 0; i < 0xC; i++)
		ressi->resname[i] = chunkname[i];
}

legacy_u16 mmgr_get_chunk_size(legacy_s8 far* ptr) {
	legacy_s16 i;
	legacy_u16 regax, regbx, regcx, regdx;
	legacy_s8* strdi;
	struct MEMCHUNK* ressi;
	struct MEMCHUNK* resdi;

	regax = FP_SEG(ptr);
	ressi = resptr2;

#ifdef RESTUNTS_DOS
	if (highpool_owns_seg(regax)) {
		struct HIGHCHUNK* highchunk = highpool_chunk_by_seg(regax);
		if (highchunk == 0)
			fatal_error("memory manager - BLOCK NOT FOUND at SEG= %x", regax);
		return highchunk->resparas;
	}
#endif

	for (;;) {
		if (ressi == resptr1) 
			fatal_error("memory manager - BLOCK NOT FOUND at SEG= %x", regax);
		if (regax == ressi->resofs) break;
		ressi--;
	}
	return ressi->ressize;
}

legacy_u16 mmgr_resize_memory(legacy_u16 arg_0, legacy_u16 arg_2, legacy_u16 arg_4) {
	legacy_s16 i;
	legacy_u16 regax, regbx, regcx, regdx;
	legacy_s8* strdi;
	struct MEMCHUNK* ressi;
	struct MEMCHUNK* resdi;

	pushregs();

	(void)arg_0;
	regax = arg_2;
	ressi = resptr2;

#ifdef RESTUNTS_DOS
	if (highpool_owns_seg(regax)) {
		struct HIGHCHUNK* highchunk = highpool_chunk_by_seg(regax);
		if (highchunk == 0)
			fatal_error("memory manager - BLOCK NOT FOUND at SEG= %x", arg_2);
		if (arg_4 <= highchunk->resparas) {
			highchunk->resparas = arg_4;
			popregs();
			return arg_4;
		}
		if (highpool_fits_at(regax, arg_4)) {
			highchunk->resparas = arg_4;
			popregs();
			return 0;
		}
		fatal_error("resizememory - NO MEMORY LEFT TO EXPAND HW=%x", resmaxsize);
	}
#endif

	for (;;) {
		if (ressi == resptr1)
			fatal_error("memory manager - BLOCK NOT FOUND at SEG= %x", arg_2);
		if (regax == ressi->resofs) break;
		ressi--;
	}

	regax = arg_4;
	if (regax <= ressi->ressize) {
		ressi->ressize = regax;
		popregs();
		return regax;
	}

	if (ressi != resptr2)
		fatal_error("resizememory - CANNOT EXPAND BLOCK NOT AT TOP");
	ressi->ressize = regax;
	resdi = resendptr1;
	regax += ressi->resofs;
	if (regax >= resmaxsize)
		resmaxsize = regax;

	if (regax <= resdi->resofs) {
		popregs();
		return 0;
	}

	ressi = resendptr1;
	resdi = resptr2;
	regax = resdi->resofs + resdi->ressize;

	for (;;) {
		if (regax <= ressi->resofs) break;
		if (ressi == resendptr2) 
			fatal_error("resizememory - NO MEMORY LEFT TO EXPAND HW=%x", resmaxsize);
	
		ressi->resunk = 0;
		ressi++;
		resendptr1 = ressi;
	}
	popregs();
	return 0;
}

void far* mmgr_op_unk(legacy_s8 far* ptr) {
	legacy_s16 i;
	legacy_u16 regax, regbx, regcx, regdx;
	legacy_s8* strdi;
	struct MEMCHUNK* ressi;
	struct MEMCHUNK* resdi;

	regax = FP_SEG(ptr);
	ressi = resptr2;

#ifdef RESTUNTS_DOS
	if (highpool_owns_seg(regax)) {
		if (highpool_chunk_by_seg(regax) == 0)
			fatal_error("memory manager - BLOCK NOT FOUND at SEG= %x", regax);
		// Pool chunks are never compacted; they stay where they are.
		return MK_FP(regax, 0);
	}
#endif

	for (;;) {
		if (ressi == resptr1)
			fatal_error("memory manager - BLOCK NOT FOUND at SEG= %x", regax);
		if (regax == ressi->resofs) break;
		ressi--;
	}

	resdi = ressi;
	resdi--;
	if (resdi->resunk == 0) {
	
		do {
			resdi--;
		} while (resdi->resunk == 0);
	
		ressi->resunk = 0;
		regax = resdi->resofs + resdi->ressize;
		resdi++;
		if (ressi == resptr2) {
			resptr2 = resdi;
		}
	
		resdi->resofs = regax;
		resdi->ressize = ressi->ressize;
		resdi->resunk = 2;
		for (i = 0; i < 0xC; i++) {
			resdi->resname[i] = ressi->resname[i];
		}
		mmgr_copy_paras(ressi->resofs, regax, ressi->ressize);

	} else {
		resdi = ressi;
	}

	return MK_FP(resdi->resofs, 0);
}

legacy_u32 mmgr_get_res_ofs_diff_scaled(void) {
	legacy_u32 result = mmgr_get_ofs_diff();
	return result << 4;
}

legacy_u32 mmgr_get_chunk_size_bytes(legacy_s8 far* ptr) {
	legacy_u32 result = mmgr_get_chunk_size(ptr);
	return result << 4;
}
//#endif


legacy_s8 far* locate_resource(legacy_s8 far* data, legacy_s8* name, legacy_u16 fatal) {
	legacy_u16 chunk_count, i, j;
	legacy_s8 far* resnames = (legacy_s8 far*)data + 6; // point at first 4-byte resource identifier
	legacy_s8 huge* result = data; // cannot add >64k on a far pointer, use a huge pointer instead
	legacy_u8 far* offset;

	chunk_count = LEGACY_READ_U16_LE((legacy_u8 far*)data + 4);

	//printf("locate_resource: %s\n", name);

	// pad name with spaces
	for (i = 0; i < 4; i++) {
		if (name[i] == 0) {
			for (; i < 4; i++) {
				name[i] = 0x20;
			}
			break;
		}
	}

	// The original runs this compare chunks+1 times, not chunks: it counts
	// down with `dec ax / jge short loc_30FDC`, which still takes the branch
	// at ax == 0. The extra slot is the four bytes just past the name table,
	// i.e. the entire first offset dword. Not reproduced here - it
	// reads as 00 00 00 00 for the usual first-chunk-at-zero layout, and the
	// query is always space-padded above, so only an all-blank name could
	// match it and no caller passes one.
	for (j = 0; j < chunk_count; j++) {
		for (i = 0; i < 4; i++) {
			if (resnames[i] != name[i]) {
				break;
			}
		}
		if (i == 4 || (resnames[i] == 0 && name[i] == 0x20)) {
			result = data;
			result += chunk_count * 8 + 6; // header, names and offsets
			offset = (legacy_u8 far*)resnames + chunk_count * 4;
			result += LEGACY_READ_U32_LE(offset);
			return (legacy_s8 far*)result;
		}
		resnames += 4; // move pointer to next 4-byte resource identifier
	}

	if (fatal > 1)
		fatal_error(aLocatesound4_4sSoundNotF, name);
	if (fatal == 1)
		fatal_error(aLocateshape4_4sShapeNotF, name);
	return MK_FP(0, 0);
}

legacy_s8 far* locate_shape_nofatal(legacy_s8 far* data, legacy_s8* name) {
	return locate_resource(data, name, 0);
}

legacy_s8 far* locate_shape_fatal(legacy_s8 far* data, legacy_s8* name) {
	return locate_resource(data, name, 1);
}

legacy_s8 far* locate_shape_alt(legacy_s8 far* data, legacy_s8* name) {
	return locate_shape_fatal(data, name);
}

legacy_s8 far* locate_sound_fatal(legacy_s8 far* data, legacy_s8* name) {
	return locate_resource(data, name, 2);
}

void locate_many_resources(legacy_s8 far* data, legacy_s8* names, legacy_s8 far** result) {
	while (*names != 0) {
		*result = locate_shape_fatal(data, names);
		names += 4;
		result ++;
	}
}

static void locate_many_resources_nofatal(legacy_s8 far* data, legacy_s8* names,
	legacy_s8 far** result) {
	while (*names != 0) {
		*result = locate_shape_nofatal(data, names);
		names += 4;
		result++;
	}
}

void nopsub_367E4(legacy_s8 far* data, legacy_s8* names, legacy_s8 far** result) {
	locate_many_resources_nofatal(data, names, result);
}

void nopsub_36826(legacy_s8 far* data, legacy_s8* names, legacy_s8 far** result) {
	while (*names != 0) {
		*result = locate_sound_fatal(data, names);
		names += 4;
		result++;
	}
}

void nopsub_36868(legacy_s8 far* data, legacy_s8* names, legacy_s8 far** result) {
	locate_many_resources_nofatal(data, names, result);
}

legacy_s8 far* locate_text_res(legacy_s8 far* data, legacy_s8* name) {
	legacy_s8 textname[4];
	textname[0] = textresprefix;
	textname[1] = name[0];
	textname[2] = name[1];
	textname[3] = name[2];
	return locate_shape_fatal(data, textname);
}

