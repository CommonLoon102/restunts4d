#include <assert.h>
#include <string.h>

#include "../pixldump/md5.h"

static void digest_to_hex(const legacy_u8 digest[PIXLDUMP_MD5_SIZE],
	char output[33])
{
	static const char digits[] = "0123456789abcdef";
	legacy_u16 index;

	for (index = 0; index < PIXLDUMP_MD5_SIZE; index++) {
		output[index * 2U] = digits[digest[index] >> 4];
		output[index * 2U + 1U] = digits[digest[index] & 0x0FU];
	}
	output[32] = 0;
}

static void test_text_vector(const char* input, const char* expected)
{
	legacy_u8 digest[PIXLDUMP_MD5_SIZE];
	char actual[33];

	pixldump_md5((const legacy_u8*)input,
		(legacy_u16)strlen(input), digest);
	digest_to_hex(digest, actual);
	assert(strcmp(actual, expected) == 0);
}

static void test_framebuffer_sized_input(void)
{
	static legacy_u8 framebuffer[64000];
	legacy_u8 digest[PIXLDUMP_MD5_SIZE];
	char actual[33];

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
