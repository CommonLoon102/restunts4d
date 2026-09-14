#include <assert.h>
#include <string.h>

#include "../pixldump/murmur3.h"

/* Values from the independent upstream SMHasher implementation, seed 0. */
static void test_text_vectors(void)
{
	assert(pixldump_murmur3((const legacy_u8 *)"", 0U) == 0x00000000UL);
	assert(pixldump_murmur3((const legacy_u8 *)"a", 1U) == 0x3c2569b2UL);
	assert(pixldump_murmur3((const legacy_u8 *)"ab", 2U) == 0x9bbfd75fUL);
	assert(pixldump_murmur3((const legacy_u8 *)"abc", 3U) == 0xb3dd93faUL);
	assert(pixldump_murmur3((const legacy_u8 *)"abcd", 4U) == 0x43ed676aUL);
	assert(pixldump_murmur3((const legacy_u8 *)"abcde", 5U) == 0xe89b9af6UL);
	assert(pixldump_murmur3((const legacy_u8 *)"foo", 3U) == 0xf6a5c420UL);
	assert(pixldump_murmur3((const legacy_u8 *)"message digest", 14U) == 0x638f4169UL);
	assert(pixldump_murmur3((const legacy_u8 *)"The quick brown fox jumps over the lazy dog",
							43U) == 0x2e4ff723UL);
}

static void test_binary_vectors(void)
{
	static legacy_u8 data[65536UL];
	static const struct {
		legacy_u16 length;
		legacy_u32 expected;
	} vectors[] = {
		{0U, 0x00000000UL},		{1U, 0x4d79446cUL},		{2U, 0x6bfe18a9UL},
		{3U, 0x389b84e7UL},		{4U, 0x1939e63dUL},		{5U, 0xa4811facUL},
		{6U, 0x2f82c86dUL},		{7U, 0x0ea050ddUL},		{8U, 0x3ae449f2UL},
		{9U, 0x593f000dUL},		{10U, 0x868522c3UL},	{11U, 0xd3ece69eUL},
		{12U, 0x7d0fe536UL},	{13U, 0x6fa10f80UL},	{14U, 0xb7b2cf95UL},
		{15U, 0x6b580432UL},	{16U, 0xe6271c29UL},	{17U, 0xc792fd21UL},
		{31U, 0xa5df4a5bUL},	{32U, 0x76cd521cUL},	{63U, 0x139217adUL},
		{64U, 0x0928bc0cUL},	{65U, 0xea161f44UL},	{255U, 0xfcbed7fdUL},
		{256U, 0xe7fef2a0UL},	{1023U, 0x423a89b2UL},	{1024U, 0x9f559fd8UL},
		{63999U, 0xfb42fba4UL}, {64000U, 0x29dc155eUL}, {65534U, 0xee44913cUL},
		{65535U, 0x88590672UL},
	};
	legacy_u32 index;

	for (index = 0; index < sizeof(data); index++) {
		data[index] = (legacy_u8)(index * 37UL + 11UL);
	}
	for (index = 0; index < sizeof(vectors) / sizeof(vectors[0]); index++) {
		assert(pixldump_murmur3(data, vectors[index].length) == vectors[index].expected);
	}
	assert(pixldump_murmur3(data + 1, 1U) == 0xd271c07fUL);
	assert(pixldump_murmur3(data + 1, 2U) == 0x3be98cf5UL);
	assert(pixldump_murmur3(data + 1, 3U) == 0x8b03436bUL);
	assert(pixldump_murmur3(data + 1, 4U) == 0xaf065058UL);
	assert(pixldump_murmur3(data + 1, 5U) == 0x231c3edfUL);
	assert(pixldump_murmur3(data + 1, 15U) == 0xd70e8763UL);
	assert(pixldump_murmur3(data + 1, 16U) == 0xfbe919b0UL);
	assert(pixldump_murmur3(data + 1, 17U) == 0x70596859UL);
	assert(pixldump_murmur3(data + 1, 64000U) == 0x1ca3a1b0UL);
	assert(pixldump_murmur3(data + 1, 65535U) == 0x5a019218UL);
	memset(data, 0, sizeof(data));
	assert(pixldump_murmur3(data, 64000U) == 0x38a171e3UL);
	data[63999] = 1;
	assert(pixldump_murmur3(data, 64000U) == 0xbdfabd1fUL);
}

int main(void)
{
	test_text_vectors();
	test_binary_vectors();
	return 0;
}
