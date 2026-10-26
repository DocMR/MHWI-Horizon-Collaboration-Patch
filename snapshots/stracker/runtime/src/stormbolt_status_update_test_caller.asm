EXTERN stormbolt_status_update_entry:PROC
EXTERN stormbolt_status_test_seen:QWORD
EXTERN stormbolt_status_test_flag:BYTE
EXTERN stormbolt_status_test_manager:QWORD
EXTERN stormbolt_status_test_next_manager:QWORD
PUBLIC stormbolt_status_test_call
PUBLIC stormbolt_status_test_dispatch
PUBLIC stormbolt_status_test_patch_site
.code
stormbolt_status_test_dispatch PROC
    mov stormbolt_status_test_seen, rcx
    mov rax, stormbolt_status_test_next_manager
    mov stormbolt_status_test_manager, rax
    xor eax, eax
    xor ecx, ecx
    xor edx, edx
    xor r8d, r8d
    xor r9d, r9d
    xor r10d, r10d
    xor r11d, r11d
    pxor xmm0, xmm0
    pxor xmm1, xmm1
    pxor xmm2, xmm2
    pxor xmm3, xmm3
    pxor xmm4, xmm4
    pxor xmm5, xmm5
    ret
stormbolt_status_test_dispatch ENDP
stormbolt_status_test_call PROC FRAME
    push rsi
    .pushreg rsi
    push rbx
    .pushreg rbx
    sub rsp, 28h
    .allocstack 28h
    .endprolog
    mov rsi, rcx
    mov rbx, rdx
    mov rax, 1234h
    mov rcx, rax
    mov rdx, rax
    mov r8, rax
    mov r9, rax
    mov r10, rax
    mov r11, rax
    pcmpeqd xmm0, xmm0
    movdqa xmm1, xmm0
    movdqa xmm2, xmm0
    movdqa xmm3, xmm0
    movdqa xmm4, xmm0
    movdqa xmm5, xmm0
    cmp stormbolt_status_test_flag, 1
    pushfq
    pop qword ptr [rbx+0A0h]
stormbolt_status_test_patch_site LABEL BYTE
    mov rcx, stormbolt_status_test_manager
collect_result:
    pushfq
    pop qword ptr [rbx+0A8h]
    setz byte ptr [rbx+98h]
    mov [rbx], rax
    mov [rbx+8], rcx
    mov [rbx+10h], rdx
    mov [rbx+18h], r8
    mov [rbx+20h], r9
    mov [rbx+28h], r10
    mov [rbx+30h], r11
    movdqu [rbx+38h], xmm0
    movdqu [rbx+48h], xmm1
    movdqu [rbx+58h], xmm2
    movdqu [rbx+68h], xmm3
    movdqu [rbx+78h], xmm4
    movdqu [rbx+88h], xmm5
    add rsp, 28h
    pop rbx
    pop rsi
    ret
stormbolt_status_test_call ENDP
END
