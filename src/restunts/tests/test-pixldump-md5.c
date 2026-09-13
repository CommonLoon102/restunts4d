#include <assert.h>
#include <string.h>

#include "../pixldump/md5.h"

#define MD5_HEX_DIGITS_PER_BYTE 2U
#define MD5_HEX_OUTPUT_SIZE (PIXLDUMP_MD5_SIZE * MD5_HEX_DIGITS_PER_BYTE + 1U)
#define MD5_HEX_TERMINATOR_OFFSET (MD5_HEX_OUTPUT_SIZE - 1U)
#define MD5_HIGH_NIBBLE_SHIFT 4U
#define MD5_NIBBLE_MASK 15U
#define MD5_LOW_NIBBLE_OFFSET 1U
#define TEST_FRAMEBUFFER_SIZE 64000U

static void digest_to_hex(const legacy_u8 digest[PIXLDUMP_MD5_SIZE],
						  char output[MD5_HEX_OUTPUT_SIZE])
{
	static const char digits[] = "0123456789abcdef";
	legacy_u16 index;

	for (index = 0; index < PIXLDUMP_MD5_SIZE; index++) {
		output[index * MD5_HEX_DIGITS_PER_BYTE] = digits[digest[index] >> MD5_HIGH_NIBBLE_SHIFT];
		output[index * MD5_HEX_DIGITS_PER_BYTE + MD5_LOW_NIBBLE_OFFSET] =
			digits[digest[index] & MD5_NIBBLE_MASK];
	}
	output[MD5_HEX_TERMINATOR_OFFSET] = 0;
}

static void test_text_vector(const char *input, const char *expected)
{
	legacy_u8 digest[PIXLDUMP_MD5_SIZE];
	char actual[MD5_HEX_OUTPUT_SIZE];

	pixldump_md5((const legacy_u8 *)input, (legacy_u16)strlen(input), digest);
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

static void test_patterned_inputs(void)
{
	static const struct {
		legacy_u16 length;
		const char *expected;
	} vectors[] = {
		{1U, "8d39dd7eef115ea6975446ef4082951f"},	  {55U, "05b6f147013ad96f3964c9eeb56d8baf"},
		{56U, "eb89fb7dc809845b334abe86a2a47a20"},	  {57U, "e9faaf7444b6438b09e9284f6dd87720"},
		{63U, "f4768d343cac289c9b58f06c36906fa4"},	  {64U, "65aa793f755efddcbfdd26d69bffec92"},
		{65U, "1e1d9a93592c75f925af743eb472b6fd"},	  {119U, "fcda3aaf770bffe1720a02c595564c2d"},
		{120U, "319c053d4d90b0ebf039c6bef68d7e72"},	  {127U, "64f86c54f82a0c18c344a0f80746f7b8"},
		{128U, "4469de88981f837969c9d36b25621796"},	  {129U, "d9defda2f2b6790db1501fae9a5b5083"},
		{1024U, "25a17d3f4127ef3bc9cbc6f41c99a7c3"},  {64000U, "c54f7139697330bdc92a67d32bd1ec23"},
		{65534U, "8eb42cb8eb9a221094caa3678dd5cde7"}, {65535U, "3f2f05349489a617eaf2019fffaa89dd"},
	};
	static legacy_u8 input[LEGACY_U16_MAX];
	legacy_u8 digest[PIXLDUMP_MD5_SIZE];
	char actual[MD5_HEX_OUTPUT_SIZE];
	legacy_u32 index;

	/* Independent hashlib vectors exercise both padding paths, high-bit bytes,
	 * multiple blocks, the framebuffer size, and the maximum 16-bit input length. */
	for (index = 0; index < sizeof(input); index++) {
		input[index] = (legacy_u8)(index * 37UL + (index >> LEGACY_BYTE_BITS) * 11UL + 128UL);
	}
	for (index = 0; index < sizeof(vectors) / sizeof(vectors[0]); index++) {
		pixldump_md5(input, vectors[index].length, digest);
		digest_to_hex(digest, actual);
		assert(strcmp(actual, vectors[index].expected) == 0);
	}
}

int main(void)
{
	test_text_vector("", "d41d8cd98f00b204e9800998ecf8427e");
	test_text_vector("abc", "900150983cd24fb0d6963f7d28e17f72");
	test_text_vector("message digest", "f96b697d7cb7938d525a2f31aaf161d0");
	test_framebuffer_sized_input();
	test_patterned_inputs();
	return 0;
}
