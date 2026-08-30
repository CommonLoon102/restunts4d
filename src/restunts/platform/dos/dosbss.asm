; Mark the beginning of Borland's DGROUP BSS so the custom DOS startup can
; provide the zero-initialization normally performed by the C runtime.
.model medium
_DATA segment word public 'DATA'
_DATA ends
_BSS segment word public 'BSS'
	public headless_bss_start
headless_bss_start label byte
_BSS ends
end
