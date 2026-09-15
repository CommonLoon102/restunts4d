include ../watcom.mk
TARGET ?= dosfull
OBJDIR = build/watcom/$(CONFIG)/$(TARGET)
ifeq ($(TARGET),dosheadless)
  CFLAGS = -dRESTUNTS_HEADLESS
else ifeq ($(TARGET),dosfull)
  CFLAGS = -dRESTUNTS_FULL
else
  $(error Unsupported C target: $(TARGET))
endif

C_FILES = fileio.c legacy.c resource.c math.c memmgr.c strlib.c numlib.c kevinrnd.c replinit.c gameinit.c gamestep.c simd.c carsetup.c trkutil.c trackres.c restunts.c fatal.c timing.c game_input.c ui_text.c ui_input.c ui_dialog.c intro.c highscore.c replay_record.c replay_viewer.c race.c race_resources.c dashboard.c menu_common.c menu_main.c menu_options.c menu_track.c menu_track_editor.c menu_opponent.c menu_car.c audio_engine.c audio_resources.c audio_car.c audio_sequence.c shape3d.c shape3d_resources.c shape3d_car.c shape3d_prerender.c shape3d_lines.c shape2d.c shape2d_blit.c shape2d_resources.c heapsort.c \
		  frame.c skybox.c frame_overlay.c intro_render.c track_preview.c state.c physics_grip.c opponent.c track_setup.c stateply.c physics_collision.c statecrs.c statecar.c trackobj.c replay.c stateio.c

DOS_STARTUP_OBJ =

ifeq ($(TARGET),dosheadless)
  C_FILES = fileio.c legacy.c resource.c math.c memmgr.c strlib.c numlib.c kevinrnd.c replinit.c gameinit.c gamestep.c simd.c carsetup.c trkutil.c trackres.c headless.c \
			headless_data.c headless_trackdata.c state.c physics_grip.c opponent.c track_setup.c stateply.c physics_collision.c statecrs.c statecar.c trackobj.c replay.c stateio.c
  DOS_STARTUP_OBJ = $(OBJDIR)/dosstart.obj
endif

ifeq ($(TARGET),dosfull)
  C_FILES += headless_data.c headless_trackdata.c full_data.c full_strings.c full_tables.c
  DOS_STARTUP_OBJ = $(OBJDIR)/dosstart.obj
endif

OBJ_FILES = $(addprefix $(OBJDIR)/, $(C_FILES:.c=.obj)) $(DOS_STARTUP_OBJ)
C_HEADERS = $(wildcard *.h)

.PHONY: all clean
all: $(OBJ_FILES)

$(OBJDIR):
	$(call MKDIR,$@)

$(OBJDIR)/%.obj: %.c $(C_HEADERS) makefile ../watcom.mk $(WATCOM_STAMP) | $(OBJDIR)
	$(CC) $(WATCOM_CFLAGS) $(CFLAGS) -fo=$@ $<

$(OBJDIR)/dosstart.obj: ../platform/dos/dosstart.c $(C_HEADERS) $(wildcard ../platform/dos/*.h) makefile ../watcom.mk $(WATCOM_STAMP) | $(OBJDIR)
	$(CC) $(WATCOM_PLATFORM_CFLAGS) $(CFLAGS) -i=. -fo=$@ $<

clean:
	$(call RMDIR,$(OBJDIR))
