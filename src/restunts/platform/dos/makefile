ifeq ($(ASSEMBLER),)
  $(error ASSEMBLER is not defined)
endif

OBJDIR = build
TFLAGS = /zn
ifeq ($(CONFIG),debug)
  TFLAGS = /zi
endif

ASM = $(ASSEMBLER) /m2 /s $(TFLAGS)
CC = bcc /c /u- /mm /v /n$(OBJDIR) /DRESTUNTS_DOS

all: $(OBJDIR)/dosbss.obj $(OBJDIR)/dosstack.obj $(OBJDIR)/timer.obj $(OBJDIR)/runtime.obj $(OBJDIR)/memory.obj $(OBJDIR)/file.obj $(OBJDIR)/keyboard.obj $(OBJDIR)/divide.obj $(OBJDIR)/joystick.obj $(OBJDIR)/mouse.obj $(OBJDIR)/video.obj $(OBJDIR)/audio.obj $(OBJDIR)/critical.obj

$(OBJDIR)/%.obj: %.asm
	$(ASM) $<, $(subst /,\,$@)

$(OBJDIR)/timer.obj: timer.c ../../c/legacy.h
	$(CC) timer.c

$(OBJDIR)/runtime.obj: runtime.c ../../c/legacy.h
	$(CC) runtime.c

$(OBJDIR)/memory.obj: memory.c ../../c/legacy.h ../../c/memmgr.h ../../c/platform.h
	$(CC) -I../../c memory.c

$(OBJDIR)/file.obj: file.c ../../c/legacy.h ../../c/platform.h
	$(CC) -I../../c file.c

$(OBJDIR)/keyboard.obj: keyboard.c ../../c/legacy.h ../../c/keyboard.h
	$(CC) -I../../c keyboard.c

$(OBJDIR)/divide.obj: divide.c ../../c/legacy.h ../../c/platform.h
	$(CC) -I../../c divide.c

$(OBJDIR)/joystick.obj: joystick.c ../../c/legacy.h ../../c/platform.h
	$(CC) -I../../c joystick.c

$(OBJDIR)/mouse.obj: mouse.c ../../c/legacy.h ../../c/platform.h
	$(CC) -I../../c mouse.c

$(OBJDIR)/video.obj: video.c ../../c/legacy.h
	$(CC) video.c

$(OBJDIR)/audio.obj: audio.c ../../c/legacy.h
	$(CC) audio.c

$(OBJDIR)/critical.obj: critical.c ../../c/legacy.h
	$(CC) critical.c

clean:
	del $(OBJDIR)\dosbss.obj
	del $(OBJDIR)\dosstack.obj
	del $(OBJDIR)\timer.obj
	del $(OBJDIR)\runtime.obj
	del $(OBJDIR)\memory.obj
	del $(OBJDIR)\file.obj
	del $(OBJDIR)\keyboard.obj
	del $(OBJDIR)\divide.obj
	del $(OBJDIR)\joystick.obj
	del $(OBJDIR)\mouse.obj
	del $(OBJDIR)\video.obj
	del $(OBJDIR)\audio.obj
	del $(OBJDIR)\critical.obj
