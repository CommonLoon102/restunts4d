#include "restunts.h"

#define STRING_TERMINATOR '\0'
#define STRING_COMPARE_EQUAL 0
#define STRING_COMPARE_LESS (-1)
#define STRING_COMPARE_GREATER 1

void copy_string(legacy_s8* destination, legacy_s8 far* source)
{
	/* Preserve the original post-copy lookahead, including its empty input bug. */
	do {
		*destination = *source;
		destination++;
		source++;
	} while (*source != STRING_TERMINATOR);
	*destination = STRING_TERMINATOR;
}

legacy_s8* _strcpy(legacy_s8* destination, const legacy_s8* source)
{
	legacy_s8* result;

	result = destination;
	do {
		*destination = *source;
		destination++;
	} while (*source++ != STRING_TERMINATOR);
	return result;
}

legacy_u16 _strlen(const legacy_s8* string)
{
	const legacy_s8* end;

	end = string;
	while (*end != STRING_TERMINATOR)
		end++;
	return (legacy_u16)(end - string);
}

legacy_s8* _strcat(legacy_s8* destination, const legacy_s8* source)
{
	_strcpy(destination + _strlen(destination), source);
	return destination;
}

legacy_s16 _strcmp(const legacy_s8* left, const legacy_s8* right)
{
	const legacy_u8* left_bytes;
	const legacy_u8* right_bytes;

	left_bytes = (const legacy_u8*)left;
	right_bytes = (const legacy_u8*)right;
	while (*left_bytes == *right_bytes) {
		if (*left_bytes == STRING_TERMINATOR)
			return STRING_COMPARE_EQUAL;
		left_bytes++;
		right_bytes++;
	}
	return *left_bytes < *right_bytes ?
		STRING_COMPARE_LESS : STRING_COMPARE_GREATER;
}

static legacy_u8 legacy_ascii_lower(legacy_u8 character)
{
	if (character >= 'A' && character <= 'Z')
		return (legacy_u8)(character + ('a' - 'A'));
	return character;
}

legacy_s16 _stricmp(const legacy_s8* left, const legacy_s8* right)
{
	legacy_u8 left_character;
	legacy_u8 right_character;

	do {
		left_character = legacy_ascii_lower((legacy_u8)*left++);
		right_character = legacy_ascii_lower((legacy_u8)*right++);
		if (left_character != right_character)
			return left_character < right_character ?
				STRING_COMPARE_LESS : STRING_COMPARE_GREATER;
	} while (left_character != STRING_TERMINATOR);
	return STRING_COMPARE_EQUAL;
}
