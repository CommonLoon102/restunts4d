#ifndef RESTUNTS_PIXLDUMP_MD5_H
#define RESTUNTS_PIXLDUMP_MD5_H

#include "../c/legacy.h"

#define PIXLDUMP_MD5_SIZE 16U

void pixldump_md5(const legacy_u8 far* source, legacy_u16 length,
	legacy_u8 digest[PIXLDUMP_MD5_SIZE]);

#endif
