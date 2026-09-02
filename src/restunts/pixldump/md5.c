#include "md5.h"

static const legacy_u8 md5_shifts[64] = {
	7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
	5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
	4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
	6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21
};

static const legacy_u32 md5_constants[64] = {
	0xD76AA478UL, 0xE8C7B756UL, 0x242070DBUL, 0xC1BDCEEEUL,
	0xF57C0FAFUL, 0x4787C62AUL, 0xA8304613UL, 0xFD469501UL,
	0x698098D8UL, 0x8B44F7AFUL, 0xFFFF5BB1UL, 0x895CD7BEUL,
	0x6B901122UL, 0xFD987193UL, 0xA679438EUL, 0x49B40821UL,
	0xF61E2562UL, 0xC040B340UL, 0x265E5A51UL, 0xE9B6C7AAUL,
	0xD62F105DUL, 0x02441453UL, 0xD8A1E681UL, 0xE7D3FBC8UL,
	0x21E1CDE6UL, 0xC33707D6UL, 0xF4D50D87UL, 0x455A14EDUL,
	0xA9E3E905UL, 0xFCEFA3F8UL, 0x676F02D9UL, 0x8D2A4C8AUL,
	0xFFFA3942UL, 0x8771F681UL, 0x6D9D6122UL, 0xFDE5380CUL,
	0xA4BEEA44UL, 0x4BDECFA9UL, 0xF6BB4B60UL, 0xBEBFBC70UL,
	0x289B7EC6UL, 0xEAA127FAUL, 0xD4EF3085UL, 0x04881D05UL,
	0xD9D4D039UL, 0xE6DB99E5UL, 0x1FA27CF8UL, 0xC4AC5665UL,
	0xF4292244UL, 0x432AFF97UL, 0xAB9423A7UL, 0xFC93A039UL,
	0x655B59C3UL, 0x8F0CCC92UL, 0xFFEFF47DUL, 0x85845DD1UL,
	0x6FA87E4FUL, 0xFE2CE6E0UL, 0xA3014314UL, 0x4E0811A1UL,
	0xF7537E82UL, 0xBD3AF235UL, 0x2AD7D2BBUL, 0xEB86D391UL
};

static void md5_transform(legacy_u32 state[4], const legacy_u8 block[64])
{
	legacy_u32 words[16];
	legacy_u32 a;
	legacy_u32 b;
	legacy_u32 c;
	legacy_u32 d;
	legacy_u32 function;
	legacy_u32 sum;
	legacy_u32 previous_d;
	legacy_u16 index;
	legacy_u16 word_index;

	for (index = 0; index < 16U; index++) {
		word_index = (legacy_u16)(index * 4U);
		words[index] = LEGACY_READ_U32_LE(&block[word_index]);
	}

	a = state[0];
	b = state[1];
	c = state[2];
	d = state[3];

	for (index = 0; index < 64U; index++) {
		if (index < 16U) {
			function = (b & c) | ((~b) & d);
			word_index = index;
		} else if (index < 32U) {
			function = (d & b) | ((~d) & c);
			word_index = (legacy_u16)((5U * index + 1U) & 15U);
		} else if (index < 48U) {
			function = b ^ c ^ d;
			word_index = (legacy_u16)((3U * index + 5U) & 15U);
		} else {
			function = c ^ (b | (~d));
			word_index = (legacy_u16)((7U * index) & 15U);
		}

		previous_d = d;
		d = c;
		c = b;
		sum = LEGACY_U32_WRAP_ADD(a, function);
		sum = LEGACY_U32_WRAP_ADD(sum, md5_constants[index]);
		sum = LEGACY_U32_WRAP_ADD(sum, words[word_index]);
		b = LEGACY_U32_WRAP_ADD(b,
			LEGACY_U32_ROL(sum, md5_shifts[index]));
		a = previous_d;
	}

	state[0] = LEGACY_U32_WRAP_ADD(state[0], a);
	state[1] = LEGACY_U32_WRAP_ADD(state[1], b);
	state[2] = LEGACY_U32_WRAP_ADD(state[2], c);
	state[3] = LEGACY_U32_WRAP_ADD(state[3], d);
}

void pixldump_md5(const legacy_u8 far* source, legacy_u16 length,
	legacy_u8 digest[PIXLDUMP_MD5_SIZE])
{
	legacy_u32 state[4];
	legacy_u32 bit_length;
	legacy_u8 block[64];
	legacy_u16 remaining;
	legacy_u16 block_length;
	legacy_u16 index;

	state[0] = 0x67452301UL;
	state[1] = 0xEFCDAB89UL;
	state[2] = 0x98BADCFEUL;
	state[3] = 0x10325476UL;
	remaining = length;

	while (remaining >= 64U) {
		for (index = 0; index < 64U; index++)
			block[index] = source[index];
		md5_transform(state, block);
		source += 64U;
		remaining = (legacy_u16)(remaining - 64U);
	}

	block_length = remaining;
	for (index = 0; index < block_length; index++)
		block[index] = source[index];
	block[block_length++] = 0x80U;

	if (block_length > 56U) {
		while (block_length < 64U)
			block[block_length++] = 0;
		md5_transform(state, block);
		block_length = 0;
	}

	while (block_length < 56U)
		block[block_length++] = 0;
	bit_length = (legacy_u32)length * 8UL;
	LEGACY_WRITE_U32_LE(&block[56], bit_length);
	LEGACY_WRITE_U32_LE(&block[60], 0UL);
	md5_transform(state, block);

	for (index = 0; index < 4U; index++)
		LEGACY_WRITE_U32_LE(&digest[index * 4U], state[index]);
}
