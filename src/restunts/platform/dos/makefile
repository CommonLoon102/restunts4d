include ../../watcom.mk
OBJDIR = build/watcom/$(CONFIG)
ASMOBJDIR = $(OBJDIR)/$(ASSEMBLER)
ifeq ($(ASSEMBLER),wasm)
ASM = $(ASM_COMMAND) $(WASM_AFLAGS)
else
ASM = $(ASM_COMMAND) /m2 /ml /s /zn
endif
C_HEADERS = $(wildcard ../../c/*.h) $(wildcard *.h)

.PHONY: all clean
all: $(ASMOBJDIR)/dosbss.obj $(ASMOBJDIR)/dosstack.obj $(OBJDIR)/timer.obj $(OBJDIR)/runtime.obj $(OBJDIR)/memory.obj $(OBJDIR)/file.obj $(OBJDIR)/keyboard.obj $(OBJDIR)/divide.obj $(OBJDIR)/joystick.obj $(OBJDIR)/mouse.obj $(OBJDIR)/video.obj $(OBJDIR)/audio.obj $(OBJDIR)/critical.obj

$(OBJDIR) $(ASMOBJDIR):
	$(call MKDIR,$@)

$(ASMOBJDIR)/%.obj: %.asm makefile ../../watcom.mk $(WATCOM_STAMP) | $(ASMOBJDIR)
ifeq ($(ASSEMBLER),wasm)
	$(ASM) -fo="$@" $<
else
	$(ASM) $<, "$(subst /,\,$@)"
endif

$(OBJDIR)/%.obj: %.c $(C_HEADERS) makefile ../../watcom.mk $(WATCOM_STAMP) | $(OBJDIR)
	$(CC) $(WATCOM_PLATFORM_CFLAGS) -i=../../c -fo=$@ $<

clean:
	$(call RMDIR,$(OBJDIR))
