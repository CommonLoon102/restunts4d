#include "md5.h"

#define MD5_STATE_WORD_COUNT 4U
#define MD5_BLOCK_SIZE 64U
#define MD5_BLOCK_WORD_COUNT 16U
#define MD5_WORD_SIZE 4U
#define MD5_PADDING_BYTE 128U
#define MD5_LENGTH_OFFSET 56U
#define MD5_LENGTH_HIGH_OFFSET 60U
#define MD5_BITS_PER_BYTE 8UL
#define MD5_INITIAL_A 1732584193UL
#define MD5_INITIAL_B 4023233417UL
#define MD5_INITIAL_C 2562383102UL
#define MD5_INITIAL_D 271733878UL

/* Original physics consumes addresses and stack residue from its renderer.
 * Preserve the oracle's code/data layout while optimizing the C port. */
#ifdef RESTUNTS_ORIGINAL
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

static const legacy_u8 md5_shifts[MD5_ROUND_COUNT] = {
	7,	12, 17, 22, 7,	12, 17, 22, 7,	12, 17, 22, 7,	12, 17, 22, 5,	9,	14, 20, 5,	9,
	14, 20, 5,	9,	14, 20, 5,	9,	14, 20, 4,	11, 16, 23, 4,	11, 16, 23, 4,	11, 16, 23,
	4,	11, 16, 23, 6,	10, 15, 21, 6,	10, 15, 21, 6,	10, 15, 21, 6,	10, 15, 21};

static const legacy_u32 md5_constants[MD5_ROUND_COUNT] = {
	3614090360UL, 3905402710UL, 606105819UL,  3250441966UL, 4118548399UL, 1200080426UL,
	2821735955UL, 4249261313UL, 1770035416UL, 2336552879UL, 4294925233UL, 2304563134UL,
	1804603682UL, 4254626195UL, 2792965006UL, 1236535329UL, 4129170786UL, 3225465664UL,
	643717713UL,  3921069994UL, 3593408605UL, 38016083UL,	3634488961UL, 3889429448UL,
	568446438UL,  3275163606UL, 4107603335UL, 1163531501UL, 2850285829UL, 4243563512UL,
	1735328473UL, 2368359562UL, 4294588738UL, 2272392833UL, 1839030562UL, 4259657740UL,
	2763975236UL, 1272893353UL, 4139469664UL, 3200236656UL, 681279174UL,  3936430074UL,
	3572445317UL, 76029189UL,	3654602809UL, 3873151461UL, 530742520UL,  3299628645UL,
	4096336452UL, 1126891415UL, 2878612391UL, 4237533241UL, 1700485571UL, 2399980690UL,
	4293915773UL, 2240044497UL, 1873313359UL, 4264355552UL, 2734768916UL, 1309151649UL,
	4149444226UL, 3174756917UL, 718787259UL,  3951481745UL};

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
			word_index =
				(legacy_u16)((MD5_ROUND_2_WORD_MULTIPLIER * index + MD5_ROUND_2_WORD_OFFSET) &
							 MD5_WORD_INDEX_MASK);
		} else if (index < MD5_ROUND_3_END) {
			function = b ^ c ^ d;
			word_index =
				(legacy_u16)((MD5_ROUND_3_WORD_MULTIPLIER * index + MD5_ROUND_3_WORD_OFFSET) &
							 MD5_WORD_INDEX_MASK);
		} else {
			function = c ^ (b | (~d));
			word_index = (legacy_u16)((MD5_ROUND_4_WORD_MULTIPLIER * index) & MD5_WORD_INDEX_MASK);
		}

		previous_d = d;
		d = c;
		c = b;
		sum = LEGACY_U32_WRAP_ADD(a, function);
		sum = LEGACY_U32_WRAP_ADD(sum, md5_constants[index]);
		sum = LEGACY_U32_WRAP_ADD(sum, words[word_index]);
		b = LEGACY_U32_WRAP_ADD(b, LEGACY_U32_ROL(sum, md5_shifts[index]));
		a = previous_d;
	}

	state[0] = LEGACY_U32_WRAP_ADD(state[0], a);
	state[1] = LEGACY_U32_WRAP_ADD(state[1], b);
	state[2] = LEGACY_U32_WRAP_ADD(state[2], c);
	state[3] = LEGACY_U32_WRAP_ADD(state[3], d);
}

