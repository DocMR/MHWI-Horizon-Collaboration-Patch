; Call insertion replacing only the eight-byte MOV at PC1199F4B.
; Original RSP is 16-aligned at the insertion point. CALL supplies a normal
; return address; this frame retains standard Windows x64 unwind semantics.
EXTERN stormbolt_sync_complete_dispatch_target:QWORD
PUBLIC stormbolt_sync_complete_entry
.code
stormbolt_sync_complete_entry PROC FRAME
    push rbp
    .pushreg rbp
    lea rsp, [rsp-0D0h]
    .allocstack 0D0h
    lea rbp, [rsp+0D0h]
    .setframe rbp, 0D0h
    .endprolog
    mov [rsp+20h], rax
    mov [rsp+28h], rcx
    mov [rsp+30h], rdx
    mov [rsp+38h], r8
    mov [rsp+40h], r9
    mov [rsp+48h], r10
    mov [rsp+50h], r11
    movdqu [rsp+60h], xmm0
    movdqu [rsp+70h], xmm1
    movdqu [rsp+80h], xmm2
    movdqu [rsp+90h], xmm3
    movdqu [rsp+0A0h], xmm4
    movdqu [rsp+0B0h], xmm5
    pushfq
    pop rax
    mov [rsp+0C0h], rax
    mov rcx, rsi
    lea rdx, [rbp+10h] ; Original producer RSP, before inserted CALL.
    call qword ptr [stormbolt_sync_complete_dispatch_target]
    movdqu xmm0, [rsp+60h]
    movdqu xmm1, [rsp+70h]
    movdqu xmm2, [rsp+80h]
    movdqu xmm3, [rsp+90h]
    movdqu xmm4, [rsp+0A0h]
    movdqu xmm5, [rsp+0B0h]
    mov rax, [rsp+20h]
    mov rcx, [rsp+28h]
    mov rdx, [rsp+30h]
    mov r8, [rsp+38h]
    mov r9, [rsp+40h]
    mov r10, [rsp+48h]
    mov r11, [rsp+50h]
    push qword ptr [rsp+0C0h]
    popfq
    mov r13, qword ptr [rbp+0C8h] ; original RSP = bridge RBP+10h
    lea rsp, [rbp] ; Legal frame-pointer epilogue preserves entering flags.
    pop rbp
    ret
stormbolt_sync_complete_entry ENDP
PUBLIC stormbolt_sync_complete_span_entry
stormbolt_sync_complete_span_entry PROC FRAME
    push rax
    .allocstack 8
    .endprolog
    mov rax, [rsp+8]
    lea rax, [rax+3]
    mov [rsp+8], rax
    pop rax
    jmp stormbolt_sync_complete_entry
stormbolt_sync_complete_span_entry ENDP

END
