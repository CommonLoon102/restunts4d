#include <stddef.h>
#include "externs.h"
#include "fileio.h"
#include "memmgr.h"
#include "platform.h"

#ifdef RESTUNTS_HEADLESS
static const legacy_s8 headless_file_error[] = "File error: %s";
static const legacy_s8 headless_write_error[] = "File write error: %s";
static const legacy_s8 headless_size_error[] = "File size error: %s";
static const legacy_s8 headless_pack_error[] = "Invalid packed resource: %s";
#define aSFileError headless_file_error
#define aSFileError_0 headless_write_error
#define aSFileError_1 headless_size_error
#define aSInvalidPackTy headless_pack_error
#endif

#define PAGE_SIZE 0x4000
#define PAGE_GAP  0x400
#define FILENAME_LEN 13

#define RS_RLE_ESCLEN_MAX    0x10
#define RS_RLE_ESCLOOKUP_LEN 0x100
#define RS_RLE_ESCSEQ_POS    0x01

#define RS_VLE_ESC_LEN   0x10
#define RS_VLE_ALPH_LEN  0x100
#define RS_VLE_ESC_WIDTH 0x40
#define RS_VLE_NUM_SYMB  0x80

#define COMPR_HEADER_SIZE 4U
#define COMPR_SIZE_LOW_OFFSET 1U
#define COMPR_SIZE_HIGH_OFFSET 3U

#define COMPR_RLE_HEADER_SIZE 5U
#define COMPR_RLE_SIZE_HIGH_OFFSET 2U
#define COMPR_RLE_ESCLEN_OFFSET 4U

// Minimal stdio.h "support" until we can link with a real CRT.
#ifndef __STDIO_H
#define __STDIO_H
typedef legacy_u16 FILE;

FILE* fopen(const legacy_s8* path, const legacy_s8* mode)
{
	return (FILE*)dos_file_open(path, mode[0] == 'w');
}

legacy_s16 fclose(FILE* file)
{
	return dos_file_close((legacy_u16)file);
}

size_t fread(void far* dst, size_t size, size_t nmemb, FILE* file)
{
	return dos_file_read((legacy_u16)file, dst,
		(legacy_u16)(size * nmemb));
}

size_t fwrite(const void far* src, size_t size, size_t nmemb, FILE* file)
{
	return dos_file_write((legacy_u16)file, src,
		(legacy_u16)(size * nmemb));
}

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

legacy_s16 fseek(FILE *file, legacy_s32 offset, legacy_s16 origin)
{
	return dos_file_seek((legacy_u16)file, offset, origin);
}
legacy_s32 ftell(FILE *file)
{
	return dos_file_tell((legacy_u16)file);
}

legacy_s16 ferror(FILE* file)
{
	(void)file;
	return dos_file_error();
}

legacy_s16 remove(const legacy_s8* path)
{
	return dos_file_remove(path);
}
#endif

struct file_find_dos {
	legacy_s8 path[128];
	legacy_s8* dirdelim;
} g_find;

// Find file matching given query. Returns pointer to first matched filename
// including path from the query. NULL is returned on error/no hits.
// FIXME: DOS specific implementation.
const legacy_s8* file_find(const legacy_s8* query)
{
	legacy_s8 const* chsrc;
	legacy_s8* chdst;
	const legacy_s8* found_name;

	found_name = dos_file_find_first(query);
	if (found_name == 0) {
		return  0;
	}

	// Copy path from query.
	chdst = g_find.dirdelim = g_find.path;
	for (chsrc = query; *chsrc; ++chsrc, ++chdst) {
		*chdst = *chsrc;
		
		if (*chdst == ':' || *chdst == '\\') {
			g_find.dirdelim = chdst + 1;
		}
	}

	// Copy found filename to result path.
	memcpy(g_find.dirdelim, found_name, FILENAME_LEN);

	return g_find.path;
}

// Returns next found filename from file_find() query.
// FIXME: DOS specific implementation.
const legacy_s8* file_find_next(void)
{
	const legacy_s8* found_name;

	found_name = dos_file_find_next();
	if (found_name == 0) {
		return  0;
	}

	// Copy found filename to result path.
	memcpy(g_find.dirdelim, found_name, FILENAME_LEN);

	return g_find.path;
}

