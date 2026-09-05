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

#define FILE_IO_PAGE_SIZE 16384
#define FILE_IO_PAGE_SEGMENT_GAP 1024
#define DOS_FILENAME_LENGTH 13
#define FILE_FIND_PATH_SIZE 128
#define FILE_COMBINE_PATH_SIZE 80
#define RESOURCE_NAME_BUFFER_SIZE 80

#define RS_RLE_ESCLEN_MAX 16
#define RS_RLE_ESCLOOKUP_LEN 256
#define RS_RLE_ESCSEQ_POS 1

#define RS_VLE_ESC_LEN 16
#define RS_VLE_ALPH_LEN 256
#define RS_VLE_ESC_WIDTH 64
#define RS_VLE_NUM_SYMB 128

#define DOS_PARAGRAPH_SHIFT 4U
#define DOS_PARAGRAPH_MASK 15L
#define BYTE_HIGH_BIT 128U
#define BYTE_SHIFT LEGACY_BYTE_BITS
#define BYTE_BIT_COUNT LEGACY_BYTE_BITS
#define RLE_BYTE_COUNT_CODE 1
#define RLE_WORD_COUNT_CODE 3
#define RLE_SEQUENCE_TRAILER_SIZE 2
#define VLE_EXTENDED_CODE_START_WIDTH 7U
#define VLE_ALPHABET_BRANCH_FACTOR 2U
#define VLE_CODE_WORD_SECOND_BYTE_OFFSET 1U
#define COMPRESSION_WORKSPACE_PARAGRAPHS 4U
#define COMPRESSION_RLE_TYPE 1U
#define COMPRESSION_VLE_TYPE 2U
#define FILE_ERROR_DIALOG_ABORT 2

#define COMPR_HEADER_SIZE 4U
#define COMPR_SIZE_LOW_OFFSET 1U
#define COMPR_SIZE_HIGH_OFFSET 3U

#define COMPR_RLE_HEADER_SIZE 5U
#define COMPR_RLE_SIZE_HIGH_OFFSET 2U
#define COMPR_RLE_ESCLEN_OFFSET 4U

typedef legacy_u16 fileio_handle;

#define FILEIO_INVALID_HANDLE 0U
#define FILEIO_SEEK_END 2

static fileio_handle fileio_open(const legacy_s8* path, legacy_s16 create)
{
	return dos_file_open(path, create);
}

static legacy_s16 fileio_close(fileio_handle file)
{
	return dos_file_close(file);
}

static size_t fileio_read(void far* dst, size_t size, size_t nmemb,
	fileio_handle file)
{
	return dos_file_read(file, dst, (legacy_u16)(size * nmemb));
}

static size_t fileio_write(const void far* src, size_t size, size_t nmemb,
	fileio_handle file)
{
	return dos_file_write(file, src, (legacy_u16)(size * nmemb));
}

static legacy_s16 fileio_seek(fileio_handle file, legacy_s32 offset,
	legacy_s16 origin)
{
	return dos_file_seek(file, offset, origin);
}

static legacy_s32 fileio_tell(fileio_handle file)
{
	return dos_file_tell(file);
}

static legacy_s16 fileio_error(void)
{
	return dos_file_error();
}

static legacy_s16 fileio_remove(const legacy_s8* path)
{
	return dos_file_remove(path);
}

struct file_find_dos {
	legacy_s8 path[FILE_FIND_PATH_SIZE];
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
	memcpy(g_find.dirdelim, found_name, DOS_FILENAME_LENGTH);

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
	memcpy(g_find.dirdelim, found_name, DOS_FILENAME_LENGTH);

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
	legacy_s8 path[FILE_COMBINE_PATH_SIZE];

	file_build_path(dir, name, ext, path);

	return file_find(path);
}

// Round a byte count up to the number of whole 16-byte paragraphs holding it.
static legacy_u16 file_bytes_to_paras(legacy_s32 length)
{
	return (legacy_u16)((length >> DOS_PARAGRAPH_SHIFT) +
		(length & DOS_PARAGRAPH_MASK ? 1 : 0));
}

