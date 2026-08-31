# Native Open Watcom 2 build for the ported DOS executables.
#
# Set WATCOM to an Open Watcom 2 installation. On 64-bit Linux the portable
# snapshot's tools live in $(WATCOM)/binl64; override OW_BIN for another host.

CONFIG ?= release
VALID_CONFIGS := debug release

ifeq (,$(filter $(CONFIG),$(VALID_CONFIGS)))
  $(error unknown CONFIG value: $(CONFIG))
endif

OW_BIN ?= $(WATCOM)/binl64
WCC ?= $(OW_BIN)/wcc
WASM ?= $(OW_BIN)/wasm
WLINK ?= $(OW_BIN)/wlink
OW_CLIBM := $(WATCOM)/lib286/dos/clibm.lib

OW_BUILD := build/openwatcom/$(CONFIG)
OW_FULL_C_BUILD := $(OW_BUILD)/full/c
OW_HEADLESS_C_BUILD := $(OW_BUILD)/headless/c
OW_PLATFORM_BUILD := $(OW_BUILD)/platform/dos

OW_COMMON_CFLAGS := -zq -bt=dos -0 -mm -ecc -j -s -zl \
	-i=$(WATCOM)/h -i=c
OW_ASMFLAGS := -zq -bt=dos -0 -mm

ifeq ($(CONFIG),debug)
  OW_CONFIG_CFLAGS := -d2 -od
  OW_CONFIG_ASMFLAGS := -d2
  OW_LINK_DEBUG := debug all
else
  OW_CONFIG_CFLAGS := -os
  OW_CONFIG_ASMFLAGS :=
  OW_LINK_DEBUG :=
endif

OW_FULL_C_NAMES := fileio legacy resource math memmgr strlib numlib \
	kevinrnd replinit gameinit gamestep simd carsetup trkutil trackres \
	restunts audio_sequence shape3d shape2d heapsort frame state stateply \
	statecrs statecar trackobj replay headless_data headless_trackdata \
	full_data full_strings full_tables

OW_HEADLESS_C_NAMES := fileio legacy resource math memmgr strlib numlib \
	kevinrnd replinit gameinit gamestep simd carsetup trkutil trackres \
	headless headless_data headless_trackdata state stateply statecrs \
	statecar trackobj replay stateio

OW_FULL_PLATFORM_NAMES := timer runtime memory file keyboard divide joystick \
	mouse video audio critical
OW_HEADLESS_PLATFORM_NAMES := runtime memory file divide

OW_FULL_C_OBJECTS := $(addprefix $(OW_FULL_C_BUILD)/,$(addsuffix .obj,$(OW_FULL_C_NAMES)))
OW_HEADLESS_C_OBJECTS := $(addprefix $(OW_HEADLESS_C_BUILD)/,$(addsuffix .obj,$(OW_HEADLESS_C_NAMES)))
OW_FULL_PLATFORM_OBJECTS := $(addprefix $(OW_PLATFORM_BUILD)/,$(addsuffix .obj,$(OW_FULL_PLATFORM_NAMES)))
OW_HEADLESS_PLATFORM_OBJECTS := $(addprefix $(OW_PLATFORM_BUILD)/,$(addsuffix .obj,$(OW_HEADLESS_PLATFORM_NAMES)))

OW_FULL_STARTUP := $(OW_FULL_C_BUILD)/dosstart.obj
OW_HEADLESS_STARTUP := $(OW_HEADLESS_C_BUILD)/dosstart.obj
OW_REPLDUMP_OBJECT := $(OW_HEADLESS_C_BUILD)/repldump.obj
OW_BSS_OBJECT := $(OW_PLATFORM_BUILD)/dosbss.obj
OW_STACK_OBJECT := $(OW_PLATFORM_BUILD)/dosstack.obj

