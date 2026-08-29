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

all: $(OBJDIR)/dosbss.obj $(OBJDIR)/dosstack.obj $(OBJDIR)/timer.obj $(OBJDIR)/runtime.obj $(OBJDIR)/joystick.obj $(OBJDIR)/mouse.obj $(OBJDIR)/video.obj $(OBJDIR)/audio.obj

$(OBJDIR)/%.obj: %.asm
	$(ASM) $<, $(subst /,\,$@)

$(OBJDIR)/timer.obj: timer.c ../../c/legacy.h
	$(CC) timer.c

$(OBJDIR)/runtime.obj: runtime.c ../../c/legacy.h
	$(CC) runtime.c

$(OBJDIR)/joystick.obj: joystick.c ../../c/legacy.h
	$(CC) joystick.c

$(OBJDIR)/mouse.obj: mouse.c ../../c/legacy.h
	$(CC) mouse.c

$(OBJDIR)/video.obj: video.c ../../c/legacy.h
	$(CC) video.c

$(OBJDIR)/audio.obj: audio.c ../../c/legacy.h
	$(CC) audio.c

clean:
	del $(OBJDIR)\dosbss.obj
	del $(OBJDIR)\dosstack.obj
	del $(OBJDIR)\timer.obj
	del $(OBJDIR)\runtime.obj
	del $(OBJDIR)\joystick.obj
	del $(OBJDIR)\mouse.obj
	del $(OBJDIR)\video.obj
	del $(OBJDIR)\audio.obj
