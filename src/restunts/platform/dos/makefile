include ../../watcom.mk
OBJDIR = build/watcom/$(CONFIG)
ASM = $(ASM_COMMAND) /m2 /ml /s /zn
C_HEADERS = $(wildcard ../../c/*.h) $(wildcard *.h)

.PHONY: all clean
all: $(OBJDIR)/dosbss.obj $(OBJDIR)/dosstack.obj $(OBJDIR)/timer.obj $(OBJDIR)/runtime.obj $(OBJDIR)/memory.obj $(OBJDIR)/file.obj $(OBJDIR)/keyboard.obj $(OBJDIR)/divide.obj $(OBJDIR)/joystick.obj $(OBJDIR)/mouse.obj $(OBJDIR)/video.obj $(OBJDIR)/audio.obj $(OBJDIR)/critical.obj

$(OBJDIR):
	$(call MKDIR,$@)

$(OBJDIR)/%.obj: %.asm makefile ../../watcom.mk | $(OBJDIR)
	$(ASM) $<, "$(subst /,\,$@)"

$(OBJDIR)/%.obj: %.c $(C_HEADERS) makefile ../../watcom.mk $(WATCOM_STAMP) | $(OBJDIR)
	$(CC) $(WATCOM_PLATFORM_CFLAGS) -i=../../c -fo=$@ $<

clean:
	$(call RMDIR,$(OBJDIR))