const legacy_s8* file_find_next_alt(void)
{
	return file_find_next();
}

void file_build_path(const legacy_s8* dir, const legacy_s8* name, const legacy_s8* ext, legacy_s8* dst)
{
	legacy_s16 dirlen;

	if (dir) {
		strcpy(dst, dir);
		dirlen = strlen(dir);
	}
	else {
		dst[0] = 0;
		dirlen = 0;
	}
	
	// Add directory separator if needed.
	if (dirlen && dir[dirlen - 1] != ':' && dir[dirlen - 1] != '\\') {
		strcat(dst, "\\");
	}
	
	strcat(dst, name);
	strcat(dst, ext);
}

const legacy_s8* file_combine_and_find(const legacy_s8* dir, const legacy_s8* name, const legacy_s8* ext)
{
	// The original reserves 80 bytes of stack for this (var_50 = byte ptr -80).
	legacy_s8 path[80];

	file_build_path(dir, name, ext, path);
	
	return file_find(path);
}

// Get number of 16-byte blocks needed to store entire file.
legacy_u16 file_paras(const legacy_s8* filename, legacy_s16 fatal)
{
	legacy_s32 length;
	FILE* file;
	if ((file = fopen(filename, "rb")) != 0) {
		fseek(file, 0, SEEK_END);
		length = ftell(file);
		fclose(file);
		
		if (!ferror(file)) {
			// May overflow, but all Stunts files are rather small.
			return (length >> 4) + (length & 0xF ? 1 : 0);
		}
	}

	if (fatal) {
		fatal_error(aSFileError, filename);
	}

	return 0;
}

legacy_u16 file_paras_fatal(const legacy_s8* filename)
{
	return file_paras(filename, 1);
}

legacy_u16 file_paras_nofatal(const legacy_s8* filename)
{
	return file_paras(filename, 0);
}

// Get number of 16-byte blocks needed to store the final result of an assumed compressed file.
legacy_u16 file_decomp_paras(const legacy_s8* filename, legacy_s16 fatal)
{
	legacy_s32 length;
	FILE* file;
	legacy_u8 header[COMPR_HEADER_SIZE];
	if ((file = fopen(filename, "rb")) != 0) {
		fread(header, sizeof(header), 1, file);
		fclose(file);
		
		if (!ferror(file)) {
			// May overflow, but all Stunts files are rather small.
			length = (legacy_s32)LEGACY_READ_U16_LE(
				header + COMPR_SIZE_LOW_OFFSET) |
				((legacy_s32)header[COMPR_SIZE_HIGH_OFFSET] << 16);
			return (length >> 4) + (length & 0xF ? 1 : 0);
		}
	}

	if (fatal) {
		fatal_error(aSFileError_1, filename);
	}

	return 0;
}

legacy_u16 file_decomp_paras_fatal(const legacy_s8* filename)
{
	return file_decomp_paras(filename, 1);
}

legacy_u16 file_decomp_paras_nofatal(const legacy_s8* filename)
{
	return file_decomp_paras(filename, 0);
}

// Read entire file to given destination. Optionally handle errors as fatal.
void far* file_read(const legacy_s8* filename, void far* dst, legacy_s16 fatal)
{
	legacy_s16 readlen;
	void far* curdst = dst;
	FILE* file;

	if ((file = fopen(filename, "rb")) != 0) {
		// Read one page at a time.
		do {
			readlen = fread(curdst, PAGE_SIZE, 1, file);
			curdst = dos_memory_make_pointer(
				dos_memory_pointer_segment(curdst) + PAGE_GAP,
				dos_memory_pointer_offset(dst));
		} while (readlen == PAGE_SIZE);

		fclose(file);

		if (!ferror(file)) {
			return dst;
		}
	}

	if (fatal) {
		fatal_error(aSFileError, filename);
	}

	return 0;
}

// Read entire file to given destination, handle errors as fatal.
void far* file_read_fatal(const legacy_s8* filename, void far* dst)
{
	return file_read(filename, dst, 1);
}

