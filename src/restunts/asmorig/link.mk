# Original DOS image layout shared by the game and original-assembly dump tools.
# Callers live one directory below src/restunts and include ../watcom.mk first.
ASMORIG_OBJ_FILES = segments.obj seg000.obj seg001.obj seg002.obj seg003.obj seg004.obj seg005.obj \
                    seg006.obj seg007.obj seg008.obj seg009.obj seg010.obj seg011.obj seg012.obj \
                    seg013.obj seg014.obj seg015.obj seg016.obj seg017.obj seg018.obj seg019.obj \
                    seg020.obj seg021.obj seg022.obj seg023.obj seg024.obj seg025.obj seg026.obj \
                    seg027.obj seg028.obj seg029.obj seg030.obj seg031.obj seg032.obj seg033.obj \
                    seg034.obj seg035.obj seg036.obj seg037.obj seg038.obj seg039.obj seg041.obj dseg.obj

ASMORIG_DIR = ../asmorig/build/watcom/$(CONFIG)/wasm
# Empty private segments confuse WLINK; explicit ordering below preserves
# the original layout while keeping the original sources unchanged.
ASMORIG_LINK_OBJECTS = $(addprefix $(ASMORIG_DIR)/,$(filter-out segments.obj,$(ASMORIG_OBJ_FILES)))
ORIGINAL_CODE_OBJECTS = $(filter-out segments.obj seg038.obj seg039.obj seg041.obj dseg.obj,$(ASMORIG_OBJ_FILES))
ORIGINAL_LINK_ORDER = order clname STUNTSC $(foreach obj,$(ORIGINAL_CODE_OBJECTS),segment $(basename $(obj))) \
               clname STUNTSD segment seg038 segment dseg segment seg039 \
               clname STACK segment STACK segment seg041 clname DATA clname BSS clname CODE clname ENDSEG
# Retain the original far calls, unpacked segments, and 8000-byte stack.
ORIGINAL_LINK_OPTIONS = option NOFARCALLS option NOCASEEXACT option PACKCODE=0 option PACKDATA=0 option STACK=8000
