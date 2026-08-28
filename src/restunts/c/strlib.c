#include "restunts.h"

legacy_s8* _strcpy(legacy_s8* destination, const legacy_s8* source)
{
	legacy_s8* result;

	result = destination;
	do {
		*destination = *source;
		destination++;
	} while (*source++ != '\0');
	return result;
}

legacy_u16 _strlen(const legacy_s8* string)
{
	const legacy_s8* end;

	end = string;
	while (*end != '\0')
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
		if (*left_bytes == '\0')
			return 0;
		left_bytes++;
		right_bytes++;
	}
	return *left_bytes < *right_bytes ? -1 : 1;
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
			return left_character < right_character ? -1 : 1;
	} while (left_character != '\0');
	return 0;
}
