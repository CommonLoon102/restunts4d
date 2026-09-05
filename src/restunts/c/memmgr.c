#include "externs.h"
#include "memmgr.h"
#include "platform.h"

#define MMGR_RESOURCE_SENTINEL_INDEX 49
#define MMGR_RESOURCE_STATE_FREE 0
#define MMGR_RESOURCE_STATE_CACHED 1
#define MMGR_RESOURCE_STATE_LIVE 2
#define HIGHPOOL_RESOURCE_STATE_RESERVED 3
#define HIGHPOOL_REGULAR_BLOCK 0
#define HIGHPOOL_VIDEO_ONLY_BLOCK 1
#define HIGHPOOL_LARGE_REQUEST_MIN_PARAS 3840U
#define HIGHPOOL_WINDOW_PARAS 4002U
#define HIGHPOOL_WINDOW_NAME "MCGA WINDOW"
#define HIGHPOOL_ZERO_WORDS_PER_PARAGRAPH 8
#define MMGR_INITIAL_ARENA_PARAS 100U
#define MMGR_ARENA_END_SEGMENT 40960U
#define DOS_PARAGRAPH_BYTES 16L
#define DOS_PARAGRAPHS_PER_COPY_CHUNK 4096
#define DOS_WORDS_PER_COPY_CHUNK 32768U
#define DOS_WORDS_PER_PARAGRAPH_SHIFT 3U
#define DOS_BYTES_PER_PARAGRAPH_SHIFT 4U
#define DOS_BYTES_PER_WORD_SHIFT 1U
#define DOS_BYTES_PER_WORD 2U
#define RESOURCE_IDENTIFIER_LENGTH 4

legacy_u16 word_3FF82 = 0; // last para reserved by memmgr
legacy_u16 word_3FF84 = 0; // first para reserved by memmgr
legacy_u16 resmaxsize = 0; // size of largest chunk?
legacy_u16 pspofs = 0;
legacy_u16 pspseg = 0;

struct MEMCHUNK resources[] = {
	{ { ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ' },
		0, 0, MMGR_RESOURCE_STATE_LIVE },
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
	{ { ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ' },
		0, 0, MMGR_RESOURCE_STATE_CACHED },
};
struct MEMCHUNK* resendptr1 = &resources[MMGR_RESOURCE_SENTINEL_INDEX];
struct MEMCHUNK* resendptr2 = &resources[MMGR_RESOURCE_SENTINEL_INDEX];
struct MEMCHUNK* resptr1 = resources;
struct MEMCHUNK* resptr2 = resources;

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
	legacy_s8 resname[MMGR_RESOURCE_NAME_LENGTH];
	legacy_u16 resseg;
	legacy_u16 resparas;
	legacy_u16 resstate;
};

// The full-screen window is the largest single allocation the game makes and
// the last one it asks for, by which time the pool is full of smaller
// resources and the arena has been whittled down - so it is the one that
// fails. Set its space aside before anything else can take it. 4002 paras
// is a 320 by 200 frame plus the bitmap header.
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
		if (seg < highblocks[k].blockseg + highblocks[k].blockparas && highblocks[k].blockseg < seg + paras)
			return;
	}

	if (highblockcount >= HIGHPOOL_MAXBLOCKS)
		return;

	// Fresh conventional memory is zero-filled at boot; give the pool the
	// same starting content so reads of never-written chunk bytes see the
	// same values as they would in the regular arena. Video memory is left
	// alone: it holds the visible screen, and its only tenant overwrites it
	// completely anyway.
	if (largeonly == HIGHPOOL_REGULAR_BLOCK) {
		for (p = 0; p < paras; p++) {
			wipe = dos_memory_make_pointer(seg + p, 0);
			for (i = 0; i < HIGHPOOL_ZERO_WORDS_PER_PARAGRAPH; i++)
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
		if (seg >= highblocks[i].blockseg && seg < highblocks[i].blockseg + highblocks[i].blockparas)
			return 1;
	}
	return 0;
}

