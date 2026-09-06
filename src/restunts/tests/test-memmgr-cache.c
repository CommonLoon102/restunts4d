#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../c/fatal.h"
#include "../c/memmgr.h"
#include "../c/platform.h"

#define TEST_PARAGRAPH_BYTES 16U
#define TEST_ARENA_START 4096U
#define TEST_ARENA_END 28672U
#define TEST_MEMORY_BYTES (TEST_ARENA_END * TEST_PARAGRAPH_BYTES)
#define TEST_RESOURCE_SLOTS 48U
#define TEST_CHURN_COUNT 2000U
#define TEST_CHURN_RESOURCE_COUNT 53U

extern struct MEMCHUNK* mmgr_live_sentinel;
extern struct MEMCHUNK* mmgr_last_live_chunk;
extern struct MEMCHUNK* mmgr_first_cached_chunk;
extern struct MEMCHUNK* mmgr_cache_sentinel;

static union {
    legacy_u16 alignment;
    legacy_u8 bytes[TEST_MEMORY_BYTES];
} test_memory;

void fatal_error(const legacy_s8* format, ...)
{
    fprintf(stderr, "Unexpected memory manager error: %s\n", format);
    abort();
}

void* _memcpy(void* destination, const void* source, legacy_u16 length)
{
    return memmove(destination, source, length);
}

void far* dos_memory_make_pointer(legacy_u16 segment, legacy_u16 offset)
{
    size_t address = (size_t)segment * TEST_PARAGRAPH_BYTES + offset;
    assert(address < sizeof(test_memory.bytes));
    return &test_memory.bytes[address];
}

legacy_u16 dos_memory_pointer_segment(const void far* pointer)
{
    ptrdiff_t address = (const legacy_u8*)pointer - test_memory.bytes;
    assert(address >= 0 && (size_t)address < sizeof(test_memory.bytes));
    return (legacy_u16)((size_t)address / TEST_PARAGRAPH_BYTES);
}

legacy_u16 dos_memory_pointer_offset(const void far* pointer)
{
    ptrdiff_t address = (const legacy_u8*)pointer - test_memory.bytes;
    assert(address >= 0 && (size_t)address < sizeof(test_memory.bytes));
    return (legacy_u16)((size_t)address % TEST_PARAGRAPH_BYTES);
}

void far* dos_memory_get_psp(void)
{
    return dos_memory_make_pointer(TEST_ARENA_START - 16U, 0U);
}

legacy_u16 dos_memory_allocate(legacy_u16 paragraphs)
{
    assert(paragraphs <= TEST_ARENA_END - TEST_ARENA_START);
    return TEST_ARENA_START;
}

legacy_u16 dos_memory_resize(legacy_u16 segment, legacy_u16 paragraphs)
{
    assert(segment == TEST_ARENA_START);
    assert(paragraphs <= TEST_ARENA_END - TEST_ARENA_START);
    return paragraphs;
}

static void reset_arena(void)
{
    memset(test_memory.bytes, 0, sizeof(test_memory.bytes));
    mmgr_alloc_resmem(TEST_ARENA_END);
    assert(mmgr_last_live_chunk == mmgr_live_sentinel);
    assert(mmgr_first_cached_chunk == mmgr_cache_sentinel);
}

static void make_name(legacy_s8 name[MMGR_RESOURCE_NAME_LENGTH + 1],
    unsigned int resource_id)
{
    memset(name, 0, MMGR_RESOURCE_NAME_LENGTH + 1);
    snprintf((char*)name, MMGR_RESOURCE_NAME_LENGTH + 1,
        "RES%08u", resource_id);
}

static legacy_u8 payload_byte(unsigned int resource_id, size_t offset)
{
    return (legacy_u8)(resource_id * 37U + offset * 13U + 1U);
}

static void write_payload(void* pointer, legacy_u16 paragraphs,
    unsigned int resource_id)
{
    size_t i;
    legacy_u8* bytes = (legacy_u8*)pointer;
    for (i = 0; i < (size_t)paragraphs * TEST_PARAGRAPH_BYTES; i++)
        bytes[i] = payload_byte(resource_id, i);
}

static void check_payload(const void* pointer, legacy_u16 paragraphs,
    unsigned int resource_id)
{
    size_t i;
    const legacy_u8* bytes = (const legacy_u8*)pointer;
    assert(pointer != NULL);
    for (i = 0; i < (size_t)paragraphs * TEST_PARAGRAPH_BYTES; i++) {
        if (bytes[i] != payload_byte(resource_id, i)) {
            fprintf(stderr, "Cached resource %u corrupted at byte %lu: "
                "expected %u, got %u\n", resource_id, (unsigned long)i,
                (unsigned int)payload_byte(resource_id, i),
                (unsigned int)bytes[i]);
            abort();
        }
    }
}

