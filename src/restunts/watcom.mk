# Shared Open Watcom 2 settings for the 16-bit DOS targets.
CONFIG ?= release
ASSEMBLER ?= tasm32
LINKER ?= wlink

ifeq (,$(filter $(CONFIG),release debug))
  $(error Unsupported CONFIG: $(CONFIG))
endif
ifneq ($(LINKER),wlink)
  $(error Open Watcom 2 wlink is required)
endif
TOOLS_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../../tools)
ifeq ($(OS),Windows_NT)
  ifndef WATCOM
    $(error Run setpath.bat after installing Open Watcom 2)
  endif
  CC = $(WATCOM)/binnt/wcc.exe
  WLINK = $(WATCOM)/binnt/wlink.exe
  ASM_COMMAND = $(ASSEMBLER)
  MKDIR = if not exist "$(subst /,\,$(1))" mkdir "$(subst /,\,$(1))"
  RMDIR = if exist "$(subst /,\,$(1))" rmdir /s /q "$(subst /,\,$(1))"
  # REMOVE accepts a single filename or wildcard; use RMDIR for object trees.
  REMOVE = if exist "$(subst /,\,$(1))" del "$(subst /,\,$(1))"
  COPY = copy /b "$(subst /,\,$(1))" "$(subst /,\,$(2))"
else
  WATCOM ?= $(TOOLS_DIR)/watcom
  CC = $(WATCOM)/binl64/wcc
  WLINK = $(WATCOM)/binl64/wlink
  ASM_COMMAND = WINEDEBUG=-all wine $(TOOLS_DIR)/bin/$(ASSEMBLER).exe
  MKDIR = mkdir -p "$(1)"
  RMDIR = rm -rf "$(1)"
  REMOVE = rm -f $(1)
  COPY = cp "$(1)" "$(2)"
endif
export WATCOM
WATCOM_STAMP = $(wildcard $(WATCOM)/restunts-toolchain.version)

# Medium model, stack C calling convention, signed char, byte-packed records.
# Our DOS startup owns stack/BSS initialization, so omit CRT stack probes.
WATCOM_CFLAGS = -zq -bt=dos -0 -mm -ecc -j -zp1 -s -i=$(WATCOM)/h
ifeq ($(CONFIG),debug)
  WATCOM_CFLAGS += -d2 -od
  WATCOM_LFLAGS = debug watcom all
else
  WATCOM_CFLAGS += -os
  WATCOM_LFLAGS =
endif
WATCOM_LIBPATH = libpath $(WATCOM)/lib286 libpath $(WATCOM)/lib286/dos library clibm

# Watcom 2's optimizer can tail-merge bytes inside inline-assembly blocks,
# corrupting local jump targets (observed in dos_file_write). Keep all DOS
# platform/startup code unoptimized; portable game code retains -os.
WATCOM_PLATFORM_CFLAGS = $(filter-out -os -od,$(WATCOM_CFLAGS)) -od