static struct HIGHCHUNK* highpool_chunk_by_seg(legacy_u16 seg) {
	legacy_s16 i;
	for (i = 0; i < HIGHPOOL_MAXCHUNKS; i++) {
		if (highchunks[i].resstate != MMGR_RESOURCE_STATE_FREE && highchunks[i].resseg == seg)
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
	if (paras < HIGHPOOL_LARGE_REQUEST_MIN_PARAS && name[0] == 'M' && name[1] == 'C')
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
static legacy_u16 highpool_find_gap(struct HIGHBLOCK* block, legacy_u16 paras, legacy_s16 dropcached) {
	legacy_u16 cand, blockend;
	legacy_s16 i, conflict;

	cand = block->blockseg;
	blockend = block->blockseg + block->blockparas;

	for (;;) {
		if (paras > blockend - cand || cand >= blockend)
			return 0;
		conflict = 0;
		for (i = 0; i < HIGHPOOL_MAXCHUNKS; i++) {
			if (highchunks[i].resstate == MMGR_RESOURCE_STATE_FREE)
				continue;
			if (dropcached && highchunks[i].resstate == MMGR_RESOURCE_STATE_CACHED)
				continue; // discardable, so it does not block the search
			if (highchunks[i].resseg < cand + paras && cand < highchunks[i].resseg + highchunks[i].resparas) {
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
		if (highchunks[i].resstate != MMGR_RESOURCE_STATE_CACHED)
			continue;
		if (highchunks[i].resseg < seg + paras && seg < highchunks[i].resseg + highchunks[i].resparas)
			highchunks[i].resstate = MMGR_RESOURCE_STATE_FREE;
	}
}

// Does a chunk of this size fit at exactly this segment?
static legacy_s16 highpool_fits_at(legacy_u16 seg, legacy_u16 paras) {
	legacy_s16 i, b, inblock;

	inblock = 0;
	for (b = 0; b < highblockcount; b++) {
		if (seg >= highblocks[b].blockseg && seg + paras <= highblocks[b].blockseg + highblocks[b].blockparas) {
			inblock = 1;
			break;
		}
	}
	if (!inblock)
		return 0;

	for (i = 0; i < HIGHPOOL_MAXCHUNKS; i++) {
		if (highchunks[i].resstate == MMGR_RESOURCE_STATE_FREE)
			continue;
		if (highchunks[i].resseg == seg)
			continue;
		if (highchunks[i].resseg < seg + paras && seg < highchunks[i].resseg + highchunks[i].resparas)
			return 0;
	}
	return 1;
}

legacy_s16 highpool_can_fit(legacy_u16 paras) {
	legacy_s16 i;

	if (paras <= HIGHPOOL_WINDOW_PARAS && highpool_reserved_window() != 0)
		return 1;

	for (i = 0; i < highblockcount; i++) {
		if (highblocks[i].blocklarge == HIGHPOOL_VIDEO_ONLY_BLOCK &&
			paras < HIGHPOOL_LARGE_REQUEST_MIN_PARAS)
			continue;
		if (highpool_find_gap(&highblocks[i], paras, 1) != 0)
			return 1;
	}
	return 0;
}

static struct HIGHCHUNK* highpool_reserved_window(void) {
	legacy_s16 i;
	for (i = 0; i < HIGHPOOL_MAXCHUNKS; i++) {
		if (highchunks[i].resstate == HIGHPOOL_RESOURCE_STATE_RESERVED)
			return &highchunks[i];
	}
	return 0;
}

void highpool_reserve_window(void) {
	struct HIGHCHUNK* slot = 0;
	legacy_u16 seg;
	legacy_s16 i, b;

	for (i = 0; i < HIGHPOOL_MAXCHUNKS; i++) {
		if (highchunks[i].resstate == MMGR_RESOURCE_STATE_FREE) {
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
			for (i = 0; i < MMGR_RESOURCE_NAME_LENGTH; i++) {
				slot->resname[i] = nm[i];
				if (nm[i] == 0)
					break;
			}
			for (; i < MMGR_RESOURCE_NAME_LENGTH; i++)
				slot->resname[i] = 0;
			slot->resseg = seg;
			slot->resparas = HIGHPOOL_WINDOW_PARAS;
			slot->resstate = HIGHPOOL_RESOURCE_STATE_RESERVED;
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
				if (nm[i] == 0 && (name[i] == 0 ||
					i == MMGR_RESOURCE_NAME_LENGTH)) {
					res->resstate = MMGR_RESOURCE_STATE_LIVE;
					return dos_memory_make_pointer(res->resseg, 0);
				}
				if (i == MMGR_RESOURCE_NAME_LENGTH || nm[i] != name[i])
					break;
			}
		}
	}

	for (i = 0; i < HIGHPOOL_MAXCHUNKS; i++) {
		if (highchunks[i].resstate == MMGR_RESOURCE_STATE_FREE) {
			slot = &highchunks[i];
			break;
		}
	}
	if (slot == 0)
		return 0;

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
			if (highblocks[b].blocklarge == HIGHPOOL_VIDEO_ONLY_BLOCK &&
				paras < HIGHPOOL_LARGE_REQUEST_MIN_PARAS)
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
			for (i = 0; i < MMGR_RESOURCE_NAME_LENGTH; i++) {
				slot->resname[i] = name[i];
				if (name[i] == 0)
					break;
			}
			for (; i < MMGR_RESOURCE_NAME_LENGTH; i++)
				slot->resname[i] = 0;
			slot->resseg = seg;
			slot->resparas = paras;
			slot->resstate = MMGR_RESOURCE_STATE_LIVE;
			return dos_memory_make_pointer(seg, 0);
		}
	}
	return 0;
}

// Cache lookup mirroring mmgr_get_chunk_by_name: a cached pool chunk is
// revived in place instead of being copied back into the arena.
void far* highpool_get_by_name(const legacy_s8* name) {
	legacy_s16 i, j, found;
	struct HIGHCHUNK* chunk;

	for (i = 0; i < HIGHPOOL_MAXCHUNKS; i++) {
		chunk = &highchunks[i];
		if (chunk->resstate != MMGR_RESOURCE_STATE_CACHED)
			continue;
		found = 0;
		for (j = 0; j < MMGR_RESOURCE_NAME_LENGTH; j++) {
			if (name[j] == 0) {
				if (chunk->resname[j] == '.' || chunk->resname[j] == 0)
					found = 1;
				break;
			}
			if (name[j] != chunk->resname[j])
				break;
		}
		if (j == MMGR_RESOURCE_NAME_LENGTH)
			found = 1;
		if (found) {
			chunk->resstate = MMGR_RESOURCE_STATE_LIVE;
			return dos_memory_make_pointer(chunk->resseg, 0);
		}
	}
	return 0;
}
const legacy_s8* mmgr_path_to_name(const legacy_s8* filename) {
	const legacy_s8* c;
	const legacy_s8* result;

	result = filename;
	for (c = filename; *c; c++) {
		if (*c == ':' || *c == '\\')
			result = c + 1;
	}

	return result;
}

void far* mmgr_alloc_pages(const legacy_s8* arg_0, legacy_u16 arg_2) {
	legacy_s16 i;
	struct MEMCHUNK* resdi;
	struct MEMCHUNK* ressi;
	const legacy_s8* chunkname;
	legacy_u16 rax, rdx;

	if (highpool_route(mmgr_path_to_name(arg_0), arg_2)) {
		void far* highptr = highpool_alloc(mmgr_path_to_name(arg_0), arg_2);
		if (dos_memory_pointer_segment(highptr) != 0)
			return highptr;
		// The pool is full; fall through to the regular arena.
	}

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
	for (i = 0; i < MMGR_RESOURCE_NAME_LENGTH; i++)
		resdi->resname[i] = chunkname[i];

	rax = arg_2;
	resdi->resofs = rdx;
	resdi->ressize = rax;
	resdi->resunk = MMGR_RESOURCE_STATE_LIVE;

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

			ressi->resunk = MMGR_RESOURCE_STATE_FREE;
			ressi++;
			resendptr1 = ressi;
		}
	}

	return dos_memory_make_pointer(rdx, 0);
}

