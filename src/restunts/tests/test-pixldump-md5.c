#include <assert.h>
#include <string.h>

#include "../pixldump/md5.h"

#define MD5_HEX_DIGITS_PER_BYTE   2U
#define MD5_HEX_OUTPUT_SIZE       \
	(PIXLDUMP_MD5_SIZE * MD5_HEX_DIGITS_PER_BYTE + 1U)
#define MD5_HEX_TERMINATOR_OFFSET (MD5_HEX_OUTPUT_SIZE - 1U)
#define MD5_HIGH_NIBBLE_SHIFT     4U
#define MD5_NIBBLE_MASK           15U
#define MD5_LOW_NIBBLE_OFFSET     1U
#define TEST_FRAMEBUFFER_SIZE     64000U

static void digest_to_hex(const legacy_u8 digest[PIXLDUMP_MD5_SIZE],
	char output[MD5_HEX_OUTPUT_SIZE])
{
	static const char digits[] = "0123456789abcdef";
	legacy_u16 index;

	for (index = 0; index < PIXLDUMP_MD5_SIZE; index++) {
		output[index * MD5_HEX_DIGITS_PER_BYTE] =
			digits[digest[index] >> MD5_HIGH_NIBBLE_SHIFT];
		output[index * MD5_HEX_DIGITS_PER_BYTE + MD5_LOW_NIBBLE_OFFSET] =
			digits[digest[index] & MD5_NIBBLE_MASK];
	}
	output[MD5_HEX_TERMINATOR_OFFSET] = 0;
}

static void test_text_vector(const char* input, const char* expected)
{
	legacy_u8 digest[PIXLDUMP_MD5_SIZE];
	char actual[MD5_HEX_OUTPUT_SIZE];

	pixldump_md5((const legacy_u8*)input,
		(legacy_u16)strlen(input), digest);
	digest_to_hex(digest, actual);
	assert(strcmp(actual, expected) == 0);
}

static void test_framebuffer_sized_input(void)
{
	static legacy_u8 framebuffer[TEST_FRAMEBUFFER_SIZE];
	legacy_u8 digest[PIXLDUMP_MD5_SIZE];
	char actual[MD5_HEX_OUTPUT_SIZE];

	memset(framebuffer, 0, sizeof(framebuffer));
	pixldump_md5(framebuffer, (legacy_u16)sizeof(framebuffer), digest);
	digest_to_hex(digest, actual);
	assert(strcmp(actual, "cf7cf997851fba0edbb0524841ce37bd") == 0);
}

int main(void)
{
	test_text_vector("", "d41d8cd98f00b204e9800998ecf8427e");
	test_text_vector("abc", "900150983cd24fb0d6963f7d28e17f72");
	test_text_vector("message digest",
		"f96b697d7cb7938d525a2f31aaf161d0");
	test_framebuffer_sized_input();
	return 0;
}
