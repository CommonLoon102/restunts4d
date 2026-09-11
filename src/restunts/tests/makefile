include ../watcom.mk

OBJDIR = build/watcom/$(CONFIG)
OUTFILE = $(OBJDIR)/DOSPLAT.EXE
PLATFORM_DIR = ../platform/dos/build/watcom/$(CONFIG)
PLATFORM_ASM_DIR = $(PLATFORM_DIR)/wasm
PLATFORM_NAMES = runtime memory file
PLATFORM_OBJECTS = $(PLATFORM_ASM_DIR)/dosbss.obj $(addprefix $(PLATFORM_DIR)/,$(addsuffix .obj,$(PLATFORM_NAMES))) $(PLATFORM_ASM_DIR)/dosstack.obj
OBJ_FILES = $(PLATFORM_ASM_DIR)/dosbss.obj $(OBJDIR)/dosstart.obj $(OBJDIR)/test-dos-platform.obj $(OBJDIR)/resource.obj $(filter-out $(PLATFORM_ASM_DIR)/dosbss.obj,$(PLATFORM_OBJECTS))
C_HEADERS = $(wildcard ../c/*.h) $(wildcard ../platform/dos/*.h)
SPACE := $(empty) $(empty)
COMMA := ,

.PHONY: all platform clean force-link
all: $(OUTFILE)

platform:
	$(MAKE) -C ../platform/dos CONFIG=$(CONFIG) LINKER=$(LINKER)

$(OBJDIR):
	$(call MKDIR,$@)

$(PLATFORM_OBJECTS): | platform

$(OBJDIR)/test-dos-platform.obj: test-dos-platform.c $(C_HEADERS) makefile ../watcom.mk $(WATCOM_STAMP) | $(OBJDIR)
	$(CC) $(WATCOM_PLATFORM_CFLAGS) -i=../c -fo=$@ $<

$(OBJDIR)/dosstart.obj: ../platform/dos/dosstart.c $(C_HEADERS) makefile ../watcom.mk $(WATCOM_STAMP) | $(OBJDIR)
	$(CC) $(WATCOM_PLATFORM_CFLAGS) -dRESTUNTS_HEADLESS -i=../c -fo=$@ $<

$(OBJDIR)/resource.obj: ../c/resource.c $(C_HEADERS) makefile ../watcom.mk $(WATCOM_STAMP) | $(OBJDIR)
	$(CC) $(WATCOM_CFLAGS) -i=../c -fo=$@ $<

# Relink after the platform submake refreshes its objects.
force-link:

$(OUTFILE): $(OBJ_FILES) makefile ../watcom.mk force-link | platform
	$(file >$(OBJDIR)/link.lnk,name $(OUTFILE))
	$(file >>$(OBJDIR)/link.lnk,format dos $(WATCOM_LFLAGS) option START=_headless_start option MAP=$(OBJDIR)/dosplat.map)
	$(file >>$(OBJDIR)/link.lnk,$(WATCOM_LIBPATH))
	$(file >>$(OBJDIR)/link.lnk,file $(subst $(SPACE),$(COMMA),$(OBJ_FILES)))
	$(WLINK) @$(OBJDIR)/link.lnk

clean:
	$(call RMDIR,build/watcom)
