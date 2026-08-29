TARGET ?= dos

OBJDIR = build\$(TARGET)

# Set up tools for DOS
ifeq ($(TARGET),dos)
  CC = bcc /c /u- /mm /v /n$(OBJDIR) /DRESTUNTS_DOS

else ifeq ($(TARGET),dosstage)
  CC = bcc /c /u- /mm /v /n$(OBJDIR) /DRESTUNTS_DOS /DRESTUNTS_HEADLESS

else ifeq ($(TARGET),dosfull)
  CC = bcc /c /u- /mm /v /n$(OBJDIR) /DRESTUNTS_DOS /DRESTUNTS_FULL

# Set up tools for SDL
else ifeq ($(TARGET),sdl)
  SDLPATH = s:/src/sdl
  CC = cl /Zi /MD /c /I$(SDLPATH)/include /DRESTUNTS_SDL /Fo$(OBJDIR)/$?

# Unsupported target
else
  $(error Unsupported C target: $(TARGET))
endif

C_FILES = fileio.c math.c memmgr.c strlib.c numlib.c kevinrnd.c replinit.c gameinit.c gamestep.c carsetup.c trkutil.c restunts.c shape3d.c shape2d.c heapsort.c \
		  frame.c state.c stateply.c statecrs.c statecar.c trackobj.c replay.c stateio.c

ifeq ($(TARGET),dosstage)
  C_FILES = fileio.c math.c memmgr.c strlib.c numlib.c kevinrnd.c replinit.c gameinit.c gamestep.c carsetup.c trkutil.c headless.c \
			dosstart.c headless_data.c headless_trackdata.c state.c stateply.c statecrs.c statecar.c trackobj.c replay.c stateio.c
endif

ifeq ($(TARGET),dosfull)
  C_FILES += dosstart.c headless_data.c headless_trackdata.c full_data.c full_strings.c full_tables.c
endif

OBJ_FILES = $(addprefix $(OBJDIR)/, $(C_FILES:.c=.obj))
OBJ_FILES_BACKSLASH = $(subst /,\,$(OBJ_FILES))

all: $(OBJ_FILES)

$(OBJDIR)/%.obj: %.c
  # c file name
	$(eval C_FILE = $<)
  # compiler run
	$(CC) $(C_FILE)

clean:
	del $(OBJ_FILES_BACKSLASH)
