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

all: $(OBJDIR)/dosbss.obj $(OBJDIR)/dosstack.obj $(OBJDIR)/timer.obj

$(OBJDIR)/%.obj: %.asm
	$(ASM) $<, $(subst /,\,$@)

$(OBJDIR)/timer.obj: timer.c ../../c/legacy.h
	$(CC) timer.c

clean:
	del $(OBJDIR)\dosbss.obj
	del $(OBJDIR)\dosstack.obj
	del $(OBJDIR)\timer.obj