static void test_free_last_resource_with_full_table(void)
{
    unsigned int i;
    legacy_s8 name[MMGR_RESOURCE_NAME_LENGTH + 1];
    void* last_resource = NULL;
    void* cached_resource;

    reset_arena();
    for (i = 0; i < TEST_RESOURCE_SLOTS; i++) {
        make_name(name, i);
        last_resource = mmgr_alloc_pages(name, 2U);
        write_payload(last_resource, 2U, i);
    }
    assert(mmgr_last_live_chunk + 1 == mmgr_first_cached_chunk);

    /* The last live block must survive when its descriptor becomes cached. */
    cached_resource = mmgr_free((legacy_s8*)last_resource);
    assert(cached_resource != last_resource);
    check_payload(cached_resource, 2U, TEST_RESOURCE_SLOTS - 1U);
    last_resource = mmgr_get_chunk_by_name(name);
    check_payload(last_resource, 2U, TEST_RESOURCE_SLOTS - 1U);
    assert(mmgr_last_live_chunk + 1 == mmgr_first_cached_chunk);
}

static void test_free_non_last_resource_with_full_table(void)
{
    unsigned int i;
    legacy_s8 first_name[MMGR_RESOURCE_NAME_LENGTH + 1] = "FIRST";
    legacy_s8 last_name[MMGR_RESOURCE_NAME_LENGTH + 1] = "LAST";
    legacy_s8 name[MMGR_RESOURCE_NAME_LENGTH + 1];
    void* first_resource;
    void* last_resource;
    void* cached_resource;
    struct MEMCHUNK* last_chunk;

    reset_arena();
    first_resource = mmgr_alloc_pages(first_name, 2U);
    write_payload(first_resource, 2U, 900U);
    last_resource = mmgr_alloc_pages(last_name, 3U);
    write_payload(last_resource, 3U, 901U);
    for (i = 0; i < TEST_RESOURCE_SLOTS - 2U; i++) {
        void* resource;
        make_name(name, i);
        resource = mmgr_alloc_pages(name, 1U);
        write_payload(resource, 1U, i);
        cached_resource = mmgr_free((legacy_s8*)resource);
        check_payload(cached_resource, 1U, i);
    }
    last_chunk = mmgr_last_live_chunk;
    assert(last_chunk + 1 == mmgr_first_cached_chunk);

    /* Freeing an older block must not consume the last live descriptor. */
    mmgr_free((legacy_s8*)first_resource);
    assert(mmgr_last_live_chunk == last_chunk);
    assert(last_chunk->resofs == dos_memory_pointer_segment(last_resource));
    assert(mmgr_get_chunk_size((legacy_s8*)last_resource) == 3U);
    check_payload(last_resource, 3U, 901U);

    cached_resource = mmgr_free((legacy_s8*)last_resource);
    check_payload(cached_resource, 3U, 901U);
    assert(mmgr_last_live_chunk == mmgr_live_sentinel);
    assert(mmgr_get_ofs_diff() == TEST_ARENA_END - TEST_ARENA_START);
    last_resource = mmgr_get_chunk_by_name(last_name);
    check_payload(last_resource, 3U, 901U);
    mmgr_release(last_resource);

    first_resource = mmgr_alloc_pages(first_name, 2U);
    assert(dos_memory_pointer_segment(first_resource) == TEST_ARENA_START);
    write_payload(first_resource, 2U, 902U);
    check_payload(first_resource, 2U, 902U);
    mmgr_release(first_resource);
    assert(mmgr_get_ofs_diff() == TEST_ARENA_END - TEST_ARENA_START);
}

static void test_repeated_cache_churn(void)
{
    unsigned int iteration;
    legacy_s8 name[MMGR_RESOURCE_NAME_LENGTH + 1];
    legacy_s8 pinned_name[MMGR_RESOURCE_NAME_LENGTH + 1] = "PINNED";
    void* pinned_resource;
    unsigned int cache_hits = 0;

    reset_arena();
    pinned_resource = mmgr_alloc_pages(pinned_name, 7U);
    write_payload(pinned_resource, 7U, 999U);
    for (iteration = 0; iteration < TEST_CHURN_COUNT; iteration++) {
        unsigned int resource_id = iteration % TEST_CHURN_RESOURCE_COUNT;
        legacy_u16 paragraphs = (legacy_u16)(1U + resource_id % 5U);
        void* resource;
        void* cached_resource;

        make_name(name, resource_id);
        resource = mmgr_get_chunk_by_name(name);
        if (resource != NULL) {
            cache_hits++;
            check_payload(resource, paragraphs, resource_id);
        } else {
            resource = mmgr_alloc_pages(name, paragraphs);
            write_payload(resource, paragraphs, resource_id);
        }
        cached_resource = mmgr_free((legacy_s8*)resource);
        check_payload(cached_resource, paragraphs, resource_id);
        check_payload(pinned_resource, 7U, 999U);
        assert(mmgr_get_ofs_diff() ==
            TEST_ARENA_END - TEST_ARENA_START - 7U);
        assert(mmgr_last_live_chunk == mmgr_live_sentinel + 1);
        assert(mmgr_first_cached_chunk > mmgr_last_live_chunk);
    }
    assert(cache_hits > TEST_CHURN_COUNT / 2U);
    mmgr_release(pinned_resource);
    assert(mmgr_get_ofs_diff() == TEST_ARENA_END - TEST_ARENA_START);
}

int main(void)
{
    test_free_last_resource_with_full_table();
    test_free_non_last_resource_with_full_table();
    test_repeated_cache_churn();
    return 0;
}
