# default target
TARGET ?= ported

VALID_TARGETS = ported original
ifeq (,$(filter $(TARGET),$(VALID_TARGETS)))
  $(error Unsupported pixel-dump target: $(TARGET))
endif

SRC = s:\src\restunts

RESTUNTS_OBJ_FILES = fileio.obj legacy.obj resource.obj math.obj memmgr.obj strlib.obj numlib.obj \
                     kevinrnd.obj replinit.obj gameinit.obj gamestep.obj simd.obj carsetup.obj trkutil.obj trackres.obj restunts.obj audio_sequence.obj \
                     shape3d.obj shape2d.obj heapsort.obj frame.obj state.obj stateply.obj statecrs.obj statecar.obj trackobj.obj replay.obj \
                     headless_data.obj headless_trackdata.obj full_data.obj full_strings.obj full_tables.obj

ASMORIG_OBJ_FILES = segments.obj seg000.obj seg001.obj seg002.obj seg003.obj seg004.obj seg005.obj \
                    seg006.obj seg007.obj seg008.obj seg009.obj seg010.obj seg011.obj seg012.obj \
                    seg013.obj seg014.obj seg015.obj seg016.obj seg017.obj seg018.obj seg019.obj \
                    seg020.obj seg021.obj seg022.obj seg023.obj seg024.obj seg025.obj seg026.obj \
                    seg027.obj seg028.obj seg029.obj seg030.obj seg031.obj seg032.obj seg033.obj \
                    seg034.obj seg035.obj seg036.obj seg037.obj seg038.obj seg039.obj seg041.obj dseg.obj

DOS_PLATFORM_RUNTIME_OBJ_FILES = timer.obj runtime.obj memory.obj file.obj keyboard.obj divide.obj joystick.obj mouse.obj video.obj audio.obj critical.obj
CM_LIB_FILES = getvect.obj labs.obj memcpy.obj \
               fmemcpy.obj h_ldiv.obj f_lxmul.obj f_scopy.obj \
               h_lrsh.obj h_padd.obj h_pina.obj h_pada.obj \
               f_pcmp.obj h_lursh.obj h_psbp.obj h_llsh.obj

RESTUNTS_OBJDIR = $(SRC)\c\build\dosfull
ASMORIG_OBJDIR = $(SRC)\asmorig\build
DOS_PLATFORM_OBJDIR = $(SRC)\platform\dos\build
OBJDIR = $(SRC)\pixldump\build\$(TARGET)

RESTUNTS_OBJFILES = $(addprefix $(RESTUNTS_OBJDIR)\, $(RESTUNTS_OBJ_FILES))
ASMORIG_OBJFILES = $(addprefix $(ASMORIG_OBJDIR)\, $(ASMORIG_OBJ_FILES))
DOS_PLATFORM_RUNTIME_OBJFILES = $(addprefix $(DOS_PLATFORM_OBJDIR)\, $(DOS_PLATFORM_RUNTIME_OBJ_FILES))
DOS_PLATFORM_FILE_OBJFILE = $(DOS_PLATFORM_OBJDIR)\file.obj
DOS_BSS_START_OBJFILE = $(DOS_PLATFORM_OBJDIR)\dosbss.obj
DOS_STACK_OBJFILE = $(DOS_PLATFORM_OBJDIR)\dosstack.obj
PIXLDUMP_OBJFILES = $(OBJDIR)\pixldump.obj $(OBJDIR)\md5.obj
PIXLDUMP_LINK_DEPS =

RESTUNTS_CFLAGS = -c -u- -mm -n$(OBJDIR) -v

ifeq ($(TARGET),original)
  ALL_OBJ_FILES = $(ASMORIG_OBJFILES) $(PIXLDUMP_OBJFILES) $(DOS_PLATFORM_FILE_OBJFILE) $(CM_LIB_FILES)
  CFLAGS = -DRESTUNTS_ORIGINAL
  OUTFILE = pixldumo.exe
  PIXLDUMP_LINK_DEPS = $(DOS_PLATFORM_FILE_OBJFILE)
endif

ifeq ($(TARGET),ported)
  PIXLDUMP_START_OBJFILE = $(OBJDIR)\pixlstart.obj
  ALL_OBJ_FILES = $(DOS_BSS_START_OBJFILE) $(RESTUNTS_OBJFILES) $(PIXLDUMP_OBJFILES) $(PIXLDUMP_START_OBJFILE) $(DOS_PLATFORM_RUNTIME_OBJFILES) $(CM_LIB_FILES) $(DOS_STACK_OBJFILE)
  CFLAGS = -DRESTUNTS_PIXLDUMP
  OUTFILE = pixldump.exe
  WLINK_OPTIONS = option START=headless_start option MAP=pixldump.map
  WLINK_LIBRARIES = library s:\tools\lib\cm.lib
  PIXLDUMP_LINK_DEPS = $(RESTUNTS_OBJFILES) $(OBJDIR)/pixlstart.obj
  FORBIDDEN_GAME_ASM_REFERENCES = $(foreach obj,$(ASMORIG_OBJ_FILES),$(findstring $(obj),$(ALL_OBJ_FILES)))
  ifneq (,$(strip $(FORBIDDEN_GAME_ASM_REFERENCES)))
    $(error Ported target must not link game assembly objects: $(strip $(FORBIDDEN_GAME_ASM_REFERENCES)))
  endif
endif

SEP := +
ifeq ($(LINKER),wlink)
  SEP := ,
endif
SPACE=$() $()
CTARGETS = $(subst $(SPACE),$(SEP),$(ALL_OBJ_FILES))

CC = bcc $(RESTUNTS_CFLAGS) $(CFLAGS)

LFLAGS =
ifeq ($(CONFIG),debug)
  LFLAGS = /v
endif

TOOLS_LIB = s:\tools\lib

pixldump.exe: force_me $(OBJDIR)/pixldump.obj $(OBJDIR)/md5.obj $(PIXLDUMP_LINK_DEPS)
  ifeq ($(LINKER),wlink)
		$(LINKER) name $(OUTFILE) format dos option NOCASEEXACT $(WLINK_OPTIONS) PATH $(TOOLS_LIB) file $(CTARGETS) $(WLINK_LIBRARIES)
  else
		echo $(CTARGETS) > lscript.txt
		echo $(OUTFILE) >> lscript.txt
		$(LINKER) /s /P- $(LFLAGS) /j$(TOOLS_LIB) @lscript.txt
		del lscript.txt
  endif
	copy $(OUTFILE) s:\stunts

force_me: ;

$(OBJDIR)/pixldump.obj: pixldump.c md5.h
	$(CC) -I../c pixldump.c

$(OBJDIR)/md5.obj: md5.c md5.h
	$(CC) -I../c md5.c

$(OBJDIR)/pixlstart.obj: pixlstart.c ../platform/dos/dosstart.c
	$(CC) -I../c pixlstart.c

clean:
	del build\original\pixldump.obj
	del build\original\md5.obj
	del build\ported\pixldump.obj
	del build\ported\md5.obj
	del build\ported\pixlstart.obj
	del pixldump.exe
	del pixldump.map
	del pixldumo.exe
	del pixldumo.map
