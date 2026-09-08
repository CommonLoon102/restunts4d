include watcom.mk

VALID_ASSEMBLERS = tasmbox tasmx tasm32
ifeq (,$(filter $(ASSEMBLER),$(VALID_ASSEMBLERS)))
  $(error Unsupported ASSEMBLER: $(ASSEMBLER))
endif

MAKE_PARAMS = CONFIG=$(CONFIG) ASSEMBLER=$(ASSEMBLER) LINKER=$(LINKER)
.DEFAULT_GOAL := restunts
.PHONY: restunts restunts-original asmorig dosplatform c-headless c-full repldump repldump-original pixldump pixldump-original test-dos-platform clean

restunts: dosplatform c-full
	$(MAKE) -C dos TARGET=full $(MAKE_PARAMS)

restunts-original: asmorig
	$(MAKE) -C dos TARGET=original $(MAKE_PARAMS)

asmorig:
	$(MAKE) -C asmorig $(MAKE_PARAMS)

dosplatform:
	$(MAKE) -C platform/dos $(MAKE_PARAMS)

c-headless:
	$(MAKE) -C c TARGET=dosheadless $(MAKE_PARAMS)

c-full:
	$(MAKE) -C c TARGET=dosfull $(MAKE_PARAMS)

repldump: dosplatform c-headless
	$(MAKE) -C repldump $(MAKE_PARAMS)

# The regression oracles are archived Borland builds, never rebuilt by Watcom.
repldump-original:
	$(call COPY,../../tools/oracles/borland/REPLDUMO.EXE,../../stunts/REPLDUMO.EXE)

pixldump: dosplatform c-full
	$(MAKE) -C pixldump $(MAKE_PARAMS)

pixldump-original:
	$(call COPY,../../tools/oracles/borland/PIXLDUMO.EXE,../../stunts/PIXLDUMO.EXE)

test-dos-platform:
	$(MAKE) -C tests $(MAKE_PARAMS)

clean:
	$(MAKE) -C dos $(MAKE_PARAMS) clean
	$(MAKE) -C asmorig $(MAKE_PARAMS) clean
	$(MAKE) -C platform/dos $(MAKE_PARAMS) clean
	$(MAKE) -C c TARGET=dosheadless $(MAKE_PARAMS) clean
	$(MAKE) -C c TARGET=dosfull $(MAKE_PARAMS) clean
	$(MAKE) -C repldump $(MAKE_PARAMS) clean
	$(MAKE) -C pixldump $(MAKE_PARAMS) clean
	$(MAKE) -C tests $(MAKE_PARAMS) clean