// Read entire file to given destination, returns NULL pointer if errors occur.
void far* file_read_nofatal(const legacy_s8* filename, void far* dst)
{
	return file_read(filename, dst, 0);
}

// Write given source buffer to file.
//
// NOTE the polarity of `fatal`: the original tests it the other way round
// (`cmp [bp+var_fatal],0 / jnz` jumps to the RETURN), so the entry point named
// file_write_fatal, which passes 1, hands the DOS error code back to its
// caller, and file_write_nofatal, which passes 0, is the one that aborts.
// That looks like a slip in the 1990 code, but the callers are built around
// it - asmorig/seg009.asm:2299 checks the result and puts up a disk-error
// panel - so it is reproduced rather than corrected.
legacy_s16 file_write(const legacy_s8* filename, void far* src, legacy_u32 length, legacy_s16 fatal)
{
	legacy_u16 retval;
	legacy_u16 wrtlen;
	FILE* file;
	
	retval = 0;

	if ((file = fopen(filename, "wb")) != 0) {
		// Write one page at a time.
		while (length != 0) {
			wrtlen = length > PAGE_SIZE ? PAGE_SIZE : length;

			if (fwrite(src, wrtlen, 1, file) != wrtlen) {
				// Either int 21h AH=40h set carry, or it wrote fewer bytes
				// than asked (`cmp ax, cx / jnz` -> `mov ax, 1`).
				retval = 1;
				break;
			}
			length -= wrtlen;
			src = dos_memory_make_pointer(
				dos_memory_pointer_segment(src) + PAGE_GAP,
				dos_memory_pointer_offset(src));
		}

		fclose(file);

		// The original ignores the close result. Clear the shim's sticky error
		// state without turning a close failure into a failed write.
		(void)ferror(file);

		if (retval == 0) {
			return 0;
		}
	}
	else {
		// The create failed. The original enters the error tail with the DOS
		// error code in ax; the shim does not carry that out, so any non-zero
		// status stands in for it - every caller only tests for non-zero.
		retval = 1;
	}

	// loc_32570 closes the handle and unlinks the file on EVERY error, before
	// it ever looks at the flag, so a truncated file is never left on disk.
	remove(filename);

	if (!fatal) {
		fatal_error(aSFileError_0, filename);
	}

	return retval;
}

// Write given source buffer to file, handle errors as fatal.
legacy_s16 file_write_fatal(const legacy_s8* filename, void far* src, legacy_u32 length)
{
	return file_write(filename, src, length, 1);
}

// Write given source buffer to file, returns a non-zero value on error.
legacy_s16 file_write_nofatal(const legacy_s8* filename, void far* src, legacy_u32 length)
{
	return file_write(filename, src, length, 0);
}

// Sequential byte runs pass of run-length encoding.
//
// The original opens with a bail-out this has no equivalent for:
//
//     cmp     byte ptr [bp-12h], 1    ; file_decomp_rle::var_esclen
//     jnz     short has_codes
//     retn
//
// Not reproduced. With a single escape code the caller's
// subhdr->esc[RS_RLE_ESCSEQ_POS] would be reading past the table anyway, and
// the original's own bail-out returns with ax:dx undefined, which the caller
// then feeds into copy_paras_reverse - so neither side is coherent there. No
// Stunts resource carries esclen == 1.
legacy_u32 file_decomp_rle_seq(legacy_u8 huge* src, legacy_u8 huge* dst, legacy_u32 srclen, legacy_u8 esc)
{
	legacy_u8 cur, rep;
	legacy_u8 huge* seqstart, huge* seqend;

	legacy_u8 huge* srcend = src + srclen;
	legacy_u8 huge* dststart = dst;

	while (src < srcend) {
		cur = *src++;

		// Byte sequence start.
		if (cur == esc) {
			seqstart = src;

			// Copy sequence.
			while ((cur = *src++) != esc) {
				*dst++ = cur;
			}

			 // Number of repetitions, already written once.
			rep = (*src++) - 1;
			seqend = src;

			// Copy remaining repetitions. The original is a do-while
			// (`dec dl / jnz short loc_30D5A`), so a stored count of 1
			// makes dl wrap and it copies 256 times where this copies none.
			// Counts of 2 and up agree exactly, and a count of 1 would mean
			// "repeat this run once", which the encoder has no reason to
			// emit.
			while (rep--) {
				src = seqstart;
				while (src < seqend - 2) {
					*dst++ = *src++;
				}
			}

			src = seqend;
		}
		// No sequence.
		else {
			*dst++ = cur;
		}
	}

	return dst - dststart;
}

