; DOS-only compiler-ABI wrappers for C memory-manager implementations.
;
; The original routines happened to preserve DX, and mmgr_release also
; preserved BX.  Assembly callers rely on that narrower ABI even though the
; C calling convention permits those registers to be clobbered.  Keep the
; preservation outside the portable game implementation.

.model medium

extrn mmgr_path_to_name_impl:proc
extrn copy_paras_reverse_impl:proc
extrn mmgr_find_free_impl:proc
extrn mmgr_release_impl:proc
extrn mmgr_resize_memory_impl:proc

.code

public mmgr_path_to_name
mmgr_path_to_name proc far
	push bp
	mov bp, sp
	push dx
	push word ptr [bp+6]
	call far ptr mmgr_path_to_name_impl
	add sp, 2
	pop dx
	pop bp
	retf
mmgr_path_to_name endp

public copy_paras_reverse
copy_paras_reverse proc far
	push bp
	mov bp, sp
	push dx
	push word ptr [bp+10]
	push word ptr [bp+8]
	push word ptr [bp+6]
	call far ptr copy_paras_reverse_impl
	add sp, 6
	pop dx
	pop bp
	retf
copy_paras_reverse endp

public mmgr_find_free
mmgr_find_free proc far
	push dx
	call far ptr mmgr_find_free_impl
	pop dx
	retf
mmgr_find_free endp

public mmgr_release
mmgr_release proc far
	push bp
	mov bp, sp
	push bx
	push dx
	push word ptr [bp+8]
	push word ptr [bp+6]
	call far ptr mmgr_release_impl
	add sp, 4
	pop dx
	pop bx
	pop bp
	retf
mmgr_release endp

public mmgr_resize_memory
mmgr_resize_memory proc far
	push bp
	mov bp, sp
	push dx
	push word ptr [bp+10]
	push word ptr [bp+8]
	push word ptr [bp+6]
	call far ptr mmgr_resize_memory_impl
	add sp, 6
	pop dx
	pop bp
	retf
mmgr_resize_memory endp

end
