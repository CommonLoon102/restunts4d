include ../watcom.mk
TARGET ?= dos
ifneq ($(TARGET),dos)
  $(error Unsupported TARGET: $(TARGET))
endif

OBJDIR = build/watcom/$(CONFIG)/wasm

ASM_FILES = segments.asm seg000.asm seg001.asm seg002.asm seg003.asm seg004.asm seg005.asm \
            seg006.asm seg007.asm seg008.asm seg009.asm seg010.asm seg011.asm seg012.asm \
            seg013.asm seg014.asm seg015.asm seg016.asm seg017.asm seg018.asm seg019.asm \
            seg020.asm seg021.asm seg022.asm seg023.asm seg024.asm seg025.asm seg026.asm \
            seg027.asm seg028.asm seg029.asm seg030.asm seg031.asm seg032.asm seg033.asm \
            seg034.asm seg035.asm seg036.asm seg037.asm seg038.asm seg039.asm seg041.asm dseg.asm

OBJ_FILES = $(addprefix $(OBJDIR)/, $(ASM_FILES:.asm=.obj))

ASM = $(WASM) $(WASM_AFLAGS) -cx
GENERATED_DIR = $(OBJDIR)/source
PREPARE_SCRIPT = ../../../tools/scripts/prepare-wasm-original.py
GENERATED_FILES = $(addprefix $(GENERATED_DIR)/,$(ASM_FILES) $(notdir $(wildcard *.inc)))
GENERATED_OUTPUTS = $(GENERATED_DIR)/manifest.json $(GENERATED_FILES)

.PHONY: all clean
all: $(OBJ_FILES)

$(OBJDIR):
	$(call MKDIR,$@)

# One preparation pass also recovers a deleted generated source or include.
$(GENERATED_OUTPUTS) &: $(ASM_FILES) $(wildcard *.inc) $(PREPARE_SCRIPT) makefile ../watcom.mk $(WATCOM_STAMP) | $(OBJDIR)
	$(PYTHON) $(PREPARE_SCRIPT) --source-dir . --output-dir $(GENERATED_DIR)

$(OBJDIR)/%.obj: %.asm $(GENERATED_OUTPUTS) makefile ../watcom.mk $(WATCOM_STAMP) | $(OBJDIR)
	cd "$(GENERATED_DIR)" && $(ASM) -fo="../$(@F)" $(<F)

clean:
	$(call RMDIR,build/watcom)