// Single byte runs pass of run-length encoding.
legacy_u32 file_decomp_rle_single(legacy_u8 huge* src, legacy_u8 huge* dst, legacy_u32 len, legacy_u8* esclookup)
{
	legacy_u8 cur, rep;
	legacy_u16 repw;

	legacy_u8 huge* dststart = dst;
	legacy_u8 huge* dstend = dst + len;

	while (dst < dstend) {
		cur = *src++;

		if (esclookup[cur]) {
			switch (esclookup[cur]) {
				case 1:
					rep = *src++;
					cur = *src++;

					while (rep--) {
						*dst++ = cur;
					}
					break;

				case 3:
					repw = *src++;
					repw |= *src++ << 8;
					cur = *src++;

					while (repw--) {
						*dst++ = cur;
					}
					break;

				default:
					rep = esclookup[cur] - 1;
					cur = *src++;

					while (rep--) {
						*dst++ = cur;
					}
					break;
			}
		}
		else {
			*dst++ = cur;
		}
	}

	return dst - dststart;
}

// Decompress run-length encoded sub-file.
legacy_u32 file_decomp_rle(legacy_u8 huge* src, legacy_u8 huge* dst, legacy_u16 decompparas)
{
	legacy_u32 len, srclen, passlen;
	legacy_u16 skipseq, i;
	legacy_u8 esclookup[RS_RLE_ESCLOOKUP_LEN];
	legacy_u8 huge* origsrc;
	legacy_u8 huge* escapes;
	legacy_u16 paras;
	legacy_u8 esclen;

	(void)decompparas;

	// Get decompressed size from header.
	len = LEGACY_READ_U16_LE(src + COMPR_SIZE_LOW_OFFSET) |
		((legacy_u32)src[COMPR_SIZE_HIGH_OFFSET] << 16);
	origsrc = src += COMPR_HEADER_SIZE;
	
	// Get source size and escape codes.
	srclen = LEGACY_READ_U16_LE(src) |
		((legacy_u32)src[COMPR_RLE_SIZE_HIGH_OFFSET] << 16);
	esclen = src[COMPR_RLE_ESCLEN_OFFSET];
	escapes = src + COMPR_RLE_HEADER_SIZE;
	
	// MSB denotes skipping the initial pass for byte sequence runs. Match the
	// original's strict `cmp ... 80h / ja`: exactly 80h still runs the pass,
	// using the byte in the sequence-escape slot even though the declared
	// escape-code count is zero.
	skipseq = esclen > 0x80;
	esclen &= (legacy_u8)~0x80U;

	// Set pos to after escape codes.
	src = origsrc + COMPR_RLE_HEADER_SIZE + esclen;

	// Escape code lookup.
	for (i = 0; i < RS_RLE_ESCLOOKUP_LEN; ++i) {
		esclookup[i] = 0;
	}

	for (i = 0; i < esclen; ++i) {
		esclookup[escapes[i]] = i + 1;
	}

	if (!skipseq) {
		passlen = file_decomp_rle_seq(
			src, dst, srclen, escapes[RS_RLE_ESCSEQ_POS]);

		paras = (passlen >> 4) + (passlen & 0xF ? 1 : 0);

		// In main decomp func:
		src = dos_memory_make_pointer(
			decompparas - paras + dos_memory_pointer_segment(dst),
			dos_memory_pointer_offset(dst));
		copy_paras_reverse(dos_memory_pointer_segment(dst),
			dos_memory_pointer_segment(src), paras);
	}

	// The original discards the single pass's count and returns the size out
	// of the compression header instead:
	//
	//     call    near ptr file_decomp_rle_single
	//     mov     ax, [bp+var_lenlo]
	//     mov     dx, [bp+var_lenhi]
	//
	// The two only differ when the last run overshoots - file_decomp_rle_single
	// stops on `dst < dstend` and a run that straddles the end writes past it,
	// so its count can exceed len. The declared length must still be returned:
	// file_decomp uses it to position the source for a following pass.
	file_decomp_rle_single(src, dst, len, esclookup);
	return len;
}

