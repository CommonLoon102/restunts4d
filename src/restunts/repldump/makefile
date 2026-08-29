# default target
TARGET ?= ported

SRC = s:\src\restunts

RESTUNTS_OBJ_FILES = fileio.obj math.obj memmgr.obj strlib.obj numlib.obj kevinrnd.obj replinit.obj gameinit.obj gamestep.obj carsetup.obj trkutil.obj restunts.obj \
                     shape3d.obj shape2d.obj keyboard.obj heapsort.obj \
                     frame.obj state.obj stateply.obj statecrs.obj \
                     statecar.obj trackobj.obj replay.obj stateio.obj

ASM_OBJ_FILES = segments.obj seg000.obj seg001.obj seg002.obj seg003.obj seg004.obj seg005.obj \
                seg006.obj seg007.obj seg008.obj seg009.obj seg010.obj seg011.obj seg012.obj \
                seg013.obj seg014.obj seg015.obj seg016.obj seg017.obj seg018.obj seg019.obj \
                seg020.obj seg021.obj seg022.obj seg023.obj seg024.obj seg025.obj seg026.obj \
                seg027.obj seg028.obj seg029.obj seg030.obj seg031.obj seg032.obj seg033.obj \
                seg034.obj seg035.obj seg036.obj seg037.obj seg038.obj seg039.obj seg041.obj dseg.obj

# Transitional headless platform: gameplay and game data are supplied by C;
# only the DOS linker's required stack segment remains platform assembly.
DOS_BSS_START_OBJ_FILE = dosbss.obj
DOS_PLATFORM_OBJ_FILES = dosstack.obj
DOS_PLATFORM_RUNTIME_OBJ_FILES = timer.obj runtime.obj joystick.obj mouse.obj video.obj

ifeq ($(TARGET), stage)
  RESTUNTS_OBJ_FILES = fileio.obj math.obj memmgr.obj strlib.obj numlib.obj \
                       kevinrnd.obj replinit.obj gameinit.obj gamestep.obj \
                       carsetup.obj trkutil.obj headless.obj dosstart.obj headless_data.obj headless_trackdata.obj state.obj stateply.obj \
                       statecrs.obj statecar.obj trackobj.obj replay.obj stateio.obj
endif

CM_LIB_FILES = getvect.obj labs.obj memcpy.obj \
               fmemcpy.obj h_ldiv.obj f_lxmul.obj f_scopy.obj \
               h_lrsh.obj h_padd.obj h_pina.obj h_pada.obj \
               f_pcmp.obj h_lursh.obj h_psbp.obj h_llsh.obj

# prefix with path
RESTUNTS_OBJDIR = $(SRC)\c\build\dos
ASM_OBJDIR = $(SRC)\asm\build
ASMORIG_OBJDIR = $(SRC)\asmorig\build
DOS_PLATFORM_OBJDIR = $(SRC)\platform\dos\build
OBJDIR = $(SRC)\repldump\build\$(TARGET)

ifeq ($(TARGET), stage)
  RESTUNTS_OBJDIR = $(SRC)\c\build\dosstage
endif

RESTUNTS_OBJFILES = $(addprefix $(RESTUNTS_OBJDIR)\, $(RESTUNTS_OBJ_FILES))
ASM_OBJFILES = $(addprefix $(ASM_OBJDIR)\, $(ASM_OBJ_FILES))
ASMORIG_OBJFILES = $(addprefix $(ASMORIG_OBJDIR)\, $(ASM_OBJ_FILES))
DOS_PLATFORM_OBJFILES = $(addprefix $(DOS_PLATFORM_OBJDIR)\, $(DOS_PLATFORM_OBJ_FILES))
DOS_PLATFORM_RUNTIME_OBJFILES = $(addprefix $(DOS_PLATFORM_OBJDIR)\, $(DOS_PLATFORM_RUNTIME_OBJ_FILES))
DOS_BSS_START_OBJFILE = $(DOS_PLATFORM_OBJDIR)\$(DOS_BSS_START_OBJ_FILE)

# C Compiler Flags und Linker
RESTUNTS_CFLAGS = -c -u- -mm -n$(OBJDIR) -v -DRESTUNTS_DOS

ifeq ($(TARGET), ported)
  ALL_OBJ_FILES = $(ASM_OBJFILES) $(RESTUNTS_OBJFILES) $(DOS_PLATFORM_RUNTIME_OBJFILES) $(OBJDIR)\repldump.obj $(CM_LIB_FILES)
  CFLAGS = 
  OUTFILE = repldump.exe
endif

ifeq ($(TARGET), original)
  ALL_OBJ_FILES = $(ASMORIG_OBJFILES) $(OBJDIR)\repldump.obj $(CM_LIB_FILES)
  CFLAGS = -DRESTUNTS_ORIGINAL
  OUTFILE = repldumo.exe
endif

ifeq ($(TARGET), stage)
  # Keep the BSS start marker first and STACK/BSS end marker last.  The custom
  # DOS startup uses those bounds and releases memory through the final stack.
  ALL_OBJ_FILES = $(DOS_BSS_START_OBJFILE) $(RESTUNTS_OBJFILES) $(OBJDIR)\repldump.obj $(CM_LIB_FILES) $(DOS_PLATFORM_OBJFILES)
  CFLAGS = -DRESTUNTS_HEADLESS
  OUTFILE = repldumh.exe
  WLINK_OPTIONS = option START=headless_start option MAP=repldumh.map
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

repldump.exe: $(OBJDIR)/repldump.obj $(RESTUNTS_OBJFILES)
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

$(OBJDIR)/repldump.obj: repldump.c
	$(CC) -I../c $?

clean:
	del build\ported\repldump.obj
	del build\original\repldump.obj
	del build\stage\repldump.obj
	del repldump.exe
	del repldump.map
	del repldumo.exe
	del repldumo.map
	del repldumh.exe
	del repldumh.map
