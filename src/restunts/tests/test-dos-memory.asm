; Link this DOS ABI test with the actual Borland-compiled platform memory.obj.
; A host C stub cannot catch inline-assembler keyword collisions.
; From src/restunts after building dosplatform:
; tasm32 /m2 /s tests\test-dos-memory.asm, testmem.obj
; wlink name testmem.exe format dos option NOCASEEXACT option start=test_start file testmem.obj,platform\dos\build\memory.obj
; Run testmem.exe in DOSBox; a contract violation exits with status 1.
.model medium
.stack 1024
.data
passed db 'DOS memory PSP contract passed.', 13, 10, '$'
failed db 'DOS memory PSP contract failed.', 13, 10, '$'
.code
extrn dos_memory_get_psp:far
public test_start
test_start proc far
    mov ax, @data
    mov ds, ax
    mov ah, 62h
    int 21h
    push bx
    call dos_memory_get_psp
    pop bx
    cmp ax, bx
    jne test_failed
    mov cx, ds
    cmp dx, cx
    jne test_failed
    mov dx, offset passed
    mov ah, 9
    int 21h
    mov ax, 4C00h
    int 21h
test_failed:
    mov dx, offset failed
    mov ah, 9
    int 21h
    mov ax, 4C01h
    int 21h
test_start endp
end test_start