// Decompress variable-length encoded sub-file.
legacy_u32 file_decomp_vle(legacy_u8 huge* src, legacy_u8 huge* dst, legacy_u16 decompparas)
{
	legacy_u32 len, lenleft;
	legacy_u16 additive, alphlen, width, widthdistr, i, j;
	legacy_u16 esc1[RS_VLE_ESC_LEN], esc2[RS_VLE_ESC_LEN];
	legacy_u8 alph[RS_VLE_ALPH_LEN], symb[RS_VLE_ALPH_LEN], wdth[RS_VLE_ALPH_LEN];
	legacy_u8 esclen, symbwdth, numsymb, numsymbleft, cursymb, tmp;
	legacy_u8 curwdt, nextwdt, code;
	legacy_u16 curword;

	legacy_u8 huge* wdtpos;
	legacy_u8 huge* codpos;

	(void)decompparas;

	// Get decompressed size from header.
	len = lenleft = LEGACY_READ_U16_LE(src + COMPR_SIZE_LOW_OFFSET) |
		((legacy_u32)src[COMPR_SIZE_HIGH_OFFSET] << 16);
	src += COMPR_HEADER_SIZE;

	// One-byte escape codes length counter.
	esclen = *src++;
	additive = (esclen & 0x80) == 0x80; // MSB is additive flag
	esclen &= ~0x80;

	// Store postion of width data for later.
	wdtpos = src;

	// Generate escape codes.
	for (i = 0, j = 0, alphlen = 0; i < esclen; ++i, j *= 2) {
		esc1[i] = alphlen - j;
		tmp = *src++;
		j += tmp;
		alphlen += tmp;
		esc2[i] = j;
	}

	// Read alphabet.
	for (i = 0; i < alphlen; ++i) {
		alph[i] = *src++;
	}

	// Store start position of compression codes, roll back to code width data.
	codpos = src;
	src = wdtpos;

	// Generate lookup tables.
	width = 1;
	widthdistr = (esclen >= 8 ? 8 : esclen);
	numsymb = RS_VLE_NUM_SYMB;
	for (i = 0, j = 0; width <= widthdistr; ++width, numsymb >>= 1) {
		for (symbwdth = *src++; symbwdth > 0; --symbwdth, ++j) {
			for (numsymbleft = numsymb; numsymbleft; --numsymbleft, ++i) {
				symb[i] = alph[j];
				wdth[i] = width;
			}
		}
	}

	// Pad widths.
	for (; i < RS_VLE_ALPH_LEN; ++i) {
		wdth[i] = RS_VLE_ESC_WIDTH;
	}

	// Go to compression codes.
	src = codpos;

	curword = *src << 8 | *(src + 1);
	src += 2;
	curwdt = 8;
	cursymb = 0;

	++lenleft;
	while (lenleft) {
		code = curword >> 8;
		nextwdt = wdth[code];
		// Expand.
		if (nextwdt > 8) {
			code = curword;
			curword >>= 8;

			i = 7;
			while (1) {
				if (!curwdt) {
					code = *src++;
					curwdt = 8;
				}

				curword = (curword << 1) + ((code & 0x80) == 0x80);
				code <<= 1;
				--curwdt;
				++i;

				if (curword < esc2[i]) {
					curword += esc1[i];

					if (additive) {
						cursymb += alph[curword];
					}
					else {
						cursymb = alph[curword];
					}
					*dst++ = cursymb;
					--lenleft;

					break;
				}
			}

			curword = (code << curwdt) | *src++;
			nextwdt = 8 - curwdt;
			curwdt = 8;
		}
		// Direct lookup.
		else {
			if (additive) {
				cursymb += symb[code];
			}
			else {
				cursymb = symb[code];
			}

			*dst++ = cursymb;
			--lenleft;

			if (curwdt < nextwdt) {
				curword <<= curwdt;
				nextwdt -= curwdt;
				curwdt = 8;
				curword |= *src++;
			}
		}

		curword <<= nextwdt;
		curwdt -= nextwdt;
	}

	return len;
}

