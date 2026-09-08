include ../watcom.mk

OBJDIR = build/watcom/$(CONFIG)
OUTFILE = $(OBJDIR)/DOSPLAT.EXE
PLATFORM_DIR = ../platform/dos/build/watcom/$(CONFIG)
PLATFORM_NAMES = dosbss runtime memory file dosstack
PLATFORM_OBJECTS = $(addprefix $(PLATFORM_DIR)/,$(addsuffix .obj,$(PLATFORM_NAMES)))
OBJ_FILES = $(PLATFORM_DIR)/dosbss.obj $(OBJDIR)/dosstart.obj $(OBJDIR)/test-dos-platform.obj $(OBJDIR)/resource.obj $(filter-out $(PLATFORM_DIR)/dosbss.obj,$(PLATFORM_OBJECTS))
C_HEADERS = $(wildcard ../c/*.h) $(wildcard ../platform/dos/*.h)
SPACE := $(empty) $(empty)
COMMA := ,

.PHONY: all platform clean
all: $(OUTFILE)

platform:
	$(MAKE) -C ../platform/dos CONFIG=$(CONFIG) ASSEMBLER=$(ASSEMBLER) LINKER=$(LINKER)

$(OBJDIR):
	$(call MKDIR,$@)

$(PLATFORM_OBJECTS): | platform

$(OBJDIR)/test-dos-platform.obj: test-dos-platform.c $(C_HEADERS) makefile ../watcom.mk $(WATCOM_STAMP) | $(OBJDIR)
	$(CC) $(WATCOM_PLATFORM_CFLAGS) -i=../c -fo=$@ $<

$(OBJDIR)/dosstart.obj: ../platform/dos/dosstart.c $(C_HEADERS) makefile ../watcom.mk $(WATCOM_STAMP) | $(OBJDIR)
	$(CC) $(WATCOM_PLATFORM_CFLAGS) -dRESTUNTS_HEADLESS -i=../c -fo=$@ $<

$(OBJDIR)/resource.obj: ../c/resource.c $(C_HEADERS) makefile ../watcom.mk $(WATCOM_STAMP) | $(OBJDIR)
	$(CC) $(WATCOM_CFLAGS) -i=../c -fo=$@ $<

$(OUTFILE): $(OBJ_FILES) makefile ../watcom.mk | platform
	$(file >$(OBJDIR)/link.lnk,name $(OUTFILE))
	$(file >>$(OBJDIR)/link.lnk,format dos $(WATCOM_LFLAGS) option START=_headless_start option MAP=$(OBJDIR)/dosplat.map)
	$(file >>$(OBJDIR)/link.lnk,$(WATCOM_LIBPATH))
	$(file >>$(OBJDIR)/link.lnk,file $(subst $(SPACE),$(COMMA),$(OBJ_FILES)))
	$(WLINK) @$(OBJDIR)/link.lnk

clean:
	$(call RMDIR,build/watcom)