void far* mmgr_alloc_resbytes(const legacy_s8* name, legacy_s32 size) {
	/* The original allocator always reserves one paragraph after division. */
	return mmgr_alloc_pages(name, (legacy_u16)LEGACY_S32_WRAP_ADD(
		LEGACY_S32_DIV_OR_ZERO(size, DOS_PARAGRAPH_BYTES), 1L));
}

void mmgr_alloc_resmem(legacy_u16 arg_0) {

	void far* psp;
	legacy_u16 maxblocks;
	struct MEMCHUNK* rp;

	psp = dos_memory_get_psp();
	pspseg = dos_memory_pointer_segment(psp);
	pspofs = dos_memory_pointer_offset(psp);

	if (word_3FF82 == 0) {
		resptr1->resofs = dos_memory_allocate(MMGR_INITIAL_ARENA_PARAS);
		word_3FF84 = resptr1->resofs;
		maxblocks = dos_memory_resize(resptr1->resofs,
			arg_0 - resptr1->resofs);
		maxblocks = dos_memory_resize(resptr1->resofs, maxblocks);
		resendptr2->resofs = word_3FF84 + maxblocks;
		word_3FF82 = resendptr2->resofs;
		//fatal_error("%u\n", word_3FF82 - word_3FF84);
	}
	resendptr1 = resendptr2;
	resptr2 = resptr1;

	rp = resptr1;
	for (;;) {
		rp++;
		if (rp == resendptr2) break;
		rp->resunk = MMGR_RESOURCE_STATE_FREE;
	}
}

