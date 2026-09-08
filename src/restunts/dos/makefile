include ../watcom.mk

RESTUNTS_OBJ_FILES = fileio.obj legacy.obj resource.obj math.obj memmgr.obj strlib.obj numlib.obj kevinrnd.obj replinit.obj gameinit.obj gamestep.obj simd.obj carsetup.obj trkutil.obj trackres.obj restunts.obj fatal.obj timing.obj game_input.obj ui_text.obj ui_input.obj ui_dialog.obj intro.obj highscore.obj replay_record.obj replay_viewer.obj race.obj race_resources.obj dashboard.obj menu_common.obj menu_main.obj menu_options.obj menu_track.obj menu_track_editor.obj menu_opponent.obj menu_car.obj audio_engine.obj audio_resources.obj audio_car.obj audio_sequence.obj \
                     shape3d.obj shape3d_resources.obj shape3d_car.obj shape3d_prerender.obj shape3d_lines.obj shape2d.obj shape2d_blit.obj shape2d_resources.obj heapsort.obj \
                     frame.obj skybox.obj frame_overlay.obj intro_render.obj track_preview.obj state.obj physics_grip.obj opponent.obj track_setup.obj stateply.obj physics_collision.obj statecrs.obj \
                     statecar.obj trackobj.obj replay.obj

ASMORIG_OBJ_FILES = segments.obj seg000.obj seg001.obj seg002.obj seg003.obj seg004.obj seg005.obj \
                    seg006.obj seg007.obj seg008.obj seg009.obj seg010.obj seg011.obj seg012.obj \
                    seg013.obj seg014.obj seg015.obj seg016.obj seg017.obj seg018.obj seg019.obj \
                    seg020.obj seg021.obj seg022.obj seg023.obj seg024.obj seg025.obj seg026.obj \
                    seg027.obj seg028.obj seg029.obj seg030.obj seg031.obj seg032.obj seg033.obj \
                    seg034.obj seg035.obj seg036.obj seg037.obj seg038.obj seg039.obj seg041.obj dseg.obj

TARGET ?= full
ifeq (,$(filter $(TARGET),full original))
  $(error Unsupported game target: $(TARGET))
endif
OBJDIR = build/watcom/$(CONFIG)/$(TARGET)
ifeq ($(TARGET),original)
  OUTFILE = restunto.exe
  DESTFILE = RESTUNTO.EXE
  ASMORIG_DIR = ../asmorig/build/watcom/$(CONFIG)/$(ASSEMBLER)
  # The empty private segments in segments.obj order TLINK but confuse WLINK.
  # Keep all original ASM unchanged and express its layout in the response file.
  OBJ_FILES = $(addprefix $(ASMORIG_DIR)/,$(filter-out segments.obj,$(ASMORIG_OBJ_FILES))) $(OBJDIR)/main.obj
  ORIGINAL_CODE_OBJECTS = $(filter-out segments.obj seg038.obj seg039.obj seg041.obj dseg.obj,$(ASMORIG_OBJ_FILES))
  LINK_ORDER = order clname STUNTSC $(foreach obj,$(ORIGINAL_CODE_OBJECTS),segment $(basename $(obj))) \
               clname STUNTSD segment seg038 segment dseg segment seg039 \
               clname STACK segment STACK segment seg041 clname DATA clname BSS clname CODE clname ENDSEG
  CFLAGS = -dRESTUNTS_ORIGINAL
  # Preserve original far-call instructions even when WASM marks them optimizable.
  START_OPTION = option NOFARCALLS option NOCASEEXACT option PACKCODE=0 option PACKDATA=0 option STACK=8000
else
  OUTFILE = restunts.exe
  DESTFILE = RESTUNTS.EXE
  RESTUNTS_OBJ_FILES += dosstart.obj headless_data.obj headless_trackdata.obj full_data.obj full_strings.obj full_tables.obj
  OBJ_FILES = $(PLATFORM_ASM_DIR)/dosbss.obj $(addprefix $(C_DIR)/,$(RESTUNTS_OBJ_FILES)) $(PLATFORM_OBJECTS) $(PLATFORM_ASM_DIR)/dosstack.obj
  START_OPTION = option START=_headless_start
endif
C_DIR = ../c/build/watcom/$(CONFIG)/dosfull
PLATFORM_NAMES = timer runtime memory file keyboard divide joystick mouse video audio critical
PLATFORM_DIR = ../platform/dos/build/watcom/$(CONFIG)
PLATFORM_ASM_DIR = $(PLATFORM_DIR)/$(ASSEMBLER)
PLATFORM_OBJECTS = $(addprefix $(PLATFORM_DIR)/,$(addsuffix .obj,$(PLATFORM_NAMES)))
SPACE := $(empty) $(empty)
COMMA := ,

.PHONY: all clean
all: $(OBJDIR) $(OBJ_FILES)
	$(file >$(OBJDIR)/link.lnk,name $(OUTFILE))
	$(file >>$(OBJDIR)/link.lnk,format dos $(WATCOM_LFLAGS) $(START_OPTION) option MAP=$(basename $(OUTFILE)).map)
	$(file >>$(OBJDIR)/link.lnk,$(LINK_ORDER))
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
