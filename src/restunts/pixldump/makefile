include ../watcom.mk

RESTUNTS_OBJ_FILES = fileio.obj legacy.obj resource.obj math.obj memmgr.obj strlib.obj numlib.obj \
                     kevinrnd.obj replinit.obj gameinit.obj gamestep.obj simd.obj carsetup.obj trkutil.obj trackres.obj restunts.obj fatal.obj timing.obj game_input.obj ui_text.obj ui_input.obj ui_dialog.obj intro.obj highscore.obj replay_record.obj replay_viewer.obj race.obj race_resources.obj dashboard.obj menu_common.obj menu_main.obj menu_options.obj menu_track.obj menu_track_editor.obj menu_opponent.obj menu_car.obj audio_engine.obj audio_resources.obj audio_car.obj audio_sequence.obj \
                     shape3d.obj shape3d_resources.obj shape3d_car.obj shape3d_prerender.obj shape3d_lines.obj shape2d.obj shape2d_blit.obj shape2d_resources.obj heapsort.obj frame.obj skybox.obj frame_overlay.obj intro_render.obj track_preview.obj state.obj physics_grip.obj opponent.obj track_setup.obj stateply.obj physics_collision.obj statecrs.obj statecar.obj trackobj.obj replay.obj \
                     headless_data.obj headless_trackdata.obj full_data.obj full_strings.obj full_tables.obj

OBJDIR = build/watcom/$(CONFIG)
OUTFILE = pixldump.exe
DESTFILE = PIXLDUMP.EXE
C_DIR = ../c/build/watcom/$(CONFIG)/dosfull
PLATFORM_NAMES = timer runtime memory file keyboard divide joystick mouse video audio critical
OBJ_FILES = $(PLATFORM_DIR)/dosbss.obj $(addprefix $(C_DIR)/,$(RESTUNTS_OBJ_FILES)) $(OBJDIR)/pixldump.obj $(OBJDIR)/md5.obj $(OBJDIR)/pixlstart.obj $(PLATFORM_OBJECTS) $(PLATFORM_DIR)/dosstack.obj
START_OPTION = option START=_headless_start
CFLAGS = -dRESTUNTS_PIXLDUMP
PLATFORM_DIR = ../platform/dos/build/watcom/$(CONFIG)
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

$(OBJDIR)/pixlstart.obj: pixlstart.c ../platform/dos/dosstart.c $(wildcard ../platform/dos/*.h) $(wildcard ../c/*.h) makefile ../watcom.mk $(WATCOM_STAMP) | $(OBJDIR)
	$(CC) $(WATCOM_PLATFORM_CFLAGS) $(CFLAGS) -i=../c -fo=$@ $<


clean:
	$(call RMDIR,build/watcom)
	$(call REMOVE,*.exe)
	$(call REMOVE,*.map)
