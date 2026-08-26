#ifndef RESTUNTS_MEMMGR_H
#define RESTUNTS_MEMMGR_H

#ifdef RESTUNTS_SDL
#define far
#endif

#pragma pack (push, 1)
struct MEMCHUNK {
	char resname[12];
	unsigned ressize;
	unsigned resofs;
	unsigned resunk;
};
#pragma pack (pop)

const char* mmgr_path_to_name(const char* filename);
void far* mmgr_alloc_pages(const char* arg_0, unsigned short arg_2);
void mmgr_alloc_resmem(unsigned short arg_0);
void mmgr_alloc_a000(void);
unsigned short mmgr_get_ofs_diff();
void far* mmgr_free(char far* ptr);
void mmgr_copy_paras(unsigned short srcseg, unsigned short destseg, short paras);
void copy_paras_reverse(unsigned short srcseg, unsigned short destseg, short paras);
void mmgr_find_free();
void far* mmgr_get_chunk_by_name(const char* arg_0);
void mmgr_release(char far* ptr);
unsigned short mmgr_get_chunk_size(char far* ptr);
unsigned short mmgr_resize_memory(unsigned short arg_0, unsigned short arg_2, unsigned short arg_4);
void far* mmgr_op_unk(char far* ptr);
void far* mmgr_alloc_resbytes(const char* name, long int size);
unsigned long mmgr_get_res_ofs_diff_scaled(void);
unsigned long mmgr_get_chunk_size_bytes(char far* ptr);

void himem_init(void);
void ems_shutdown(void);
void highpool_add_block(unsigned short seg, unsigned short paras, unsigned short largeonly);
int highpool_owns_seg(unsigned short seg);
int highpool_route(const char* name, unsigned short paras);
int highpool_can_fit(unsigned short paras);
void far* highpool_alloc(const char* name, unsigned short paras);
void far* highpool_get_by_name(const char* name);
void mmgr_rename_chunk(char far* ptr, const char* name);

char far* locate_resource(char far* data, char* name, unsigned short fatal);
char far* locate_shape_nofatal(char far* data, char* name);
char far* locate_shape_fatal(char far* data, char* name);
char far* locate_shape_alt(char far* data, char* name);
char far* locate_sound_fatal(char far* data, char* name);
void locate_many_resources(char far* data, char* names, char far** result);
char far* locate_text_res(char far* data, char* name);

#endif