#else

#define MD5_F(b, c, d) ((d) ^ ((b) & ((c) ^ (d))))
#define MD5_G(b, c, d) ((c) ^ ((d) & ((b) ^ (c))))
#define MD5_H(b, c, d) ((b) ^ (c) ^ (d))
#define MD5_I(b, c, d) ((c) ^ ((b) | (~(d))))
#define MD5_HALF_ROTATION_MASK 15U

/* Both shift counts stay below 16, including the unused zero-rotation case. */
#define MD5_ROTATE_HALF(low, high, shift)                                                          \
	((legacy_u16)(((low) << ((shift) & MD5_HALF_ROTATION_MASK)) |                                  \
				  ((high) >> ((LEGACY_WORD_BITS - (shift)) & MD5_HALF_ROTATION_MASK))))

/* Constant rounds avoid table dispatch and rotate the state by argument order.
 * Split each rotation into 16-bit halves: the DOS compiler otherwise calls two
 * far runtime shift helpers for every one of the 64 steps in every block. */
#define MD5_STEP(function, a, b, c, d, word, shift, constant)                                      \
	do {                                                                                           \
		legacy_u16 md5_low_;                                                                       \
		legacy_u16 md5_high_;                                                                      \
		(a) = LEGACY_U32_WRAP_ADD((a), function((b), (c), (d)));                                   \
		(a) = LEGACY_U32_WRAP_ADD((a), (constant));                                                \
		(a) = LEGACY_U32_WRAP_ADD((a), words[(word)]);                                             \
		md5_low_ =                                                                                 \
			(shift) < LEGACY_WORD_BITS ? (legacy_u16)(a) : (legacy_u16)((a) >> LEGACY_WORD_BITS);  \
		md5_high_ =                                                                                \
			(shift) < LEGACY_WORD_BITS ? (legacy_u16)((a) >> LEGACY_WORD_BITS) : (legacy_u16)(a);  \
		(a) = ((shift) & MD5_HALF_ROTATION_MASK) != 0U                                             \
				  ? (legacy_u32)MD5_ROTATE_HALF(md5_low_, md5_high_, (shift)) |                    \
						((legacy_u32)MD5_ROTATE_HALF(md5_high_, md5_low_, (shift))                 \
						 << LEGACY_WORD_BITS)                                                      \
				  : (legacy_u32)md5_low_ | ((legacy_u32)md5_high_ << LEGACY_WORD_BITS);            \
		(a) = LEGACY_U32_WRAP_ADD((a), (b));                                                       \
	} while (0)

