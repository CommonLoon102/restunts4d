#ifndef RESTUNTS_FILEIO_H
#define RESTUNTS_FILEIO_H

#include "legacy.h"

#ifdef __cplusplus
extern "C" {
#endif

const legacy_s8* file_find(const legacy_s8* query);
const legacy_s8* file_find_next(void);
const legacy_s8* file_find_next_alt(void);

void file_build_path(const legacy_s8* dir, const legacy_s8* name, const legacy_s8* ext, legacy_s8* dst);
const legacy_s8* file_combine_and_find(const legacy_s8* dir, const legacy_s8* name, const legacy_s8* ext);

legacy_u16 file_paras(const legacy_s8* filename, legacy_s16 fatal);
legacy_u16 file_paras_fatal(const legacy_s8* filename);
legacy_u16 file_paras_nofatal(const legacy_s8* filename);

legacy_u16 file_decomp_paras(const legacy_s8* filename, legacy_s16 fatal);
legacy_u16 file_decomp_paras_fatal(const legacy_s8* filename);
legacy_u16 file_decomp_paras_nofatal(const legacy_s8* filename);

void far* file_read(const legacy_s8* filename, void far* dst, legacy_s16 fatal);
void far* file_read_fatal(const legacy_s8* filename, void far* dst);
void far* file_read_nofatal(const legacy_s8* filename, void far* dst);

legacy_s16 file_write(const legacy_s8* filename, void far* src, legacy_u32 length, legacy_s16 fatal);
legacy_s16 file_write_fatal(const legacy_s8* filename, void far* src, legacy_u32 length);
legacy_s16 file_write_nofatal(const legacy_s8* filename, void far* src, legacy_u32 length);

void far* file_decomp(const legacy_s8* filename, legacy_s16 fatal);
void far* file_decomp_fatal(const legacy_s8* filename);
void far* file_decomp_nofatal(const legacy_s8* filename);

void far* file_load_binary(const legacy_s8* filename, legacy_s16 fatal);
void far* file_load_binary_nofatal(const legacy_s8* filename);
void far* file_load_binary_fatal(const legacy_s8* filename);

void far* file_load_resfile(const legacy_s8* filename);
void far* file_load_resource(legacy_s16 type, const legacy_s8* filename);
void unload_resource(void far* resptr);
void file_load_audiores(const legacy_s8* songfile, const legacy_s8* voicefile, const legacy_s8* name);
void far* file_load_3dres(const legacy_s8* filename);

legacy_s16 file_load_replay(const legacy_s8* dir, const legacy_s8* name);
legacy_s16 file_write_replay(const legacy_s8* filename);

#ifdef __cplusplus
}
#endif

#endif