OW_FULL_LINK_OBJECTS := $(OW_BSS_OBJECT) $(OW_FULL_C_OBJECTS) \
	$(OW_FULL_STARTUP) $(OW_FULL_PLATFORM_OBJECTS) $(OW_STACK_OBJECT)
OW_HEADLESS_LINK_OBJECTS := $(OW_BSS_OBJECT) $(OW_HEADLESS_C_OBJECTS) \
	$(OW_HEADLESS_STARTUP) $(OW_REPLDUMP_OBJECT) \
	$(OW_HEADLESS_PLATFORM_OBJECTS) $(OW_STACK_OBJECT)

OW_FULL_EXE := dos/restunts.exe
OW_FULL_MAP := dos/restunts.map
OW_HEADLESS_EXE := repldump/repldump.exe
OW_HEADLESS_MAP := repldump/repldump.map

C_HEADERS := $(wildcard c/*.h) platform/dos/doscompat.h

.PHONY: all restunts repldump c-full c-headless dosplatform \
	check-openwatcom clean clean-modern restunts-original repldump-original \
	asmorig clean-legacy restunts-borland repldump-borland \
	legacy-c-full legacy-c-headless legacy-dosplatform force-link

all: restunts repldump

restunts: $(OW_FULL_EXE)
	cp -f $(OW_FULL_EXE) ../../stunts/RESTUNTS.EXE

repldump: $(OW_HEADLESS_EXE)
	cp -f $(OW_HEADLESS_EXE) ../../stunts/REPLDUMP.EXE

c-full: $(OW_FULL_C_OBJECTS) $(OW_FULL_STARTUP)

c-headless: $(OW_HEADLESS_C_OBJECTS) $(OW_HEADLESS_STARTUP)

dosplatform: $(OW_BSS_OBJECT) $(OW_FULL_PLATFORM_OBJECTS) $(OW_STACK_OBJECT)

check-openwatcom:
	@test -n "$(WATCOM)" || { \
		echo "WATCOM is not set; point it at an Open Watcom 2 installation" >&2; \
		exit 1; \
	}
	@test -x "$(WCC)" || { echo "Open Watcom compiler not found: $(WCC)" >&2; exit 1; }
	@test -x "$(WASM)" || { echo "Open Watcom assembler not found: $(WASM)" >&2; exit 1; }
	@test -x "$(WLINK)" || { echo "Open Watcom linker not found: $(WLINK)" >&2; exit 1; }
	@test -f "$(OW_CLIBM)" || { echo "Open Watcom DOS library not found: $(OW_CLIBM)" >&2; exit 1; }

$(OW_FULL_C_BUILD)/%.obj: c/%.c $(C_HEADERS) | check-openwatcom
	@mkdir -p $(@D)
	$(WCC) $(OW_COMMON_CFLAGS) $(OW_CONFIG_CFLAGS) -dRESTUNTS_FULL -fo=$@ $<

$(OW_HEADLESS_C_BUILD)/%.obj: c/%.c $(C_HEADERS) | check-openwatcom
	@mkdir -p $(@D)
	$(WCC) $(OW_COMMON_CFLAGS) $(OW_CONFIG_CFLAGS) -dRESTUNTS_HEADLESS -fo=$@ $<

$(OW_FULL_STARTUP): platform/dos/dosstart.c $(C_HEADERS) | check-openwatcom
	@mkdir -p $(@D)
	$(WCC) $(OW_COMMON_CFLAGS) $(OW_CONFIG_CFLAGS) -dRESTUNTS_FULL \
		-i=platform/dos -fo=$@ $<

$(OW_HEADLESS_STARTUP): platform/dos/dosstart.c $(C_HEADERS) | check-openwatcom
	@mkdir -p $(@D)
	$(WCC) $(OW_COMMON_CFLAGS) $(OW_CONFIG_CFLAGS) -dRESTUNTS_HEADLESS \
		-i=platform/dos -fo=$@ $<

$(OW_REPLDUMP_OBJECT): repldump/repldump.c $(C_HEADERS) | check-openwatcom
	@mkdir -p $(@D)
	$(WCC) $(OW_COMMON_CFLAGS) $(OW_CONFIG_CFLAGS) -dRESTUNTS_HEADLESS -fo=$@ $<

$(OW_PLATFORM_BUILD)/%.obj: platform/dos/%.c $(C_HEADERS) | check-openwatcom
	@mkdir -p $(@D)
	$(WCC) $(OW_COMMON_CFLAGS) $(OW_CONFIG_CFLAGS) -i=platform/dos -fo=$@ $<

$(OW_PLATFORM_BUILD)/%.obj: platform/dos/%.asm | check-openwatcom
	@mkdir -p $(@D)
	$(WASM) $(OW_ASMFLAGS) $(OW_CONFIG_ASMFLAGS) -fo=$@ $<

force-link:

$(OW_FULL_EXE): $(OW_FULL_LINK_OBJECTS) force-link | check-openwatcom
	$(WLINK) name $@ format dos option nocaseexact option start=headless_start \
		option map=$(OW_FULL_MAP) $(OW_LINK_DEBUG) \
		$(foreach object,$(OW_FULL_LINK_OBJECTS),file $(object)) \
		library $(OW_CLIBM)

$(OW_HEADLESS_EXE): $(OW_HEADLESS_LINK_OBJECTS) force-link | check-openwatcom
	$(WLINK) name $@ format dos option nocaseexact option start=headless_start \
		option map=$(OW_HEADLESS_MAP) $(OW_LINK_DEBUG) \
		$(foreach object,$(OW_HEADLESS_LINK_OBJECTS),file $(object)) \
		library $(OW_CLIBM)

clean: clean-modern

clean-modern:
	rm -rf build/openwatcom
	rm -f dos/restunts.exe dos/restunts.map
	rm -f repldump/repldump.exe repldump/repldump.map

# The exact original reconstruction remains on the historical Borland/TASM
# path. Run these targets from the existing Windows/Wine build environment.
ASSEMBLER ?= tasm32
LINKER ?= tlinkbox
LEGACY_PARAMS := CONFIG=$(CONFIG) ASSEMBLER=$(ASSEMBLER) LINKER=$(LINKER)

asmorig:
	$(MAKE) -C asmorig $(LEGACY_PARAMS)

restunts-original: asmorig
	$(MAKE) -C dos TARGET=original $(LEGACY_PARAMS)

repldump-original: asmorig
	$(MAKE) -C repldump TARGET=original $(LEGACY_PARAMS)

legacy-c-full:
	$(MAKE) -C c TARGET=dosfull $(LEGACY_PARAMS)

legacy-c-headless:
	$(MAKE) -C c TARGET=dosheadless $(LEGACY_PARAMS)

legacy-dosplatform:
	$(MAKE) -C platform/dos $(LEGACY_PARAMS)

restunts-borland: legacy-dosplatform legacy-c-full
	$(MAKE) -C dos TARGET=full CONFIG=$(CONFIG) ASSEMBLER=$(ASSEMBLER) LINKER=wlink

repldump-borland: legacy-dosplatform legacy-c-headless
	$(MAKE) -C repldump TARGET=headless CONFIG=$(CONFIG) ASSEMBLER=$(ASSEMBLER) LINKER=wlink

clean-legacy:
	$(MAKE) -C dos $(LEGACY_PARAMS) clean
	$(MAKE) -C asmorig $(LEGACY_PARAMS) clean
	$(MAKE) -C platform/dos $(LEGACY_PARAMS) clean
	$(MAKE) -C c TARGET=dosheadless $(LEGACY_PARAMS) clean
	$(MAKE) -C c TARGET=dosfull $(LEGACY_PARAMS) clean
	$(MAKE) -C repldump $(LEGACY_PARAMS) clean
