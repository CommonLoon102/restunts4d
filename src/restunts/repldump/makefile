# default target
TARGET ?= headless

VALID_TARGETS = headless original
ifeq (,$(filter $(TARGET),$(VALID_TARGETS)))
  $(error Unsupported replay-dump target: $(TARGET))
endif

SRC = s:\src\restunts

RESTUNTS_OBJ_FILES = fileio.obj legacy.obj resource.obj math.obj memmgr.obj strlib.obj numlib.obj \
                     kevinrnd.obj replinit.obj gameinit.obj gamestep.obj \
                     simd.obj carsetup.obj trkutil.obj trackres.obj headless.obj dosstart.obj headless_data.obj headless_trackdata.obj state.obj stateply.obj \
                     statecrs.obj statecar.obj trackobj.obj replay.obj stateio.obj

ASMORIG_OBJ_FILES = segments.obj seg000.obj seg001.obj seg002.obj seg003.obj seg004.obj seg005.obj \
                    seg006.obj seg007.obj seg008.obj seg009.obj seg010.obj seg011.obj seg012.obj \
                    seg013.obj seg014.obj seg015.obj seg016.obj seg017.obj seg018.obj seg019.obj \
                    seg020.obj seg021.obj seg022.obj seg023.obj seg024.obj seg025.obj seg026.obj \
                    seg027.obj seg028.obj seg029.obj seg030.obj seg031.obj seg032.obj seg033.obj \
                    seg034.obj seg035.obj seg036.obj seg037.obj seg038.obj seg039.obj seg041.obj dseg.obj

# Transitional headless platform: gameplay and game data are supplied by C;
# only the DOS linker's required stack segment remains platform assembly.
DOS_BSS_START_OBJ_FILE = dosbss.obj
DOS_PLATFORM_OBJ_FILES = runtime.obj memory.obj file.obj divide.obj dosstack.obj
DOS_PLATFORM_RUNTIME_OBJ_FILES = timer.obj runtime.obj memory.obj file.obj keyboard.obj divide.obj joystick.obj mouse.obj video.obj audio.obj critical.obj

CM_LIB_FILES = getvect.obj labs.obj memcpy.obj \
               fmemcpy.obj h_ldiv.obj f_lxmul.obj f_scopy.obj \
               h_lrsh.obj h_padd.obj h_pina.obj h_pada.obj \
               f_pcmp.obj h_lursh.obj h_psbp.obj h_llsh.obj

# prefix with path
RESTUNTS_OBJDIR = $(SRC)\c\build\dosheadless
ASMORIG_OBJDIR = $(SRC)\asmorig\build
DOS_PLATFORM_OBJDIR = $(SRC)\platform\dos\build
OBJDIR = $(SRC)\repldump\build\$(TARGET)

RESTUNTS_OBJFILES = $(addprefix $(RESTUNTS_OBJDIR)\, $(RESTUNTS_OBJ_FILES))
ASMORIG_OBJFILES = $(addprefix $(ASMORIG_OBJDIR)\, $(ASMORIG_OBJ_FILES))
DOS_PLATFORM_OBJFILES = $(addprefix $(DOS_PLATFORM_OBJDIR)\, $(DOS_PLATFORM_OBJ_FILES))
DOS_PLATFORM_RUNTIME_OBJFILES = $(addprefix $(DOS_PLATFORM_OBJDIR)\, $(DOS_PLATFORM_RUNTIME_OBJ_FILES))
DOS_BSS_START_OBJFILE = $(DOS_PLATFORM_OBJDIR)\$(DOS_BSS_START_OBJ_FILE)
REPLDUMP_LINK_DEPS =

# C Compiler Flags und Linker
RESTUNTS_CFLAGS = -c -u- -mm -n$(OBJDIR) -v -DRESTUNTS_DOS

ifeq ($(TARGET), original)
  ALL_OBJ_FILES = $(ASMORIG_OBJFILES) $(OBJDIR)\repldump.obj $(CM_LIB_FILES)
  CFLAGS = -DRESTUNTS_ORIGINAL
  OUTFILE = repldumo.exe
endif

ifeq ($(TARGET), headless)
  # Keep the BSS start marker first and STACK/BSS end marker last.  The custom
  # DOS startup uses those bounds and releases memory through the final stack.
  ALL_OBJ_FILES = $(DOS_BSS_START_OBJFILE) $(RESTUNTS_OBJFILES) $(OBJDIR)\repldump.obj $(CM_LIB_FILES) $(DOS_PLATFORM_OBJFILES)
  CFLAGS = -DRESTUNTS_HEADLESS
  OUTFILE = repldump.exe
  WLINK_OPTIONS = option START=headless_start option MAP=repldump.map
  REPLDUMP_LINK_DEPS = $(RESTUNTS_OBJFILES)
  ifneq ($(findstring \asm\build,$(ALL_OBJ_FILES)),)
    $(error Headless target must not link game assembly objects)
  endif
endif

# set linker separator
SEP := +
ifeq ($(LINKER),wlink)
  SEP := ,
endif
SPACE=$() $()
CTARGETS = $(subst $(SPACE),$(SEP),$(ALL_OBJ_FILES))

CC = bcc $(RESTUNTS_CFLAGS) $(CFLAGS)

LFLAGS = 
ifeq ($(CONFIG), debug)
  LFLAGS = /v
endif

TOOLS_LIB = s:\tools\lib

repldump.exe: force_me $(OBJDIR)/repldump.obj $(REPLDUMP_LINK_DEPS)
  ifeq ($(LINKER), wlink)
		$(LINKER) name $(OUTFILE) format dos option NOCASEEXACT $(WLINK_OPTIONS) PATH $(TOOLS_LIB) file $(CTARGETS)
  else
    # create linker script for tlink
		echo $(CTARGETS) > lscript.txt
		echo $(OUTFILE) >> lscript.txt
		$(LINKER) /s /P- $(LFLAGS) /j$(TOOLS_LIB) @lscript.txt
		del lscript.txt
  endif
	copy $(OUTFILE) s:\stunts

force_me: ;

$(OBJDIR)/repldump.obj: repldump.c
	$(CC) -I../c $?

clean:
	del build\original\repldump.obj
	del build\headless\repldump.obj
	del repldump.exe
	del repldump.map
	del repldumo.exe
	del repldumo.map
