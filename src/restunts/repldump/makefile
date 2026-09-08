include ../watcom.mk

RESTUNTS_OBJ_FILES = fileio.obj legacy.obj resource.obj math.obj memmgr.obj strlib.obj numlib.obj \
                     kevinrnd.obj replinit.obj gameinit.obj gamestep.obj \
                     simd.obj carsetup.obj trkutil.obj trackres.obj headless.obj dosstart.obj headless_data.obj headless_trackdata.obj state.obj physics_grip.obj opponent.obj track_setup.obj stateply.obj physics_collision.obj \
                     statecrs.obj statecar.obj trackobj.obj replay.obj stateio.obj

OBJDIR = build/watcom/$(CONFIG)
OUTFILE = repldump.exe
DESTFILE = REPLDUMP.EXE
C_DIR = ../c/build/watcom/$(CONFIG)/dosheadless
PLATFORM_NAMES = runtime memory file divide
OBJ_FILES = $(PLATFORM_ASM_DIR)/dosbss.obj $(addprefix $(C_DIR)/,$(RESTUNTS_OBJ_FILES)) $(OBJDIR)/repldump.obj $(PLATFORM_OBJECTS) $(PLATFORM_ASM_DIR)/dosstack.obj
START_OPTION = option START=_headless_start
CFLAGS = -dRESTUNTS_HEADLESS
PLATFORM_DIR = ../platform/dos/build/watcom/$(CONFIG)
PLATFORM_ASM_DIR = $(PLATFORM_DIR)/$(ASSEMBLER)
PLATFORM_OBJECTS = $(addprefix $(PLATFORM_DIR)/,$(addsuffix .obj,$(PLATFORM_NAMES)))
SPACE := $(empty) $(empty)
COMMA := ,

.PHONY: all clean
all: $(OBJDIR) $(OBJ_FILES)
	$(file >$(OBJDIR)/link.lnk,name $(OUTFILE))
	$(file >>$(OBJDIR)/link.lnk,format dos $(WATCOM_LFLAGS) $(START_OPTION) option MAP=$(basename $(OUTFILE)).map)
	$(file >>$(OBJDIR)/link.lnk,$(WATCOM_LIBPATH))
	$(file >>$(OBJDIR)/link.lnk,file $(subst $(SPACE),$(COMMA),$(OBJ_FILES)))
	$(WLINK) @$(OBJDIR)/link.lnk
	$(call COPY,$(OUTFILE),../../../stunts/$(DESTFILE))

$(OBJDIR):
	$(call MKDIR,$@)

$(OBJDIR)/%.obj: %.c $(wildcard ../c/*.h) $(wildcard *.h) makefile ../watcom.mk $(WATCOM_STAMP) | $(OBJDIR)
	$(CC) $(WATCOM_CFLAGS) $(CFLAGS) -i=../c -fo=$@ $<

clean:
	$(call RMDIR,build/watcom)
	$(call REMOVE,*.exe)
	$(call REMOVE,*.map)
