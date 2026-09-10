; DOS executables need a linker-visible STACK segment.  The portable game and
; replay engine do not depend on its contents; this is platform startup data.
.model medium
_BSS segment word public 'BSS'
	public _headless_bss_end
_headless_bss_end label byte
_BSS ends
DOS_STACK segment para stack 'STACK'
	db 8192 dup(?)
	public _headless_stack_top
_headless_stack_top label byte
DOS_STACK ends
DGROUP group DOS_STACK
end