// Decompress file. Returns pointer to result, NULL or raises fatal error.
void far* file_decomp(const legacy_s8* filename, legacy_s16 fatal)
{
	legacy_u32 passlen;
	legacy_u16 paras, decompparas;
	legacy_u8 passes, type;
	legacy_u8 far* src;
	legacy_u8 far* dst;
	legacy_s16 err = 0;

	// Check if resource archive is already loaded.
	dst = mmgr_get_chunk_by_name(filename);
	if (dst) {
		return dst;
	}

	decompparas = file_decomp_paras(filename, fatal);

	if (decompparas) {
		// Allocate extra paragraphs for alphabet and escape tables
		// overhead used during decompression.
		decompparas += 4;
		dst = mmgr_alloc_pages(filename, decompparas);

		paras = file_paras(filename, fatal);
		if (paras) {
			src = dos_memory_make_pointer(
				decompparas - paras + dos_memory_pointer_segment(dst),
				dos_memory_pointer_offset(dst));
			src = file_read(filename, src, fatal);
			if (src) {
				passes = *src;

				// If the multi-pass flag is set, the first byte contains the
				// number of compression passes and the next three bytes holds
				// the final decompressed size.
				if (passes & 0x80) {
					passes &= ~0x80;
					src += 4; // Skip past length data.
				}
				// Flag not set, first byte is compression type of the first
				// and only pass.
				else {
					passes = 1;
				}

				// Decode all compression passes.
				while (!err && passes) {
					type = *src;

					switch (type) {
						case 1:
							passlen = file_decomp_rle(src, dst, decompparas);
							break;
						case 2:
							passlen = file_decomp_vle(src, dst, decompparas);
							break;
						default:
							err = 1;
					}

					// Set source for next pass.
					if (!err && (--passes != 0)) {
						paras = (passlen >> 4) + (passlen & 0xF ? 1 : 0);
						src = dos_memory_make_pointer(
							decompparas - paras +
								dos_memory_pointer_segment(dst),
							dos_memory_pointer_offset(dst));
						copy_paras_reverse(
							dos_memory_pointer_segment(dst),
							dos_memory_pointer_segment(src), paras);
					}
				}

				// Free unneeded overhead.
				if (!err) {
					decompparas -= 4;
					mmgr_resize_memory(dos_memory_pointer_offset(dst),
						dos_memory_pointer_segment(dst), decompparas);

					return dst;
				}
			}
		}
	}

	if (fatal) {
		fatal_error(aSInvalidPackTy, filename);
	}

	return 0;
}

void far* file_decomp_fatal(const legacy_s8* filename)
{
	return file_decomp(filename, 1);
}

void far* file_decomp_nofatal(const legacy_s8* filename)
{
	return file_decomp(filename, 0);
}

// Allocates, reads and returns a pointer to the contents of a binary file
void far* file_load_binary(const legacy_s8* filename, legacy_s16 fatal) {
	void far* memptr;
	legacy_s16 numparas;

	memptr = mmgr_get_chunk_by_name(filename);
	if (dos_memory_pointer_segment(memptr) != 0) return memptr;
	
	numparas = file_paras(filename, fatal);
	if (numparas == 0) return 0;
	memptr = mmgr_alloc_pages(filename, numparas);
	return file_read(filename, memptr, fatal);
}

void far* file_load_binary_nofatal(const legacy_s8* filename) {
	return file_load_binary(filename, 0);
}

void far* file_load_binary_fatal(const legacy_s8* filename) {
	return file_load_binary(filename, 1);
}

