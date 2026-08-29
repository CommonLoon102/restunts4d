TARGET ?= dosfull

OBJDIR = build\$(TARGET)

ifeq ($(TARGET),dosheadless)
  CC = bcc /c /u- /mm /v /n$(OBJDIR) /DRESTUNTS_DOS /DRESTUNTS_HEADLESS

else ifeq ($(TARGET),dosfull)
  CC = bcc /c /u- /mm /v /n$(OBJDIR) /DRESTUNTS_DOS /DRESTUNTS_FULL

# Unsupported target
else
  $(error Unsupported C target: $(TARGET))
endif

C_FILES = fileio.c math.c memmgr.c strlib.c numlib.c kevinrnd.c replinit.c gameinit.c gamestep.c simd.c carsetup.c trkutil.c trackres.c restunts.c shape3d.c shape2d.c heapsort.c \
		  frame.c state.c stateply.c statecrs.c statecar.c trackobj.c replay.c stateio.c

DOS_STARTUP_OBJ =

ifeq ($(TARGET),dosheadless)
  C_FILES = fileio.c math.c memmgr.c strlib.c numlib.c kevinrnd.c replinit.c gameinit.c gamestep.c simd.c carsetup.c trkutil.c trackres.c headless.c \
			headless_data.c headless_trackdata.c state.c stateply.c statecrs.c statecar.c trackobj.c replay.c stateio.c
  DOS_STARTUP_OBJ = $(OBJDIR)/dosstart.obj
endif

ifeq ($(TARGET),dosfull)
  C_FILES += headless_data.c headless_trackdata.c full_data.c full_strings.c full_tables.c
  DOS_STARTUP_OBJ = $(OBJDIR)/dosstart.obj
endif

OBJ_FILES = $(addprefix $(OBJDIR)/, $(C_FILES:.c=.obj)) $(DOS_STARTUP_OBJ)
OBJ_FILES_BACKSLASH = $(subst /,\,$(OBJ_FILES))

all: $(OBJ_FILES)

$(OBJDIR)/%.obj: %.c
  # c file name
	$(eval C_FILE = $<)
  # compiler run
	$(CC) $(C_FILE)

$(OBJDIR)/dosstart.obj: ../platform/dos/dosstart.c legacy.h platform.h
	$(CC) ../platform/dos/dosstart.c

clean:
	del $(OBJ_FILES_BACKSLASH)
