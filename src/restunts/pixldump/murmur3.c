/* MurmurHash3_x86_32 by Austin Appleby (public domain), adapted for DOS.
 * Reference: https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp */
#include "murmur3.h"

#define MURMUR3_BLOCK_SIZE 4U
#define MURMUR3_KEY_MULTIPLIER_1 0xcc9e2d51UL
#define MURMUR3_KEY_MULTIPLIER_2 0x1b873593UL
#define MURMUR3_HASH_OFFSET 0xe6546b64UL
#define MURMUR3_FINAL_MULTIPLIER_1 0x85ebca6bUL
#define MURMUR3_FINAL_MULTIPLIER_2 0xc2b2ae35UL

#ifdef __BORLANDC__
/* x86 permits unaligned little-endian loads. Avoid the far shift helpers that
 * Borland otherwise emits while assembling bytes and rotating 32-bit words. */
#define MURMUR3_READ_WORD(source) (*(const legacy_u32 far *)(source))
typedef union murmur3_word {
	legacy_u32 bits;
	legacy_u16 halves[2];
} murmur3_word;
#define MURMUR3_ROTATE_LEFT(value, count)                                                          \
	do {                                                                                           \
		murmur3_word word;                                                                         \
		legacy_u16 low, high;                                                                      \
		word.bits = (value);                                                                       \
		low = word.halves[0];                                                                      \
		high = word.halves[1];                                                                     \
		word.halves[0] = (legacy_u16)((low << (count)) | (high >> (16U - (count))));               \
		word.halves[1] = (legacy_u16)((high << (count)) | (low >> (16U - (count))));               \
		(value) = word.bits;                                                                       \
	} while (0)
#else
#define MURMUR3_READ_WORD(source) LEGACY_READ_U32_LE(source)
#define MURMUR3_ROTATE_LEFT(value, count)                                                          \
	do {                                                                                           \
		(value) = LEGACY_U32_ROL(value, count);                                                    \
	} while (0)
#endif

#define MURMUR3_MIX_KEY(key)                                                                       \
	do {                                                                                           \
		(key) *= MURMUR3_KEY_MULTIPLIER_1;                                                         \
		MURMUR3_ROTATE_LEFT(key, 15U);                                                             \
		(key) *= MURMUR3_KEY_MULTIPLIER_2;                                                         \
	} while (0)

legacy_u32 pixldump_murmur3(const legacy_u8 far *source, legacy_u16 length)
{
	legacy_u32 hash = 0UL;
	legacy_u32 key;
	legacy_u16 remaining = length;
	legacy_u16 index;
	legacy_u8 tail[MURMUR3_BLOCK_SIZE];

	while (remaining >= MURMUR3_BLOCK_SIZE) {
		key = MURMUR3_READ_WORD(source);
		MURMUR3_MIX_KEY(key);
		hash ^= key;
		MURMUR3_ROTATE_LEFT(hash, 13U);
		hash = hash * 5UL + MURMUR3_HASH_OFFSET;
		source += MURMUR3_BLOCK_SIZE;
		remaining -= MURMUR3_BLOCK_SIZE;
	}
	if (remaining != 0) {
		for (index = 0; index < MURMUR3_BLOCK_SIZE; index++) {
			tail[index] = 0;
		}
		for (index = 0; index < remaining; index++) {
			tail[index] = source[index];
		}
		key = MURMUR3_READ_WORD(tail);
		MURMUR3_MIX_KEY(key);
		hash ^= key;
	}
	hash ^= length;
	hash ^= hash >> 16U;
	hash *= MURMUR3_FINAL_MULTIPLIER_1;
	hash ^= hash >> 13U;
	hash *= MURMUR3_FINAL_MULTIPLIER_2;
	hash ^= hash >> 16U;
	return hash;
}
