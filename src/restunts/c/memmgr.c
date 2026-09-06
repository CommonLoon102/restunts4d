#include "externs.h"
#include "memmgr.h"
#include "platform.h"

#define MMGR_RESOURCE_SENTINEL_INDEX 49

enum MMGR_RESOURCE_STATE {
	MMGR_RESOURCE_STATE_FREE = 0,
	MMGR_RESOURCE_STATE_CACHED = 1,
	MMGR_RESOURCE_STATE_LIVE = 2,
	HIGHPOOL_RESOURCE_STATE_RESERVED = 3
};

enum HIGHPOOL_BLOCK_TYPE {
	HIGHPOOL_REGULAR_BLOCK = 0,
	HIGHPOOL_VIDEO_ONLY_BLOCK = 1
};

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

legacy_u16 mmgr_arena_end_segment = 0; // last para reserved by memmgr
legacy_u16 mmgr_arena_start_segment = 0; // first para reserved by memmgr
legacy_u16 mmgr_high_water_segment = 0; // highest segment reached by live allocations
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
struct MEMCHUNK* mmgr_first_cached_chunk = &resources[MMGR_RESOURCE_SENTINEL_INDEX];
struct MEMCHUNK* mmgr_cache_sentinel = &resources[MMGR_RESOURCE_SENTINEL_INDEX];
struct MEMCHUNK* mmgr_live_sentinel = resources;
struct MEMCHUNK* mmgr_last_live_chunk = resources;

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

void far* mmgr_alloc_pages(const legacy_s8* name, legacy_u16 paragraphs) {
	legacy_s16 name_index;
	struct MEMCHUNK* live_chunk;
	struct MEMCHUNK* cached_chunk;
	const legacy_s8* chunkname;
	legacy_u16 size_or_end_segment, start_segment;

	if (highpool_route(mmgr_path_to_name(name), paragraphs)) {
		void far* highptr = highpool_alloc(mmgr_path_to_name(name), paragraphs);
		if (dos_memory_pointer_segment(highptr) != 0)
			return highptr;
		// The pool is full; fall through to the regular arena.
	}

	live_chunk = mmgr_last_live_chunk;
	cached_chunk = mmgr_first_cached_chunk;
	start_segment = live_chunk->resofs + live_chunk->ressize;

	live_chunk++;
	if (cached_chunk <= live_chunk) {
		if (cached_chunk == mmgr_cache_sentinel)
			fatal_error("reservememory - OUT OF MEMORY SLOTS RESERVING %s", name);

		cached_chunk++;
		mmgr_first_cached_chunk = cached_chunk;
	}

	mmgr_last_live_chunk = live_chunk;
	chunkname = mmgr_path_to_name(name);
	for (name_index = 0; name_index < MMGR_RESOURCE_NAME_LENGTH; name_index++)
		live_chunk->resname[name_index] = chunkname[name_index];

	size_or_end_segment = paragraphs;
	live_chunk->resofs = start_segment;
	live_chunk->ressize = size_or_end_segment;
	live_chunk->resstate = MMGR_RESOURCE_STATE_LIVE;

	size_or_end_segment += start_segment;
	if (size_or_end_segment > mmgr_high_water_segment)
		mmgr_high_water_segment = size_or_end_segment;

	if (size_or_end_segment > cached_chunk->resofs) {
		cached_chunk = mmgr_first_cached_chunk;
		live_chunk = mmgr_last_live_chunk;
		size_or_end_segment = live_chunk->resofs + live_chunk->ressize;

		while (size_or_end_segment > cached_chunk->resofs) {
			if (cached_chunk == mmgr_cache_sentinel) {
				fatal_error("reservememory - OUT OF MEMORY RESERVING %s P=%x HW=%x\r\n", name, live_chunk->ressize, mmgr_high_water_segment);
			}

			cached_chunk->resstate = MMGR_RESOURCE_STATE_FREE;
			cached_chunk++;
			mmgr_first_cached_chunk = cached_chunk;
		}
	}

	return dos_memory_make_pointer(start_segment, 0);
}

void far* mmgr_alloc_resbytes(const legacy_s8* name, legacy_s32 size) {
	/* The original allocator always reserves one paragraph after division. */
	return mmgr_alloc_pages(name, (legacy_u16)LEGACY_S32_WRAP_ADD(
		LEGACY_S32_DIV_OR_ZERO(size, DOS_PARAGRAPH_BYTES), 1L));
}

