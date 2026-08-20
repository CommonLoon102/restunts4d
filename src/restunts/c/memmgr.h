#ifndef RESTUNTS_MEMMGR_H
#define RESTUNTS_MEMMGR_H

#include "legacy.h"

#ifdef RESTUNTS_SDL
#define far
#endif

#pragma pack (push, 1)
struct MEMCHUNK {
	char resname[12];
	legacy_u16 ressize;
	legacy_u16 resofs;
	legacy_u16 resunk;
};

typedef char legacy_memchunk_must_be_18_bytes[
	(sizeof(struct MEMCHUNK) == 18) ? 1 : -1
];
#pragma pack (pop)

const char* mmgr_path_to_name(const char* filename);
void far* mmgr_alloc_pages(const char* arg_0, unsigned short arg_2);
void mmgr_alloc_resmem(unsigned short arg_0);
void mmgr_alloc_a000(void);
unsigned short mmgr_get_ofs_diff(void);
legacy_u32 mmgr_prepare_fullscreen_window(void);
void far* mmgr_alloc_window_pages(const char* name, unsigned short pages);
void mmgr_release_window(void far* ptr);
void far* mmgr_alloc_shape2d_pages(const char* name, unsigned short pages);
void far* mmgr_finish_shape2d_pages(void far* ptr);
void far* mmgr_free(char far* ptr);
void mmgr_copy_paras(unsigned short srcseg, unsigned short destseg, short paras);
void copy_paras_reverse(unsigned short srcseg, unsigned short destseg, short paras);
void mmgr_find_free(void);
void far* mmgr_get_chunk_by_name(const char* arg_0);
void mmgr_release(char far* ptr);
unsigned short mmgr_get_chunk_size(char far* ptr);
unsigned short mmgr_resize_memory(unsigned short arg_0, unsigned short arg_2, unsigned short arg_4);
void far* mmgr_op_unk(char far* ptr);
void far* mmgr_alloc_resbytes(const char* name, legacy_s32 size);
legacy_u32 mmgr_get_res_ofs_diff_scaled(void);
legacy_u32 mmgr_get_chunk_size_bytes(char far* ptr);

char far* locate_resource(char far* data, char* name, unsigned short fatal);
char far* locate_shape_nofatal(char far* data, char* name);
char far* locate_shape_fatal(char far* data, char* name);
char far* locate_shape_alt(char far* data, char* name);
char far* locate_sound_fatal(char far* data, char* name);
void locate_many_resources(char far* data, char* names, char far** result);
char far* locate_text_res(char far* data, char* name);

#endif
