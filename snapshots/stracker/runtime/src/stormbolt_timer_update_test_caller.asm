EXTERN stormbolt_timer_update_entry:PROC
EXTERN stormbolt_timer_test_seen:QWORD
EXTERN stormbolt_timer_test_flag:BYTE
EXTERN stormbolt_timer_test_query_value:QWORD
EXTERN stormbolt_timer_test_next_query_value:QWORD
EXTERN stormbolt_timer_test_step:DWORD
EXTERN stormbolt_timer_test_step_seen:DWORD
EXTERN stormbolt_timer_test_query_calls:DWORD
EXTERN stormbolt_timer_test_query_owner:QWORD
PUBLIC stormbolt_timer_test_query
PUBLIC stormbolt_timer_test_call
PUBLIC stormbolt_timer_test_dispatch
PUBLIC stormbolt_timer_test_patch_site
.code
stormbolt_timer_test_dispatch PROC
    mov stormbolt_timer_test_seen, rcx
    movss stormbolt_timer_test_step_seen, xmm1
    mov rax, stormbolt_timer_test_next_query_value
    mov stormbolt_timer_test_query_value, rax
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
stormbolt_timer_test_dispatch ENDP
stormbolt_timer_test_call PROC FRAME
    push rsi
    .pushreg rsi
    push rbx
    .pushreg rbx
    sub rsp, 48h
    .allocstack 48h
    movdqu [rsp+20h], xmm9
    .savexmm128 xmm9, 20h
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
    movss xmm9, stormbolt_timer_test_step
    cmp stormbolt_timer_test_flag, 1
    pushfq
    pop qword ptr [rbx+0A0h]
    mov rcx, [rsi+8]
stormbolt_timer_test_patch_site LABEL BYTE
    call stormbolt_timer_test_query
    test al, al
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
    movdqu xmm9, [rsp+20h]
    add rsp, 48h
    pop rbx
    pop rsi
    ret
stormbolt_timer_test_call ENDP
stormbolt_timer_test_query PROC
    mov stormbolt_timer_test_query_owner, rcx
    inc stormbolt_timer_test_query_calls
    mov rax, stormbolt_timer_test_query_value
    ret
stormbolt_timer_test_query ENDP
END