void mmgr_alloc_resmem(legacy_u16 end_segment) {

	void far* psp;
	legacy_u16 maxblocks;
	struct MEMCHUNK* chunk;

	psp = dos_memory_get_psp();
	pspseg = dos_memory_pointer_segment(psp);
	pspofs = dos_memory_pointer_offset(psp);

	if (mmgr_arena_end_segment == 0) {
		mmgr_live_sentinel->resofs = dos_memory_allocate(MMGR_INITIAL_ARENA_PARAS);
		mmgr_arena_start_segment = mmgr_live_sentinel->resofs;
		maxblocks = dos_memory_resize(mmgr_live_sentinel->resofs,
			end_segment - mmgr_live_sentinel->resofs);
		maxblocks = dos_memory_resize(mmgr_live_sentinel->resofs, maxblocks);
		mmgr_cache_sentinel->resofs = mmgr_arena_start_segment + maxblocks;
		mmgr_arena_end_segment = mmgr_cache_sentinel->resofs;
		//fatal_error("%u\n", mmgr_arena_end_segment - mmgr_arena_start_segment);
	}
	mmgr_first_cached_chunk = mmgr_cache_sentinel;
	mmgr_last_live_chunk = mmgr_live_sentinel;

	chunk = mmgr_live_sentinel;
	for (;;) {
		chunk++;
		if (chunk == mmgr_cache_sentinel) break;
		chunk->resstate = MMGR_RESOURCE_STATE_FREE;
	}
}

void mmgr_alloc_a000(void) {
	mmgr_alloc_resmem(MMGR_ARENA_END_SEGMENT);
}

legacy_u16 mmgr_get_ofs_diff(void) {
	return mmgr_cache_sentinel->resofs - mmgr_last_live_chunk->resofs - mmgr_last_live_chunk->ressize;
}

legacy_u16 mmgr_available_paragraphs(void) {
	return mmgr_get_ofs_diff();
}

legacy_u16 mmgr_allocated_paragraphs(void) {
	return (legacy_u16)(mmgr_last_live_chunk->resofs + mmgr_last_live_chunk->ressize - mmgr_live_sentinel->resofs);
}

#define MMGR_FIND_ARENA_CHUNK(chunk, segment) \
	while (1) { \
		if (chunk == mmgr_live_sentinel) \
			fatal_error("memory manager - BLOCK NOT FOUND at SEG= %x", segment); \
		if (chunk->resofs == segment) break; \
		chunk--; \
	}

