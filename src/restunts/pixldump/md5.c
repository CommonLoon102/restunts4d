#include "md5.h"

#define MD5_STATE_WORD_COUNT 4U
#define MD5_BLOCK_SIZE 64U
#define MD5_BLOCK_WORD_COUNT 16U
#define MD5_WORD_SIZE 4U
#define MD5_ROUND_COUNT 64U
#define MD5_ROUND_1_END 16U
#define MD5_ROUND_2_END 32U
#define MD5_ROUND_3_END 48U
#define MD5_WORD_INDEX_MASK 15U
#define MD5_ROUND_2_WORD_MULTIPLIER 5U
#define MD5_ROUND_2_WORD_OFFSET 1U
#define MD5_ROUND_3_WORD_MULTIPLIER 3U
#define MD5_ROUND_3_WORD_OFFSET 5U
#define MD5_ROUND_4_WORD_MULTIPLIER 7U
#define MD5_PADDING_BYTE 128U
#define MD5_LENGTH_OFFSET 56U
#define MD5_LENGTH_HIGH_OFFSET 60U
#define MD5_BITS_PER_BYTE 8UL
#define MD5_INITIAL_A 1732584193UL
#define MD5_INITIAL_B 4023233417UL
#define MD5_INITIAL_C 2562383102UL
#define MD5_INITIAL_D 271733878UL

static const legacy_u8 md5_shifts[MD5_ROUND_COUNT] = {
	7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
	5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
	4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
	6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21
};

static const legacy_u32 md5_constants[MD5_ROUND_COUNT] = {
	3614090360UL, 3905402710UL, 606105819UL, 3250441966UL,
	4118548399UL, 1200080426UL, 2821735955UL, 4249261313UL,
	1770035416UL, 2336552879UL, 4294925233UL, 2304563134UL,
	1804603682UL, 4254626195UL, 2792965006UL, 1236535329UL,
	4129170786UL, 3225465664UL, 643717713UL, 3921069994UL,
	3593408605UL, 38016083UL, 3634488961UL, 3889429448UL,
	568446438UL, 3275163606UL, 4107603335UL, 1163531501UL,
	2850285829UL, 4243563512UL, 1735328473UL, 2368359562UL,
	4294588738UL, 2272392833UL, 1839030562UL, 4259657740UL,
	2763975236UL, 1272893353UL, 4139469664UL, 3200236656UL,
	681279174UL, 3936430074UL, 3572445317UL, 76029189UL,
	3654602809UL, 3873151461UL, 530742520UL, 3299628645UL,
	4096336452UL, 1126891415UL, 2878612391UL, 4237533241UL,
	1700485571UL, 2399980690UL, 4293915773UL, 2240044497UL,
	1873313359UL, 4264355552UL, 2734768916UL, 1309151649UL,
	4149444226UL, 3174756917UL, 718787259UL, 3951481745UL
};

static void md5_transform(legacy_u32 state[MD5_STATE_WORD_COUNT],
	const legacy_u8 block[MD5_BLOCK_SIZE])
{
	legacy_u32 words[MD5_BLOCK_WORD_COUNT];
	legacy_u32 a;
	legacy_u32 b;
	legacy_u32 c;
	legacy_u32 d;
	legacy_u32 function;
	legacy_u32 sum;
	legacy_u32 previous_d;
	legacy_u16 index;
	legacy_u16 word_index;

	for (index = 0; index < MD5_BLOCK_WORD_COUNT; index++) {
		word_index = (legacy_u16)(index * MD5_WORD_SIZE);
		words[index] = LEGACY_READ_U32_LE(&block[word_index]);
	}

	a = state[0];
	b = state[1];
	c = state[2];
	d = state[3];

	for (index = 0; index < MD5_ROUND_COUNT; index++) {
		if (index < MD5_ROUND_1_END) {
			function = (b & c) | ((~b) & d);
			word_index = index;
		} else if (index < MD5_ROUND_2_END) {
			function = (d & b) | ((~d) & c);
			word_index = (legacy_u16)((MD5_ROUND_2_WORD_MULTIPLIER *
				index + MD5_ROUND_2_WORD_OFFSET) & MD5_WORD_INDEX_MASK);
		} else if (index < MD5_ROUND_3_END) {
			function = b ^ c ^ d;
			word_index = (legacy_u16)((MD5_ROUND_3_WORD_MULTIPLIER *
				index + MD5_ROUND_3_WORD_OFFSET) & MD5_WORD_INDEX_MASK);
		} else {
			function = c ^ (b | (~d));
			word_index = (legacy_u16)((MD5_ROUND_4_WORD_MULTIPLIER *
				index) & MD5_WORD_INDEX_MASK);
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
	legacy_u32 state[MD5_STATE_WORD_COUNT];
	legacy_u32 bit_length;
	legacy_u8 block[MD5_BLOCK_SIZE];
	legacy_u16 remaining;
	legacy_u16 block_length;
	legacy_u16 index;

	state[0] = MD5_INITIAL_A;
	state[1] = MD5_INITIAL_B;
	state[2] = MD5_INITIAL_C;
	state[3] = MD5_INITIAL_D;
	remaining = length;

	while (remaining >= MD5_BLOCK_SIZE) {
		for (index = 0; index < MD5_BLOCK_SIZE; index++)
			block[index] = source[index];
		md5_transform(state, block);
		source += MD5_BLOCK_SIZE;
		remaining = (legacy_u16)(remaining - MD5_BLOCK_SIZE);
	}

	block_length = remaining;
	for (index = 0; index < block_length; index++)
		block[index] = source[index];
	block[block_length++] = MD5_PADDING_BYTE;

	if (block_length > MD5_LENGTH_OFFSET) {
		while (block_length < MD5_BLOCK_SIZE)
			block[block_length++] = 0;
		md5_transform(state, block);
		block_length = 0;
	}

	while (block_length < MD5_LENGTH_OFFSET)
		block[block_length++] = 0;
	bit_length = (legacy_u32)length * MD5_BITS_PER_BYTE;
	LEGACY_WRITE_U32_LE(&block[MD5_LENGTH_OFFSET], bit_length);
	LEGACY_WRITE_U32_LE(&block[MD5_LENGTH_HIGH_OFFSET], 0UL);
	md5_transform(state, block);

	for (index = 0; index < MD5_STATE_WORD_COUNT; index++)
		LEGACY_WRITE_U32_LE(&digest[index * MD5_WORD_SIZE], state[index]);
}