static void md5_transform(legacy_u32 state[MD5_STATE_WORD_COUNT],
						  const legacy_u8 block[MD5_BLOCK_SIZE])
{
	legacy_u32 words[MD5_BLOCK_WORD_COUNT];
	legacy_u32 a;
	legacy_u32 b;
	legacy_u32 c;
	legacy_u32 d;
	legacy_u16 low;
	legacy_u16 high;
	legacy_u16 index;
	legacy_u16 word_index;

	for (index = 0; index < MD5_BLOCK_WORD_COUNT; index++) {
		word_index = (legacy_u16)(index * MD5_WORD_SIZE);
		low = (legacy_u16)(block[word_index] |
						   ((legacy_u16)block[word_index + 1U] << LEGACY_BYTE_BITS));
		high = (legacy_u16)(block[word_index + 2U] |
							((legacy_u16)block[word_index + 3U] << LEGACY_BYTE_BITS));
		words[index] = (legacy_u32)low | ((legacy_u32)high << LEGACY_WORD_BITS);
	}

	a = state[0];
	b = state[1];
	c = state[2];
	d = state[3];

	MD5_STEP(MD5_F, a, b, c, d, 0U, 7U, 3614090360UL);
	MD5_STEP(MD5_F, d, a, b, c, 1U, 12U, 3905402710UL);
	MD5_STEP(MD5_F, c, d, a, b, 2U, 17U, 606105819UL);
	MD5_STEP(MD5_F, b, c, d, a, 3U, 22U, 3250441966UL);
	MD5_STEP(MD5_F, a, b, c, d, 4U, 7U, 4118548399UL);
	MD5_STEP(MD5_F, d, a, b, c, 5U, 12U, 1200080426UL);
	MD5_STEP(MD5_F, c, d, a, b, 6U, 17U, 2821735955UL);
	MD5_STEP(MD5_F, b, c, d, a, 7U, 22U, 4249261313UL);
	MD5_STEP(MD5_F, a, b, c, d, 8U, 7U, 1770035416UL);
	MD5_STEP(MD5_F, d, a, b, c, 9U, 12U, 2336552879UL);
	MD5_STEP(MD5_F, c, d, a, b, 10U, 17U, 4294925233UL);
	MD5_STEP(MD5_F, b, c, d, a, 11U, 22U, 2304563134UL);
	MD5_STEP(MD5_F, a, b, c, d, 12U, 7U, 1804603682UL);
	MD5_STEP(MD5_F, d, a, b, c, 13U, 12U, 4254626195UL);
	MD5_STEP(MD5_F, c, d, a, b, 14U, 17U, 2792965006UL);
	MD5_STEP(MD5_F, b, c, d, a, 15U, 22U, 1236535329UL);

	MD5_STEP(MD5_G, a, b, c, d, 1U, 5U, 4129170786UL);
	MD5_STEP(MD5_G, d, a, b, c, 6U, 9U, 3225465664UL);
	MD5_STEP(MD5_G, c, d, a, b, 11U, 14U, 643717713UL);
	MD5_STEP(MD5_G, b, c, d, a, 0U, 20U, 3921069994UL);
	MD5_STEP(MD5_G, a, b, c, d, 5U, 5U, 3593408605UL);
	MD5_STEP(MD5_G, d, a, b, c, 10U, 9U, 38016083UL);
	MD5_STEP(MD5_G, c, d, a, b, 15U, 14U, 3634488961UL);
	MD5_STEP(MD5_G, b, c, d, a, 4U, 20U, 3889429448UL);
	MD5_STEP(MD5_G, a, b, c, d, 9U, 5U, 568446438UL);
	MD5_STEP(MD5_G, d, a, b, c, 14U, 9U, 3275163606UL);
	MD5_STEP(MD5_G, c, d, a, b, 3U, 14U, 4107603335UL);
	MD5_STEP(MD5_G, b, c, d, a, 8U, 20U, 1163531501UL);
	MD5_STEP(MD5_G, a, b, c, d, 13U, 5U, 2850285829UL);
	MD5_STEP(MD5_G, d, a, b, c, 2U, 9U, 4243563512UL);
	MD5_STEP(MD5_G, c, d, a, b, 7U, 14U, 1735328473UL);
	MD5_STEP(MD5_G, b, c, d, a, 12U, 20U, 2368359562UL);

	MD5_STEP(MD5_H, a, b, c, d, 5U, 4U, 4294588738UL);
	MD5_STEP(MD5_H, d, a, b, c, 8U, 11U, 2272392833UL);
	MD5_STEP(MD5_H, c, d, a, b, 11U, 16U, 1839030562UL);
	MD5_STEP(MD5_H, b, c, d, a, 14U, 23U, 4259657740UL);
	MD5_STEP(MD5_H, a, b, c, d, 1U, 4U, 2763975236UL);
	MD5_STEP(MD5_H, d, a, b, c, 4U, 11U, 1272893353UL);
	MD5_STEP(MD5_H, c, d, a, b, 7U, 16U, 4139469664UL);
	MD5_STEP(MD5_H, b, c, d, a, 10U, 23U, 3200236656UL);
	MD5_STEP(MD5_H, a, b, c, d, 13U, 4U, 681279174UL);
	MD5_STEP(MD5_H, d, a, b, c, 0U, 11U, 3936430074UL);
	MD5_STEP(MD5_H, c, d, a, b, 3U, 16U, 3572445317UL);
	MD5_STEP(MD5_H, b, c, d, a, 6U, 23U, 76029189UL);
	MD5_STEP(MD5_H, a, b, c, d, 9U, 4U, 3654602809UL);
	MD5_STEP(MD5_H, d, a, b, c, 12U, 11U, 3873151461UL);
	MD5_STEP(MD5_H, c, d, a, b, 15U, 16U, 530742520UL);
	MD5_STEP(MD5_H, b, c, d, a, 2U, 23U, 3299628645UL);

	MD5_STEP(MD5_I, a, b, c, d, 0U, 6U, 4096336452UL);
	MD5_STEP(MD5_I, d, a, b, c, 7U, 10U, 1126891415UL);
	MD5_STEP(MD5_I, c, d, a, b, 14U, 15U, 2878612391UL);
	MD5_STEP(MD5_I, b, c, d, a, 5U, 21U, 4237533241UL);
	MD5_STEP(MD5_I, a, b, c, d, 12U, 6U, 1700485571UL);
	MD5_STEP(MD5_I, d, a, b, c, 3U, 10U, 2399980690UL);
	MD5_STEP(MD5_I, c, d, a, b, 10U, 15U, 4293915773UL);
	MD5_STEP(MD5_I, b, c, d, a, 1U, 21U, 2240044497UL);
	MD5_STEP(MD5_I, a, b, c, d, 8U, 6U, 1873313359UL);
	MD5_STEP(MD5_I, d, a, b, c, 15U, 10U, 4264355552UL);
	MD5_STEP(MD5_I, c, d, a, b, 6U, 15U, 2734768916UL);
	MD5_STEP(MD5_I, b, c, d, a, 13U, 21U, 1309151649UL);
	MD5_STEP(MD5_I, a, b, c, d, 4U, 6U, 4149444226UL);
	MD5_STEP(MD5_I, d, a, b, c, 11U, 10U, 3174756917UL);
	MD5_STEP(MD5_I, c, d, a, b, 2U, 15U, 718787259UL);
	MD5_STEP(MD5_I, b, c, d, a, 9U, 21U, 3951481745UL);

	state[0] = LEGACY_U32_WRAP_ADD(state[0], a);
	state[1] = LEGACY_U32_WRAP_ADD(state[1], b);
	state[2] = LEGACY_U32_WRAP_ADD(state[2], c);
	state[3] = LEGACY_U32_WRAP_ADD(state[3], d);
}

