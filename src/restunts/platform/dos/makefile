ifeq ($(ASSEMBLER),)
  $(error ASSEMBLER is not defined)
endif

OBJDIR = build
TFLAGS = /zn
ifeq ($(CONFIG),debug)
  TFLAGS = /zi
endif

ASM = $(ASSEMBLER) /m2 /s $(TFLAGS)

all: $(OBJDIR)/dosbss.obj $(OBJDIR)/dosstack.obj

$(OBJDIR)/%.obj: %.asm
	$(ASM) $<, $(subst /,\,$@)

clean:
	del $(OBJDIR)\dosbss.obj
	del $(OBJDIR)\dosstack.obj
