# options
CONFIG ?= release
ASSEMBLER ?= tasm32
LINKER ?= tlinkbox

VALID_CONFIGS = debug release

VALID_ASSEMBLERS = tasmbox tasmx tasm32

BORLAND_LINKERS = tlinkbox tlink
VALID_LINKERS = $(BORLAND_LINKERS) wlink

# valid CONFIG
ifeq (,$(findstring $(CONFIG),$(VALID_CONFIGS)))
  $(error unknown CONFIG value: $(CONFIG))
endif

# valid ASSEMBLER
ifeq (,$(findstring $(ASSEMBLER),$(VALID_ASSEMBLERS)))
  $(error unknown ASSEMBLER value: $(ASSEMBLER))
endif

# valid LINKER
ifeq (,$(findstring $(LINKER),$(VALID_LINKERS)))
  $(error unknown LINKER value: $(LINKER))
endif

# valid option combination
ifeq ($(CONFIG),debug)
  ifeq ($(ASSEMBLER),tasm32)
    $(error ASSEMBLER $(ASSEMBLER) is not supported for CONFIG $(CONFIG))
  endif
  ifeq ($(LINKER),wlink)
    $(error LINKER $(LINKER) is not supported for CONFIG $(CONFIG))
  endif
endif

MAKE_PARAMS = CONFIG=$(CONFIG) ASSEMBLER=$(ASSEMBLER) LINKER=$(LINKER)

restunts: asm c dosplatform
	make -C dos $(MAKE_PARAMS)

restunts-original: asmorig
	make -C dos TARGET=original $(MAKE_PARAMS)

asm: force_me
	make -C asm $(MAKE_PARAMS)

asmorig: force_me
	make -C asmorig $(MAKE_PARAMS)

c: force_me
	make -C c $(MAKE_PARAMS)

dosplatform: force_me
	make -C platform/dos $(MAKE_PARAMS)

c-stage: force_me
	make -C c TARGET=dosstage $(MAKE_PARAMS)

c-full: force_me
	make -C c TARGET=dosfull $(MAKE_PARAMS)

restunts-full: dosplatform c-full
	make -C dos TARGET=full CONFIG=$(CONFIG) ASSEMBLER=$(ASSEMBLER) LINKER=wlink

restunts-sdl:
	make -C sdl $(MAKE_PARAMS)

repldump: asm c dosplatform
	make -C repldump $(MAKE_PARAMS)

repldump-original: asmorig
	make -C repldump TARGET=original $(MAKE_PARAMS)

repldump-stage: dosplatform c-stage
	make -C repldump TARGET=stage CONFIG=$(CONFIG) ASSEMBLER=$(ASSEMBLER) LINKER=wlink

clean:
	make -C dos $(MAKE_PARAMS) clean
	make -C asm $(MAKE_PARAMS) clean
	make -C asmorig $(MAKE_PARAMS) clean
	make -C platform/dos $(MAKE_PARAMS) clean
	make -C c $(MAKE_PARAMS) clean
	make -C sdl $(MAKE_PARAMS) clean
	make -C repldump clean

# i have no idea why i need to force pre-targets with an empty target
force_me: ;