#endif

void pixldump_md5(const legacy_u8 far *source, legacy_u16 length,
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
		for (index = 0; index < MD5_BLOCK_SIZE; index++) {
			block[index] = source[index];
		}
		md5_transform(state, block);
		source += MD5_BLOCK_SIZE;
		remaining = (legacy_u16)(remaining - MD5_BLOCK_SIZE);
	}

	block_length = remaining;
	for (index = 0; index < block_length; index++) {
		block[index] = source[index];
	}
	block[block_length++] = MD5_PADDING_BYTE;

	if (block_length > MD5_LENGTH_OFFSET) {
		while (block_length < MD5_BLOCK_SIZE) {
			block[block_length++] = 0;
		}
		md5_transform(state, block);
		block_length = 0;
	}

	while (block_length < MD5_LENGTH_OFFSET) {
		block[block_length++] = 0;
	}
	bit_length = (legacy_u32)length * MD5_BITS_PER_BYTE;
	LEGACY_WRITE_U32_LE(&block[MD5_LENGTH_OFFSET], bit_length);
	LEGACY_WRITE_U32_LE(&block[MD5_LENGTH_HIGH_OFFSET], 0UL);
	md5_transform(state, block);

	for (index = 0; index < MD5_STATE_WORD_COUNT; index++) {
		LEGACY_WRITE_U32_LE(&digest[index * MD5_WORD_SIZE], state[index]);
	}
}
