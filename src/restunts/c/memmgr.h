#ifndef RESTUNTS_MEMMGR_H
#define RESTUNTS_MEMMGR_H

#include "legacy.h"

#ifdef RESTUNTS_SDL
#define far
#endif

#pragma pack (push, 1)
struct MEMCHUNK {
	legacy_s8 resname[12];
	legacy_u16 ressize;
	legacy_u16 resofs;
	legacy_u16 resunk;
};
#pragma pack (pop)

typedef char legacy_memchunk_must_be_18_bytes[
	(sizeof(struct MEMCHUNK) == 18) ? 1 : -1];

const legacy_s8* mmgr_path_to_name(const legacy_s8* filename);
void far* mmgr_alloc_pages(const legacy_s8* arg_0, legacy_u16 arg_2);
void mmgr_alloc_resmem(legacy_u16 arg_0);
void mmgr_alloc_a000(void);
legacy_u16 mmgr_get_ofs_diff(void);
void far* mmgr_free(legacy_s8 far* ptr);
void mmgr_copy_paras(legacy_u16 srcseg, legacy_u16 destseg, legacy_s16 paras);
void copy_paras_reverse(legacy_u16 srcseg, legacy_u16 destseg, legacy_s16 paras);
void mmgr_find_free();
void far* mmgr_get_chunk_by_name(const legacy_s8* arg_0);
void mmgr_release(void far* ptr);
legacy_u16 mmgr_get_chunk_size(legacy_s8 far* ptr);
legacy_u16 mmgr_resize_memory(legacy_u16 arg_0, legacy_u16 arg_2, legacy_u16 arg_4);
void far* mmgr_op_unk(legacy_s8 far* ptr);
void far* mmgr_alloc_resbytes(const legacy_s8* name, legacy_s32 size);
legacy_u32 mmgr_get_res_ofs_diff_scaled(void);
legacy_u32 mmgr_get_chunk_size_bytes(legacy_s8 far* ptr);

void himem_init(void);
void ems_shutdown(void);
void highpool_add_block(legacy_u16 seg, legacy_u16 paras, legacy_u16 largeonly);
void highpool_reserve_window(void);
legacy_s16 highpool_owns_seg(legacy_u16 seg);
legacy_s16 highpool_route(const legacy_s8* name, legacy_u16 paras);
legacy_s16 highpool_can_fit(legacy_u16 paras);
void far* highpool_alloc(const legacy_s8* name, legacy_u16 paras);
void far* highpool_get_by_name(const legacy_s8* name);
void mmgr_rename_chunk(legacy_s8 far* ptr, const legacy_s8* name);

legacy_s8 far* locate_resource(legacy_s8 far* data,
	const legacy_s8* name, legacy_u16 fatal);
legacy_s8 far* locate_shape_nofatal(legacy_s8 far* data,
	const legacy_s8* name);
legacy_s8 far* locate_shape_fatal(legacy_s8 far* data,
	const legacy_s8* name);
legacy_s8 far* locate_shape_alt(legacy_s8 far* data,
	const legacy_s8* name);
legacy_s8 far* locate_sound_fatal(legacy_s8 far* data,
	const legacy_s8* name);
void locate_many_resources(legacy_s8 far* data, const legacy_s8* names,
	legacy_s8 far** result);
legacy_s8 far* locate_text_res(legacy_s8 far* data,
	const legacy_s8* name);

#endif