void mmgr_alloc_a000(void) {
	mmgr_alloc_resmem(MMGR_ARENA_END_SEGMENT);
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

#define MMGR_FIND_ARENA_CHUNK(chunk, segment) \
	while (1) { \
		if (chunk == resptr1) \
			fatal_error("memory manager - BLOCK NOT FOUND at SEG= %x", segment); \
		if (chunk->resofs == segment) break; \
		chunk--; \
	}

void far* mmgr_free(legacy_s8 far* ptr) {
	legacy_s16 i;
	legacy_u16 ax, bx, cx, dx, di;
	legacy_u16 ptrseg;
	struct MEMCHUNK* ressi;
	struct MEMCHUNK* resbx;

	ressi = resptr2;
	ptrseg = dos_memory_pointer_segment(ptr);

	if (highpool_owns_seg(ptrseg)) {
		struct HIGHCHUNK* highchunk = highpool_chunk_by_seg(ptrseg);
		if (highchunk == 0)
			fatal_error("memory manager - BLOCK NOT FOUND at SEG= %x", ptrseg);
		highchunk->resstate =
			(highchunk->resparas == HIGHPOOL_WINDOW_PARAS) ?
			HIGHPOOL_RESOURCE_STATE_RESERVED : MMGR_RESOURCE_STATE_CACHED;
		return dos_memory_make_pointer(
			ptrseg, dos_memory_pointer_offset(ptr));
	}

	MMGR_FIND_ARENA_CHUNK(ressi, ptrseg);

	ptrseg = 0;
	ressi->resunk = MMGR_RESOURCE_STATE_FREE;
	ax = resendptr1->resofs - resptr2->resofs - resptr2->ressize;
	if (ressi == resptr2 ||
		(ressi != resendptr1 && ax >= ressi->ressize)) {
		ptrseg = resendptr1->resofs - ressi->ressize;
		resendptr1--;
		resendptr1->resofs = ptrseg;
		resendptr1->ressize = ressi->ressize;
		resendptr1->resunk = MMGR_RESOURCE_STATE_CACHED;

		for (i = 0; i < MMGR_RESOURCE_NAME_LENGTH; i++) {
			resendptr1->resname[i] = ressi->resname[i];
		}

		copy_paras_reverse(ressi->resofs, ptrseg, ressi->ressize);
	}

	if (ressi == resptr2) {
		do {
			ressi--;
		} while (ressi->resunk == MMGR_RESOURCE_STATE_FREE);
		resptr2 = ressi;
	}

	return dos_memory_make_pointer(ptrseg, dos_memory_pointer_offset(ptr));
}

// `paras` is signed here but the original treats it as unsigned: it steps the
// count down by 4096 paragraphs and tests the borrow with jnb. Counts from
// 32768 through 36863 wrap into the non-negative range after that subtraction
// and still chunk correctly here; the divergence starts at 36864 (576 KiB),
// where the signed remainder test is taken despite no unsigned borrow. Nothing
// asks either copier for that much at once.
void mmgr_copy_paras(legacy_u16 srcseg, legacy_u16 destseg, legacy_s16 paras) {
	legacy_u16 count; // number of words to copy
	legacy_u16 far * srcptr;
	legacy_u16 far * destptr;

	while (paras != 0) {
		count = DOS_WORDS_PER_COPY_CHUNK;
		paras -= DOS_PARAGRAPHS_PER_COPY_CHUNK;
		if (paras < 0) {
			count = (paras + DOS_PARAGRAPHS_PER_COPY_CHUNK) <<
				DOS_WORDS_PER_PARAGRAPH_SHIFT;
			paras = 0;
		}
		srcptr = dos_memory_make_pointer(srcseg, 0);
		destptr = dos_memory_make_pointer(destseg, 0);

		while (count) {
			*destptr = *srcptr;
			srcptr++;
			destptr++;
			count--;
		}

		srcseg += DOS_PARAGRAPHS_PER_COPY_CHUNK;
		destseg += DOS_PARAGRAPHS_PER_COPY_CHUNK;
	}
}


// Same signedness caveat as mmgr_copy_paras above: the original's loop guard
// subtracts 4096 paragraphs and uses an unsigned no-borrow branch.
void copy_paras_reverse(legacy_u16 srcseg, legacy_u16 destseg, legacy_s16 paras) {
	legacy_u16 count, ofs;
	legacy_u16 far* destptr;
	legacy_u16 far* srcptr;

	srcseg += paras;
	destseg += paras;

	while (paras != 0) {
		count = DOS_PARAGRAPHS_PER_COPY_CHUNK;
		paras -= DOS_PARAGRAPHS_PER_COPY_CHUNK;
		if (paras < 0) {
			count = paras + DOS_PARAGRAPHS_PER_COPY_CHUNK;
			paras = 0;
		}
		srcseg -= count;
		destseg -= count;
		count <<= DOS_WORDS_PER_PARAGRAPH_SHIFT;
		ofs = (count << DOS_BYTES_PER_WORD_SHIFT) - DOS_BYTES_PER_WORD;

		srcptr = dos_memory_make_pointer(srcseg, ofs);
		destptr = dos_memory_make_pointer(destseg, ofs);
		while (count) {
			*destptr = *srcptr;
			srcptr--;
			destptr--;
			count--;
		}
	}
}

void mmgr_find_free(void) {
	legacy_s16 i;
	legacy_u16 regax, regdx, resunk;
	struct MEMCHUNK* ressi;
	struct MEMCHUNK* resdi;

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
				ressi->resunk = MMGR_RESOURCE_STATE_FREE;
				resdi->resunk = resunk;
				for (i = 0; i < MMGR_RESOURCE_NAME_LENGTH; i++) {
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

}

void far* mmgr_get_chunk_by_name(const legacy_s8* name) {
	const legacy_s8* pcdi;
	legacy_s16 regbx, regax;
	legacy_u16 srcofs, srcsize, destofs;
	struct MEMCHUNK* ressi;
	struct MEMCHUNK* resdi;
	legacy_s16 found = 0;

	pcdi = mmgr_path_to_name(name);

	{
		void far* highptr = highpool_get_by_name(pcdi);
		if (dos_memory_pointer_segment(highptr) != 0)
			return highptr;
	}

	ressi = resendptr1;

	for (; ressi < resendptr2; ressi++) {
		regbx = 0;
		if (ressi->resunk == MMGR_RESOURCE_STATE_FREE) {
			return 0;
		}

		for (; regbx < MMGR_RESOURCE_NAME_LENGTH; regbx++) {
			if (pcdi[regbx] == 0) {
				if (ressi->resname[regbx] == '.' || ressi->resname[regbx] == 0) {
					found = 1;
				}
				break;
			}
			if (pcdi[regbx] != ressi->resname[regbx])
				break;
		}
		if (regbx == MMGR_RESOURCE_NAME_LENGTH || found == 1) {
			/* Restore the cached block exactly as the original allocator does. */
			srcofs = ressi->resofs;
			srcsize = ressi->ressize;
			destofs = resptr2->resofs + resptr2->ressize;
			ressi->resunk = MMGR_RESOURCE_STATE_FREE;
			resdi = resptr2 + 1;
			resptr2 = resdi;
			resdi->resofs = destofs;
			resdi->ressize = srcsize;
			resdi->resunk = MMGR_RESOURCE_STATE_LIVE;
			memcpy(resdi->resname, ressi->resname,
				sizeof(legacy_s8[MMGR_RESOURCE_NAME_LENGTH]));
			if (resdi == resendptr1) {
				resendptr1++;
			}
			mmgr_copy_paras(srcofs, destofs, srcsize);
			regax = destofs + srcsize;
			while (regax > resendptr1->resofs) {
				resendptr1->resunk = MMGR_RESOURCE_STATE_FREE;
				resendptr1++;
			}
			mmgr_find_free();
			return dos_memory_make_pointer(resdi->resofs, 0);
		}

	}

	return 0;
}

legacy_u16 nopsub_31429(const legacy_s8* name) {
	const legacy_s8* wanted;
	legacy_s16 i;
	struct MEMCHUNK* chunk;

	wanted = mmgr_path_to_name(name);
	chunk = resendptr1;
	while (chunk < resendptr2) {
		if (chunk->resunk == MMGR_RESOURCE_STATE_FREE)
			return 0;
		for (i = 0; i < MMGR_RESOURCE_NAME_LENGTH; i++) {
			if (wanted[i] == 0) {
				if (chunk->resname[i] == '.' || chunk->resname[i] == 0)
					return 1;
				break;
			}
			if (wanted[i] != chunk->resname[i])
				break;
		}
		if (i == MMGR_RESOURCE_NAME_LENGTH)
			return 1;
		chunk++;
	}
	return 0;
}

void mmgr_release(void far* ptr) {
	legacy_s16 i;
	legacy_u16 regax, regbx, regcx, regdx;
	legacy_s8* strdi;
	struct MEMCHUNK* ressi;
	struct MEMCHUNK* resdi;

	regax = dos_memory_pointer_segment(ptr);
	ressi = resptr2;

	if (highpool_owns_seg(regax)) {
		struct HIGHCHUNK* highchunk = highpool_chunk_by_seg(regax);
		if (highchunk == 0)
			fatal_error("memory manager - BLOCK NOT FOUND at SEG= %x", regax);
		highchunk->resstate =
			(highchunk->resparas == HIGHPOOL_WINDOW_PARAS) ?
			HIGHPOOL_RESOURCE_STATE_RESERVED : MMGR_RESOURCE_STATE_FREE;
		return;
	}

	MMGR_FIND_ARENA_CHUNK(ressi, regax);

	ressi->resunk = MMGR_RESOURCE_STATE_FREE;
	if (ressi == resptr2) {
		do {
			ressi--;
		} while (ressi->resunk == MMGR_RESOURCE_STATE_FREE);
		resptr2 = ressi;
	}

}

// Rename a live arena chunk, so a buffer filled under one name can be handed
// on under the name the caller expects.
void mmgr_rename_chunk(legacy_s8 far* ptr, const legacy_s8* name) {
	legacy_s16 i;
	legacy_u16 regax;
	const legacy_s8* chunkname;
	struct MEMCHUNK* ressi;

	regax = dos_memory_pointer_segment(ptr);
	ressi = resptr2;

	if (highpool_owns_seg(regax))
		return;

	MMGR_FIND_ARENA_CHUNK(ressi, regax);

	chunkname = mmgr_path_to_name(name);
	for (i = 0; i < MMGR_RESOURCE_NAME_LENGTH; i++)
		ressi->resname[i] = chunkname[i];
}

legacy_u16 mmgr_get_chunk_size(legacy_s8 far* ptr) {
	legacy_s16 i;
	legacy_u16 regax, regbx, regcx, regdx;
	legacy_s8* strdi;
	struct MEMCHUNK* ressi;
	struct MEMCHUNK* resdi;

	regax = dos_memory_pointer_segment(ptr);
	ressi = resptr2;

	if (highpool_owns_seg(regax)) {
		struct HIGHCHUNK* highchunk = highpool_chunk_by_seg(regax);
		if (highchunk == 0)
			fatal_error("memory manager - BLOCK NOT FOUND at SEG= %x", regax);
		return highchunk->resparas;
	}

	MMGR_FIND_ARENA_CHUNK(ressi, regax);
	return ressi->ressize;
}

legacy_u16 mmgr_resize_memory(legacy_u16 arg_0, legacy_u16 arg_2, legacy_u16 arg_4) {
	legacy_s16 i;
	legacy_u16 regax, regbx, regcx, regdx;
	legacy_s8* strdi;
	struct MEMCHUNK* ressi;
	struct MEMCHUNK* resdi;

	(void)arg_0;
	regax = arg_2;
	ressi = resptr2;

	if (highpool_owns_seg(regax)) {
		struct HIGHCHUNK* highchunk = highpool_chunk_by_seg(regax);
		if (highchunk == 0)
			fatal_error("memory manager - BLOCK NOT FOUND at SEG= %x", arg_2);
		if (arg_4 <= highchunk->resparas) {
			highchunk->resparas = arg_4;
			return arg_4;
		}
		if (highpool_fits_at(regax, arg_4)) {
			highchunk->resparas = arg_4;
			return 0;
		}
		fatal_error("resizememory - NO MEMORY LEFT TO EXPAND HW=%x", resmaxsize);
	}

	MMGR_FIND_ARENA_CHUNK(ressi, arg_2);

	regax = arg_4;
	if (regax <= ressi->ressize) {
		ressi->ressize = regax;
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
		return 0;
	}

	ressi = resendptr1;
	resdi = resptr2;
	regax = resdi->resofs + resdi->ressize;

	for (;;) {
		if (regax <= ressi->resofs) break;
		if (ressi == resendptr2)
			fatal_error("resizememory - NO MEMORY LEFT TO EXPAND HW=%x", resmaxsize);

		ressi->resunk = MMGR_RESOURCE_STATE_FREE;
		ressi++;
		resendptr1 = ressi;
	}
	return 0;
}

void far* mmgr_op_unk(legacy_s8 far* ptr) {
	legacy_s16 i;
	legacy_u16 regax, regbx, regcx, regdx;
	legacy_s8* strdi;
	struct MEMCHUNK* ressi;
	struct MEMCHUNK* resdi;

	regax = dos_memory_pointer_segment(ptr);
	ressi = resptr2;

	if (highpool_owns_seg(regax)) {
		if (highpool_chunk_by_seg(regax) == 0)
			fatal_error("memory manager - BLOCK NOT FOUND at SEG= %x", regax);
		// Pool chunks are never compacted; they stay where they are.
		return dos_memory_make_pointer(regax, 0);
	}

	MMGR_FIND_ARENA_CHUNK(ressi, regax);

	resdi = ressi;
	resdi--;
	if (resdi->resunk == MMGR_RESOURCE_STATE_FREE) {

		do {
			resdi--;
		} while (resdi->resunk == MMGR_RESOURCE_STATE_FREE);

		ressi->resunk = MMGR_RESOURCE_STATE_FREE;
		regax = resdi->resofs + resdi->ressize;
		resdi++;
		if (ressi == resptr2) {
			resptr2 = resdi;
		}

		resdi->resofs = regax;
		resdi->ressize = ressi->ressize;
		resdi->resunk = MMGR_RESOURCE_STATE_LIVE;
		for (i = 0; i < MMGR_RESOURCE_NAME_LENGTH; i++) {
			resdi->resname[i] = ressi->resname[i];
		}
		mmgr_copy_paras(ressi->resofs, regax, ressi->ressize);

	} else {
		resdi = ressi;
	}

	return dos_memory_make_pointer(resdi->resofs, 0);
}

legacy_u32 mmgr_get_res_ofs_diff_scaled(void) {
	legacy_u32 result = mmgr_get_ofs_diff();
	return result << DOS_BYTES_PER_PARAGRAPH_SHIFT;
}

legacy_u32 mmgr_get_chunk_size_bytes(legacy_s8 far* ptr) {
	legacy_u32 result = mmgr_get_chunk_size(ptr);
	return result << DOS_BYTES_PER_PARAGRAPH_SHIFT;
}
//#endif



void locate_many_resources(legacy_s8 far* data, const legacy_s8* names,
	legacy_s8 far** result) {
	while (*names != 0) {
		*result = locate_shape_fatal(data, names);
		names += RESOURCE_IDENTIFIER_LENGTH;
		result ++;
	}
}

static void locate_many_resources_nofatal(legacy_s8 far* data,
	const legacy_s8* names, legacy_s8 far** result) {
	while (*names != 0) {
		*result = locate_shape_nofatal(data, names);
		names += RESOURCE_IDENTIFIER_LENGTH;
		result++;
	}
}

void nopsub_367E4(legacy_s8 far* data, const legacy_s8* names,
	legacy_s8 far** result) {
	locate_many_resources_nofatal(data, names, result);
}

void nopsub_36826(legacy_s8 far* data, const legacy_s8* names,
	legacy_s8 far** result) {
	while (*names != 0) {
		*result = locate_sound_fatal(data, names);
		names += RESOURCE_IDENTIFIER_LENGTH;
		result++;
	}
}

void nopsub_36868(legacy_s8 far* data, const legacy_s8* names,
	legacy_s8 far** result) {
	locate_many_resources_nofatal(data, names, result);
}

legacy_s8 far* locate_text_res(legacy_s8 far* data, const legacy_s8* name) {
	legacy_s8 textname[4];
	textname[0] = textresprefix;
	textname[1] = name[0];
	textname[2] = name[1];
	textname[3] = name[2];
	return locate_shape_fatal(data, textname);
}