void far* mmgr_free(legacy_s8 far* ptr) {
	legacy_s16 name_index;
	legacy_u16 free_paragraphs, unused_bx, unused_cx, unused_dx, unused_di;
	legacy_u16 ptrseg;
	struct MEMCHUNK* chunk;
	struct MEMCHUNK* unused_chunk;

	chunk = mmgr_last_live_chunk;
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

	MMGR_FIND_ARENA_CHUNK(chunk, ptrseg);

	ptrseg = 0;
	chunk->resstate = MMGR_RESOURCE_STATE_FREE;
	free_paragraphs = mmgr_first_cached_chunk->resofs - mmgr_last_live_chunk->resofs - mmgr_last_live_chunk->ressize;
	if (chunk == mmgr_last_live_chunk ||
		(chunk != mmgr_first_cached_chunk && free_paragraphs >= chunk->ressize)) {
		ptrseg = mmgr_first_cached_chunk->resofs - chunk->ressize;
		mmgr_first_cached_chunk--;
		mmgr_first_cached_chunk->resofs = ptrseg;
		mmgr_first_cached_chunk->ressize = chunk->ressize;
		mmgr_first_cached_chunk->resstate = MMGR_RESOURCE_STATE_CACHED;

		for (name_index = 0; name_index < MMGR_RESOURCE_NAME_LENGTH; name_index++) {
			mmgr_first_cached_chunk->resname[name_index] = chunk->resname[name_index];
		}

		copy_paras_reverse(chunk->resofs, ptrseg, chunk->ressize);
	}

	if (chunk == mmgr_last_live_chunk) {
		do {
			chunk--;
		} while (chunk->resstate == MMGR_RESOURCE_STATE_FREE);
		mmgr_last_live_chunk = chunk;
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
	legacy_s16 name_index;
	legacy_u16 destination_segment, free_paragraphs, resstate;
	struct MEMCHUNK* source_chunk;
	struct MEMCHUNK* destination_chunk;

	source_chunk = mmgr_cache_sentinel;
	destination_chunk = source_chunk;
	free_paragraphs = 0;

	do {
		if ((source_chunk->resstate & 1) == 0) {
			free_paragraphs += source_chunk->ressize;
		} else {

			if (free_paragraphs != 0) {
				destination_chunk++;
				destination_segment = destination_chunk->resofs - source_chunk->ressize;
				destination_chunk--;
				destination_chunk->ressize = source_chunk->ressize;
				destination_chunk->resofs = destination_segment;
				resstate = source_chunk->resstate;
				source_chunk->resstate = MMGR_RESOURCE_STATE_FREE;
				destination_chunk->resstate = resstate;
				for (name_index = 0; name_index < MMGR_RESOURCE_NAME_LENGTH; name_index++) {
					destination_chunk->resname[name_index] = source_chunk->resname[name_index];
				}
				copy_paras_reverse(source_chunk->resofs, destination_segment, source_chunk->ressize);
			}

			destination_chunk--;
		}
		source_chunk--;
	// `cmp si, mmgr_first_cached_chunk / jnb` - the entry at mmgr_first_cached_chunk is the last one
	// the original visits, not one past the end.
	} while (source_chunk >= mmgr_first_cached_chunk);

	destination_chunk++;
	mmgr_first_cached_chunk = destination_chunk;

}

void far* mmgr_get_chunk_by_name(const legacy_s8* name) {
	const legacy_s8* wanted_name;
	legacy_s16 name_index, allocation_end_segment;
	legacy_u16 srcofs, srcsize, destofs;
	struct MEMCHUNK* cached_chunk;
	struct MEMCHUNK* live_chunk;
	legacy_s16 found = 0;

	wanted_name = mmgr_path_to_name(name);

	{
		void far* highptr = highpool_get_by_name(wanted_name);
		if (dos_memory_pointer_segment(highptr) != 0)
			return highptr;
	}

	cached_chunk = mmgr_first_cached_chunk;

	for (; cached_chunk < mmgr_cache_sentinel; cached_chunk++) {
		name_index = 0;
		if (cached_chunk->resstate == MMGR_RESOURCE_STATE_FREE) {
			return 0;
		}

		for (; name_index < MMGR_RESOURCE_NAME_LENGTH; name_index++) {
			if (wanted_name[name_index] == 0) {
				if (cached_chunk->resname[name_index] == '.' || cached_chunk->resname[name_index] == 0) {
					found = 1;
				}
				break;
			}
			if (wanted_name[name_index] != cached_chunk->resname[name_index])
				break;
		}
		if (name_index == MMGR_RESOURCE_NAME_LENGTH || found == 1) {
			/* Restore the cached block exactly as the original allocator does. */
			srcofs = cached_chunk->resofs;
			srcsize = cached_chunk->ressize;
			destofs = mmgr_last_live_chunk->resofs + mmgr_last_live_chunk->ressize;
			cached_chunk->resstate = MMGR_RESOURCE_STATE_FREE;
			live_chunk = mmgr_last_live_chunk + 1;
			mmgr_last_live_chunk = live_chunk;
			live_chunk->resofs = destofs;
			live_chunk->ressize = srcsize;
			live_chunk->resstate = MMGR_RESOURCE_STATE_LIVE;
			memcpy(live_chunk->resname, cached_chunk->resname,
				sizeof(legacy_s8[MMGR_RESOURCE_NAME_LENGTH]));
			if (live_chunk == mmgr_first_cached_chunk) {
				mmgr_first_cached_chunk++;
			}
			mmgr_copy_paras(srcofs, destofs, srcsize);
			allocation_end_segment = destofs + srcsize;
			while (allocation_end_segment > mmgr_first_cached_chunk->resofs) {
				mmgr_first_cached_chunk->resstate = MMGR_RESOURCE_STATE_FREE;
				mmgr_first_cached_chunk++;
			}
			mmgr_find_free();
			return dos_memory_make_pointer(live_chunk->resofs, 0);
		}

	}

	return 0;
}

legacy_u16 mmgr_has_cached_resource(const legacy_s8* name) {
	const legacy_s8* wanted;
	legacy_s16 i;
	struct MEMCHUNK* chunk;

	wanted = mmgr_path_to_name(name);
	chunk = mmgr_first_cached_chunk;
	while (chunk < mmgr_cache_sentinel) {
		if (chunk->resstate == MMGR_RESOURCE_STATE_FREE)
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
	legacy_s16 unused_index;
	legacy_u16 segment, unused_bx, unused_cx, unused_dx;
	legacy_s8* unused_name;
	struct MEMCHUNK* chunk;
	struct MEMCHUNK* unused_chunk;

	segment = dos_memory_pointer_segment(ptr);
	chunk = mmgr_last_live_chunk;

	if (highpool_owns_seg(segment)) {
		struct HIGHCHUNK* highchunk = highpool_chunk_by_seg(segment);
		if (highchunk == 0)
			fatal_error("memory manager - BLOCK NOT FOUND at SEG= %x", segment);
		highchunk->resstate =
			(highchunk->resparas == HIGHPOOL_WINDOW_PARAS) ?
			HIGHPOOL_RESOURCE_STATE_RESERVED : MMGR_RESOURCE_STATE_FREE;
		return;
	}

	MMGR_FIND_ARENA_CHUNK(chunk, segment);

	chunk->resstate = MMGR_RESOURCE_STATE_FREE;
	if (chunk == mmgr_last_live_chunk) {
		do {
			chunk--;
		} while (chunk->resstate == MMGR_RESOURCE_STATE_FREE);
		mmgr_last_live_chunk = chunk;
	}

}

// Rename a live arena chunk, so a buffer filled under one name can be handed
// on under the name the caller expects.
void mmgr_rename_chunk(legacy_s8 far* ptr, const legacy_s8* name) {
	legacy_s16 name_index;
	legacy_u16 segment;
	const legacy_s8* chunkname;
	struct MEMCHUNK* chunk;

	segment = dos_memory_pointer_segment(ptr);
	chunk = mmgr_last_live_chunk;

	if (highpool_owns_seg(segment))
		return;

	MMGR_FIND_ARENA_CHUNK(chunk, segment);

	chunkname = mmgr_path_to_name(name);
	for (name_index = 0; name_index < MMGR_RESOURCE_NAME_LENGTH; name_index++)
		chunk->resname[name_index] = chunkname[name_index];
}

legacy_u16 mmgr_get_chunk_size(legacy_s8 far* ptr) {
	legacy_s16 unused_index;
	legacy_u16 segment, unused_bx, unused_cx, unused_dx;
	legacy_s8* unused_name;
	struct MEMCHUNK* chunk;
	struct MEMCHUNK* unused_chunk;

	segment = dos_memory_pointer_segment(ptr);
	chunk = mmgr_last_live_chunk;

	if (highpool_owns_seg(segment)) {
		struct HIGHCHUNK* highchunk = highpool_chunk_by_seg(segment);
		if (highchunk == 0)
			fatal_error("memory manager - BLOCK NOT FOUND at SEG= %x", segment);
		return highchunk->resparas;
	}

	MMGR_FIND_ARENA_CHUNK(chunk, segment);
	return chunk->ressize;
}

legacy_u16 mmgr_resize_memory(legacy_u16 unused_offset, legacy_u16 segment, legacy_u16 paragraphs) {
	legacy_s16 unused_index;
	legacy_u16 size_or_end_segment, unused_bx, unused_cx, unused_dx;
	legacy_s8* unused_name;
	struct MEMCHUNK* chunk;
	struct MEMCHUNK* limit_chunk;

	(void)unused_offset;
	size_or_end_segment = segment;
	chunk = mmgr_last_live_chunk;

	if (highpool_owns_seg(size_or_end_segment)) {
		struct HIGHCHUNK* highchunk = highpool_chunk_by_seg(size_or_end_segment);
		if (highchunk == 0)
			fatal_error("memory manager - BLOCK NOT FOUND at SEG= %x", segment);
		if (paragraphs <= highchunk->resparas) {
			highchunk->resparas = paragraphs;
			return paragraphs;
		}
		if (highpool_fits_at(size_or_end_segment, paragraphs)) {
			highchunk->resparas = paragraphs;
			return 0;
		}
		fatal_error("resizememory - NO MEMORY LEFT TO EXPAND HW=%x", mmgr_high_water_segment);
	}

	MMGR_FIND_ARENA_CHUNK(chunk, segment);

	size_or_end_segment = paragraphs;
	if (size_or_end_segment <= chunk->ressize) {
		chunk->ressize = size_or_end_segment;
		return size_or_end_segment;
	}

	if (chunk != mmgr_last_live_chunk)
		fatal_error("resizememory - CANNOT EXPAND BLOCK NOT AT TOP");
	chunk->ressize = size_or_end_segment;
	limit_chunk = mmgr_first_cached_chunk;
	size_or_end_segment += chunk->resofs;
	if (size_or_end_segment >= mmgr_high_water_segment)
		mmgr_high_water_segment = size_or_end_segment;

	if (size_or_end_segment <= limit_chunk->resofs) {
		return 0;
	}

	chunk = mmgr_first_cached_chunk;
	limit_chunk = mmgr_last_live_chunk;
	size_or_end_segment = limit_chunk->resofs + limit_chunk->ressize;

	for (;;) {
		if (size_or_end_segment <= chunk->resofs) break;
		if (chunk == mmgr_cache_sentinel)
			fatal_error("resizememory - NO MEMORY LEFT TO EXPAND HW=%x", mmgr_high_water_segment);

		chunk->resstate = MMGR_RESOURCE_STATE_FREE;
		chunk++;
		mmgr_first_cached_chunk = chunk;
	}
	return 0;
}

void far* mmgr_compact_live_chunk(legacy_s8 far* ptr) {
	legacy_s16 name_index;
	legacy_u16 destination_segment, unused_bx, unused_cx, unused_dx;
	legacy_s8* unused_name;
	struct MEMCHUNK* source_chunk;
	struct MEMCHUNK* destination_chunk;

	destination_segment = dos_memory_pointer_segment(ptr);
	source_chunk = mmgr_last_live_chunk;

	if (highpool_owns_seg(destination_segment)) {
		if (highpool_chunk_by_seg(destination_segment) == 0)
			fatal_error("memory manager - BLOCK NOT FOUND at SEG= %x", destination_segment);
		// Pool chunks are never compacted; they stay where they are.
		return dos_memory_make_pointer(destination_segment, 0);
	}

	MMGR_FIND_ARENA_CHUNK(source_chunk, destination_segment);

	destination_chunk = source_chunk;
	destination_chunk--;
	if (destination_chunk->resstate == MMGR_RESOURCE_STATE_FREE) {

		do {
			destination_chunk--;
		} while (destination_chunk->resstate == MMGR_RESOURCE_STATE_FREE);

		source_chunk->resstate = MMGR_RESOURCE_STATE_FREE;
		destination_segment = destination_chunk->resofs + destination_chunk->ressize;
		destination_chunk++;
		if (source_chunk == mmgr_last_live_chunk) {
			mmgr_last_live_chunk = destination_chunk;
		}

		destination_chunk->resofs = destination_segment;
		destination_chunk->ressize = source_chunk->ressize;
		destination_chunk->resstate = MMGR_RESOURCE_STATE_LIVE;
		for (name_index = 0; name_index < MMGR_RESOURCE_NAME_LENGTH; name_index++) {
			destination_chunk->resname[name_index] = source_chunk->resname[name_index];
		}
		mmgr_copy_paras(source_chunk->resofs, destination_segment, source_chunk->ressize);

	} else {
		destination_chunk = source_chunk;
	}

	return dos_memory_make_pointer(destination_chunk->resofs, 0);
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

void locate_many_shapes_nofatal(legacy_s8 far* data, const legacy_s8* names,
	legacy_s8 far** result) {
	locate_many_resources_nofatal(data, names, result);
}

void locate_many_sounds_fatal(legacy_s8 far* data, const legacy_s8* names,
	legacy_s8 far** result) {
	while (*names != 0) {
		*result = locate_sound_fatal(data, names);
		names += RESOURCE_IDENTIFIER_LENGTH;
		result++;
	}
}

void locate_many_resources_optional(legacy_s8 far* data, const legacy_s8* names,
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