void far* file_load_resource(legacy_s16 type, const legacy_s8* filename) {
	void far* result;
#ifdef RESTUNTS_HEADLESS
	if (type == 0 || type == 1)
		result = file_load_binary_nofatal(filename);
	else if (type == 7)
		result = file_decomp_nofatal(filename);
	else
		result = 0;
	if (result == 0 && type == 0)
		fatal_error(headless_file_error, filename);
	return result;
#else
	legacy_s16 dearesult;
	while (1) {
		switch (type) {
			case 0:
				// try load the file, if it fails, show a dialog, and retry
				result = file_load_binary_nofatal(filename);
				if (result != 0) return result;
				break;

			case 1:
				return file_load_binary_nofatal(filename);

			case 2:
				// try load a 2d shape and retry if it failed
				result = file_load_shape2d_nofatal(filename);
				if (result != 0) return result;
				break;

			case 3:
				// try load a 2d shape and retry if it failed
				result = file_load_shape2d_res_nofatal(filename);
				if (result != 0) return result;
				break;

			case 4:
				// try load a song file and retry if it failed
				result = load_song_file(filename);
				if (result != 0) return result;
				break;

			case 5:
				// try load a voice file and retry if it failed
				result = load_voice_file(filename);
				if (result != 0) return result;
				break;

			case 6:
				// try load an sfx file and retry if it failed
				result = load_sfx_file(filename);
				if (result != 0) return result;
				break;

			case 7:
				// try load a compressed file
				return file_decomp_nofatal(filename);

			case 8:
				// try load a 2d shape and retry if it failed
				result = file_load_shape2d_nofatal2(filename);
				if (result != 0) return result;
				break;
			default:
				break;
		}

		dearesult = do_dea_textres();
		if (dearesult == 2) return 0;
	}
#endif
}


void far* file_load_resfile(const legacy_s8* filename) {
	legacy_s8 name[0x50];
	void far* result;
	
#ifdef RESTUNTS_HEADLESS
	strcpy(name, filename);
	strcat(name, ".res");
	result = file_load_resource(1, name);
	if (result != 0)
		return result;
	strcpy(name, filename);
	strcat(name, ".pre");
	result = file_load_resource(7, name);
	if (result == 0)
		fatal_error(headless_file_error, filename);
	return result;
#else
	while (1) {
		strcpy(name, filename);
		strcat(name, ".res");
		
		result = file_load_resource(1, name);
		if (result != 0) return result;
	
		strcpy(name, filename);
		strcat(name, ".pre");
		
		result = file_load_resource(7, name);
		if (result != 0) return result;
			
		do_dea_textres();
	}
#endif
}

void unload_resource(void far* resptr) {
	mmgr_free(resptr);
}

#ifndef RESTUNTS_HEADLESS
void far* file_load_3dres(const legacy_s8* filename) {
	legacy_s8 name[0x50];
	void far* result;
	
	while (1) {
		strcpy(name, filename);
		strcat(name, ".p3s");
		
		result = file_load_resource(7, name);
		if (result != 0) return result;
			
		strcpy(name, filename);
		strcat(name, ".3sh");
		
		result = file_load_resource(1, name);
		if (result != 0) return result;
	
		do_dea_textres();
	}
}

void file_load_audiores(const legacy_s8* songfile, const legacy_s8* voicefile, const legacy_s8* name) {
	void far* audiores;
	voicefileptr = file_load_resource(5, voicefile);
	songfileptr = file_load_resource(4, songfile);
	audiores = init_audio_resources(songfileptr, voicefileptr, name);
	load_audio_finalize(audiores);
	is_audioloaded = 1;
}
#endif

legacy_s16 file_load_replay(const legacy_s8* dir, const legacy_s8* name)
{
	file_build_path(dir, name, ".rpl", g_path_buf);

	g_is_busy = 1;
	file_read_fatal(g_path_buf, td13_rpl_header);
	replay_gameinfo_decode(&gameconfig,
		(const legacy_u8 far*)td13_rpl_header);
	g_is_busy = 0;
	return 0;
}

legacy_s16 file_write_replay(const legacy_s8* filename)
{
	legacy_s16 ret;

	replay_gameinfo_encode((legacy_u8 far*)td13_rpl_header, &gameconfig);

	g_is_busy = 1;
	ret = file_write_fatal(filename, td13_rpl_header,
		REPLAY_GAMEINFO_SIZE + gameconfig.game_recordedframes);
	g_is_busy = 0;
	
	return ret;
}