// Get number of 16-byte blocks needed to store entire file.
legacy_u16 file_paras(const legacy_s8* filename, legacy_s16 fatal)
{
	legacy_s32 length;
	fileio_handle file;
	if ((file = fileio_open(filename, 0)) != FILEIO_INVALID_HANDLE) {
		fileio_seek(file, 0, FILEIO_SEEK_END);
		length = fileio_tell(file);
		fileio_close(file);

		if (!fileio_error()) {
			// May overflow, but all Stunts files are rather small.
			return file_bytes_to_paras(length);
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
	fileio_handle file;
	legacy_u8 header[COMPR_HEADER_SIZE];
	if ((file = fileio_open(filename, 0)) != FILEIO_INVALID_HANDLE) {
		fileio_read(header, sizeof(header), 1, file);
		fileio_close(file);

		if (!fileio_error()) {
			// May overflow, but all Stunts files are rather small.
			length = (legacy_s32)LEGACY_READ_U16_LE(
				header + COMPR_SIZE_LOW_OFFSET) |
				((legacy_s32)header[COMPR_SIZE_HIGH_OFFSET] <<
					LEGACY_WORD_BITS);
			return file_bytes_to_paras(length);
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
	fileio_handle file;

	if ((file = fileio_open(filename, 0)) != FILEIO_INVALID_HANDLE) {
		// Read one page at a time.
		do {
			readlen = fileio_read(curdst, FILE_IO_PAGE_SIZE, 1, file);
			curdst = dos_memory_make_pointer(
				dos_memory_pointer_segment(curdst) +
					FILE_IO_PAGE_SEGMENT_GAP,
				dos_memory_pointer_offset(dst));
		} while (readlen == FILE_IO_PAGE_SIZE);

		fileio_close(file);

		if (!fileio_error()) {
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
	fileio_handle file;

	retval = 0;

	if ((file = fileio_open(filename, 1)) != FILEIO_INVALID_HANDLE) {
		// Write one page at a time.
		while (length != 0) {
			wrtlen = length > FILE_IO_PAGE_SIZE ? FILE_IO_PAGE_SIZE : length;

			if (fileio_write(src, wrtlen, 1, file) != wrtlen) {
				// Either DOS interrupt 33 function 64 set carry, or it wrote
				// fewer bytes than asked (`cmp ax, cx / jnz` -> `mov ax, 1`).
				retval = 1;
				break;
			}
			length -= wrtlen;
			src = dos_memory_make_pointer(
				dos_memory_pointer_segment(src) +
					FILE_IO_PAGE_SEGMENT_GAP,
				dos_memory_pointer_offset(src));
		}

		fileio_close(file);

		// The original ignores the close result. Clear the shim's sticky error
		// state without turning a close failure into a failed write.
		(void)fileio_error();

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
	fileio_remove(filename);

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
// The original opens with a bail-out this has no equivalent for. It is not
// reproduced. With a single escape code the caller's
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
				while (src < seqend - RLE_SEQUENCE_TRAILER_SIZE) {
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
				case RLE_BYTE_COUNT_CODE:
					rep = *src++;
					cur = *src++;

					while (rep--) {
						*dst++ = cur;
					}
					break;

				case RLE_WORD_COUNT_CODE:
					repw = *src++;
					repw |= *src++ << BYTE_SHIFT;
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

static void far* file_relocate_decomp_output(void far* destination,
	legacy_u16 decompparas, legacy_u32 length)
{
	legacy_u16 paras;
	void far* source;

	paras = file_bytes_to_paras((legacy_s32)length);
	source = dos_memory_make_pointer(
		decompparas - paras + dos_memory_pointer_segment(destination),
		dos_memory_pointer_offset(destination));
	copy_paras_reverse(dos_memory_pointer_segment(destination),
		dos_memory_pointer_segment(source), paras);
	return source;
}

// Decompress run-length encoded sub-file.
legacy_u32 file_decomp_rle(legacy_u8 huge* src, legacy_u8 huge* dst, legacy_u16 decompparas)
{
	legacy_u32 len, srclen, passlen;
	legacy_u16 skipseq, i;
	legacy_u8 esclookup[RS_RLE_ESCLOOKUP_LEN];
	legacy_u8 huge* origsrc;
	legacy_u8 huge* escapes;
	legacy_u8 esclen;

	(void)decompparas;

	// Get decompressed size from header.
	len = LEGACY_READ_U16_LE(src + COMPR_SIZE_LOW_OFFSET) |
		((legacy_u32)src[COMPR_SIZE_HIGH_OFFSET] <<
			LEGACY_WORD_BITS);
	origsrc = src += COMPR_HEADER_SIZE;

	// Get source size and escape codes.
	srclen = LEGACY_READ_U16_LE(src) |
		((legacy_u32)src[COMPR_RLE_SIZE_HIGH_OFFSET] <<
			LEGACY_WORD_BITS);
	esclen = src[COMPR_RLE_ESCLEN_OFFSET];
	escapes = src + COMPR_RLE_HEADER_SIZE;

	// MSB denotes skipping the initial pass for byte sequence runs. Match the
	// original's strict compare against 128: exactly 128 still runs the pass,
	// using the byte in the sequence-escape slot even though the declared
	// escape-code count is zero.
	skipseq = esclen > BYTE_HIGH_BIT;
	esclen &= (legacy_u8)~BYTE_HIGH_BIT;

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

		// In main decomp func:
		src = file_relocate_decomp_output(dst, decompparas, passlen);
	}

	// The original discards the single pass's count and returns the size out
	// of the compression header instead. The two only differ when the last
	// run overshoots - file_decomp_rle_single
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
		((legacy_u32)src[COMPR_SIZE_HIGH_OFFSET] <<
			LEGACY_WORD_BITS);
	src += COMPR_HEADER_SIZE;

	// One-byte escape codes length counter.
	esclen = *src++;
	additive = (esclen & BYTE_HIGH_BIT) == BYTE_HIGH_BIT;
	esclen &= (legacy_u8)~BYTE_HIGH_BIT;

	// Store postion of width data for later.
	wdtpos = src;

	// Generate escape codes.
	for (i = 0, j = 0, alphlen = 0;
		i < esclen; ++i, j *= VLE_ALPHABET_BRANCH_FACTOR) {
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
	widthdistr = (esclen >= BYTE_BIT_COUNT ? BYTE_BIT_COUNT : esclen);
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

	curword = *src << BYTE_SHIFT |
		*(src + VLE_CODE_WORD_SECOND_BYTE_OFFSET);
	src += LEGACY_WORD_BYTES;
	curwdt = BYTE_BIT_COUNT;
	cursymb = 0;

	++lenleft;
	while (lenleft) {
		code = curword >> BYTE_SHIFT;
		nextwdt = wdth[code];
		// Expand.
		if (nextwdt > BYTE_BIT_COUNT) {
			code = curword;
			curword >>= BYTE_SHIFT;

			i = VLE_EXTENDED_CODE_START_WIDTH;
			while (1) {
				if (!curwdt) {
					code = *src++;
					curwdt = BYTE_BIT_COUNT;
				}

				curword = (curword << 1) +
					((code & BYTE_HIGH_BIT) == BYTE_HIGH_BIT);
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
			nextwdt = BYTE_BIT_COUNT - curwdt;
			curwdt = BYTE_BIT_COUNT;
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
				curwdt = BYTE_BIT_COUNT;
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
		decompparas += COMPRESSION_WORKSPACE_PARAGRAPHS;
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
				if (passes & BYTE_HIGH_BIT) {
					passes &= (legacy_u8)~BYTE_HIGH_BIT;
					src += COMPR_HEADER_SIZE;
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
						case COMPRESSION_RLE_TYPE:
							passlen = file_decomp_rle(src, dst, decompparas);
							break;
						case COMPRESSION_VLE_TYPE:
							passlen = file_decomp_vle(src, dst, decompparas);
							break;
						default:
							err = 1;
					}

					// Set source for next pass.
					if (!err && (--passes != 0)) {
						src = file_relocate_decomp_output(
							dst, decompparas, passlen);
					}
				}

				// Free unneeded overhead.
				if (!err) {
					decompparas -= COMPRESSION_WORKSPACE_PARAGRAPHS;
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

#ifndef RESTUNTS_HEADLESS
// One attempt at loading a resource of the given type; 0 means it failed.
static void far* file_try_load_resource(legacy_s16 resource_type,
	const legacy_s8* filename)
{
	switch (resource_type) {
		case FILE_RESOURCE_BINARY_FATAL:
		case FILE_RESOURCE_BINARY_OPTIONAL:
			return file_load_binary_nofatal(filename);

		case FILE_RESOURCE_SHAPE2D:
			return file_load_shape2d_nofatal(filename);

		case FILE_RESOURCE_SHAPE2D_COLLECTION:
			return file_load_shape2d_res_nofatal(filename);

		case FILE_RESOURCE_SONG:
			return load_song_file(filename);

		case FILE_RESOURCE_VOICE:
			return load_voice_file(filename);

		case FILE_RESOURCE_SOUND_EFFECTS:
			return load_sfx_file(filename);

		case FILE_RESOURCE_COMPRESSED_OPTIONAL:
			return file_decomp_nofatal(filename);

		case FILE_RESOURCE_SHAPE2D_ALTERNATE:
			return file_load_shape2d_nofatal2(filename);
	}

	return 0;
}
#endif

void far* file_load_resource(legacy_s16 resource_type,
	const legacy_s8* filename) {
	void far* result;
#ifdef RESTUNTS_HEADLESS
	if (resource_type == FILE_RESOURCE_BINARY_FATAL ||
		resource_type == FILE_RESOURCE_BINARY_OPTIONAL)
		result = file_load_binary_nofatal(filename);
	else if (resource_type == FILE_RESOURCE_COMPRESSED_OPTIONAL)
		result = file_decomp_nofatal(filename);
	else
		result = 0;
	if (result == 0 && resource_type == FILE_RESOURCE_BINARY_FATAL)
		fatal_error(headless_file_error, filename);
	return result;
#else
	legacy_s16 dearesult;
	while (1) {
		result = file_try_load_resource(resource_type, filename);
		// Optional resource types report failure to the caller; every other
		// type shows a dialog and retries until the dialog gives up.
		if (result != 0 ||
			resource_type == FILE_RESOURCE_BINARY_OPTIONAL ||
			resource_type == FILE_RESOURCE_COMPRESSED_OPTIONAL)
			return result;

		dearesult = do_dea_textres();
		if (dearesult == FILE_ERROR_DIALOG_ABORT)
			return 0;
	}
#endif
}

static void far* file_load_suffixed_resource(legacy_s16 resource_type,
	const legacy_s8* filename, const legacy_s8* suffix, legacy_s8* name)
{
	strcpy(name, filename);
	strcat(name, suffix);
	return file_load_resource(resource_type, name);
}


void far* file_load_resfile(const legacy_s8* filename) {
	legacy_s8 name[RESOURCE_NAME_BUFFER_SIZE];
	void far* result;

#ifdef RESTUNTS_HEADLESS
	result = file_load_suffixed_resource(FILE_RESOURCE_BINARY_OPTIONAL,
		filename, ".res", name);
	if (result != 0)
		return result;
	result = file_load_suffixed_resource(FILE_RESOURCE_COMPRESSED_OPTIONAL,
		filename, ".pre", name);
	if (result == 0)
		fatal_error(headless_file_error, filename);
	return result;
#else
	while (1) {
		result = file_load_suffixed_resource(FILE_RESOURCE_BINARY_OPTIONAL,
			filename, ".res", name);
		if (result != 0) return result;

		result = file_load_suffixed_resource(FILE_RESOURCE_COMPRESSED_OPTIONAL,
			filename, ".pre", name);
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
	legacy_s8 name[RESOURCE_NAME_BUFFER_SIZE];
	void far* result;

	while (1) {
		result = file_load_suffixed_resource(FILE_RESOURCE_COMPRESSED_OPTIONAL,
			filename, ".p3s", name);
		if (result != 0) return result;

		result = file_load_suffixed_resource(FILE_RESOURCE_BINARY_OPTIONAL,
			filename, ".3sh", name);
		if (result != 0) return result;

		do_dea_textres();
	}
}

void file_load_audiores(const legacy_s8* songfile, const legacy_s8* voicefile, const legacy_s8* name) {
	void far* audiores;
	voicefileptr = file_load_resource(FILE_RESOURCE_VOICE, voicefile);
	songfileptr = file_load_resource(FILE_RESOURCE_SONG, songfile);
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
		replay_file_size(gameconfig.game_recordedframes));
	g_is_busy = 0;

	return ret;
}
