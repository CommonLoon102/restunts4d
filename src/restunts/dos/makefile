# default target
TARGET ?= ported

SRC = s:\src\restunts

RESTUNTS_OBJ_FILES = fileio.obj math.obj memmgr.obj strlib.obj numlib.obj kevinrnd.obj replinit.obj gameinit.obj gamestep.obj carsetup.obj trkutil.obj restunts.obj \
                     shape3d.obj shape2d.obj heapsort.obj \
                     frame.obj state.obj stateply.obj statecrs.obj \
                     statecar.obj trackobj.obj replay.obj

ASM_OBJ_FILES = segments.obj seg000.obj seg001.obj seg002.obj seg003.obj seg004.obj seg005.obj \
                seg006.obj seg007.obj seg008.obj seg009.obj seg010.obj seg011.obj seg012.obj \
                seg013.obj seg014.obj seg015.obj seg016.obj seg017.obj seg018.obj seg019.obj \
                seg020.obj seg021.obj seg022.obj seg023.obj seg024.obj seg025.obj seg026.obj \
                seg027.obj seg028.obj seg029.obj seg030.obj seg031.obj seg032.obj seg033.obj \
                seg034.obj seg035.obj seg036.obj seg037.obj seg038.obj seg039.obj seg041.obj dseg.obj

CM_LIB_FILES = getvect.obj labs.obj memcpy.obj \
               fmemcpy.obj h_ldiv.obj f_lxmul.obj f_scopy.obj \
               h_lrsh.obj h_padd.obj h_pina.obj h_pada.obj \
               f_pcmp.obj h_lursh.obj h_psbp.obj h_llsh.obj

# prefix with path
RESTUNTS_OBJDIR = $(SRC)\c\build\dos
ASM_OBJDIR = $(SRC)\asm\build
ASMORIG_OBJDIR = $(SRC)\asmorig\build
DOS_PLATFORM_OBJDIR = $(SRC)\platform\dos\build
OBJDIR = $(SRC)\dos\build\$(TARGET)

RESTUNTS_OBJFILES = $(addprefix $(RESTUNTS_OBJDIR)\, $(RESTUNTS_OBJ_FILES))
ASM_OBJFILES = $(addprefix $(ASM_OBJDIR)\, $(ASM_OBJ_FILES))
ASMORIG_OBJFILES = $(addprefix $(ASMORIG_OBJDIR)\, $(ASM_OBJ_FILES))
DOS_PLATFORM_RUNTIME_OBJFILES = $(DOS_PLATFORM_OBJDIR)\timer.obj $(DOS_PLATFORM_OBJDIR)\runtime.obj $(DOS_PLATFORM_OBJDIR)\memory.obj $(DOS_PLATFORM_OBJDIR)\file.obj $(DOS_PLATFORM_OBJDIR)\keyboard.obj $(DOS_PLATFORM_OBJDIR)\divide.obj $(DOS_PLATFORM_OBJDIR)\joystick.obj $(DOS_PLATFORM_OBJDIR)\mouse.obj $(DOS_PLATFORM_OBJDIR)\video.obj $(DOS_PLATFORM_OBJDIR)\audio.obj $(DOS_PLATFORM_OBJDIR)\critical.obj
DOS_BSS_START_OBJFILE = $(DOS_PLATFORM_OBJDIR)\dosbss.obj
DOS_STACK_OBJFILE = $(DOS_PLATFORM_OBJDIR)\dosstack.obj

# C Compiler Flags und Linker
RESTUNTS_CFLAGS = -c -u- -mm -n$(OBJDIR) -v -DRESTUNTS_DOS

ifeq ($(TARGET), ported)
  ALL_OBJ_FILES = $(ASM_OBJFILES) $(RESTUNTS_OBJFILES) $(DOS_PLATFORM_RUNTIME_OBJFILES) $(OBJDIR)\main.obj $(CM_LIB_FILES)
  CFLAGS = 
  OUTFILE = restunts.exe
endif

ifeq ($(TARGET), original)
  ALL_OBJ_FILES = $(ASMORIG_OBJFILES) $(OBJDIR)\main.obj
  CFLAGS = -DRESTUNTS_ORIGINAL
  OUTFILE = restunto.exe
endif

ifeq ($(TARGET), full)
  RESTUNTS_OBJDIR = $(SRC)\c\build\dosfull
  RESTUNTS_OBJ_FILES += dosstart.obj headless_data.obj headless_trackdata.obj full_data.obj full_strings.obj full_tables.obj
  RESTUNTS_OBJFILES = $(addprefix $(RESTUNTS_OBJDIR)\, $(RESTUNTS_OBJ_FILES))
  ALL_OBJ_FILES = $(DOS_BSS_START_OBJFILE) $(RESTUNTS_OBJFILES) $(DOS_PLATFORM_RUNTIME_OBJFILES) $(CM_LIB_FILES) $(DOS_STACK_OBJFILE)
  CFLAGS = -DRESTUNTS_FULL
  OUTFILE = restuntsc.exe
  WLINK_OPTIONS = option START=headless_start option MAP=restuntsc.map
  WLINK_LIBRARIES = library s:\tools\lib\cm.lib
  ENTRY_DEP =
  ifneq ($(findstring \asm\build,$(ALL_OBJ_FILES)),)
    $(error Full target must not link game assembly objects)
  endif
else
  ENTRY_DEP = $(OBJDIR)/main.obj
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

restunts: $(ENTRY_DEP)
  ifeq ($(LINKER), wlink)
		$(LINKER) name $(OUTFILE) format dos option NOCASEEXACT $(WLINK_OPTIONS) PATH $(TOOLS_LIB) file $(CTARGETS) $(WLINK_LIBRARIES)
  else
    # create linker script for tlink
		echo $(CTARGETS) > lscript.txt
		echo $(OUTFILE) >> lscript.txt
		$(LINKER) /s /P- $(LFLAGS) /j$(TOOLS_LIB) @lscript.txt
		del lscript.txt
  endif
	copy $(OUTFILE) s:\stunts

$(OBJDIR)/main.obj: main.c
	$(CC) -I../c $?

clean:
	del build\*.obj
	del build\ported\*.obj
	del build\original\*.obj
	del restunts.exe
	del restunts.map
	del restunto.exe
	del restunto.map
