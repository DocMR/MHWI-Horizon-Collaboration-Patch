EXTERN stormbolt_growth_seen:QWORD
EXTERN stormbolt_growth_observe:PROC
.code
EXTERN stormbolt_growth_negative_entry:PROC
PUBLIC stormbolt_growth_test_negative
stormbolt_growth_test_negative PROC FRAME
    push rdi
    .pushreg rdi
    push rsi
    .pushreg rsi
    push r14
    .pushreg r14
    sub rsp, 140h
    .allocstack 140h
    .endprolog
    mov rdi, rcx
    mov r14, r8
    mov esi, r9d
    mov [rsp+118h], rdx
    mov byte ptr [rsp+128h], 0
    mov rax, 11
    mov rcx, 12
    mov rdx, 13
    mov r8, 14
    mov r9, 15
    mov r10, 16
    mov r11, 17
    movq xmm0, rax
    movdqa xmm1, xmm0
    movdqa xmm2, xmm0
    movdqa xmm3, xmm0
    movdqa xmm4, xmm0
    movdqa xmm5, xmm0
    call stormbolt_growth_negative_entry
stormbolt_growth_test_negative_return:
    mov qword ptr [stormbolt_growth_seen], rax
    mov eax, 0
    jmp stormbolt_growth_test_negative_finish
    DB (099h-($-stormbolt_growth_test_negative_return)) DUP(90h)
stormbolt_growth_test_negative_branch:
    mov qword ptr [stormbolt_growth_seen], rax
    mov eax, 1
stormbolt_growth_test_negative_finish:
    mov qword ptr [stormbolt_growth_seen+8], rcx
    mov qword ptr [stormbolt_growth_seen+16], rdx
    mov qword ptr [stormbolt_growth_seen+24], r8
    mov qword ptr [stormbolt_growth_seen+32], r9
    mov qword ptr [stormbolt_growth_seen+40], r10
    mov qword ptr [stormbolt_growth_seen+48], r11
    movdqu xmmword ptr [stormbolt_growth_seen+64], xmm0
    movdqu xmmword ptr [stormbolt_growth_seen+80], xmm1
    movdqu xmmword ptr [stormbolt_growth_seen+96], xmm2
    movdqu xmmword ptr [stormbolt_growth_seen+112], xmm3
    movdqu xmmword ptr [stormbolt_growth_seen+128], xmm4
    movdqu xmmword ptr [stormbolt_growth_seen+144], xmm5
    pushfq
    pop qword ptr [stormbolt_growth_seen+56]
    add rsp, 140h
    pop r14
    pop rsi
    pop rdi
    ret
stormbolt_growth_test_negative ENDP
EXTERN stormbolt_growth_post_effect_entry:PROC
PUBLIC stormbolt_growth_test_post_effect
stormbolt_growth_test_post_effect PROC FRAME
    push rdi
    .pushreg rdi
    push rsi
    .pushreg rsi
    push r14
    .pushreg r14
    sub rsp, 140h
    .allocstack 140h
    .endprolog
    mov rdi, rcx
    mov r14, r8
    mov esi, r9d
    mov [rsp+118h], rdx
    mov byte ptr [rsp+128h], 0
    mov rax, 11
    mov rcx, 12
    mov rdx, 13
    mov r8, 14
    mov r9, 15
    mov r10, 16
    mov r11, 17
    movq xmm0, rax
    movdqa xmm1, xmm0
    movdqa xmm2, xmm0
    movdqa xmm3, xmm0
    movdqa xmm4, xmm0
    movdqa xmm5, xmm0
    call stormbolt_growth_post_effect_entry
stormbolt_growth_test_post_effect_return:
    mov qword ptr [stormbolt_growth_seen], rax
    mov eax, 0
    jmp stormbolt_growth_test_post_effect_finish
    DB (07Ah-($-stormbolt_growth_test_post_effect_return)) DUP(90h)
stormbolt_growth_test_post_effect_branch:
    mov qword ptr [stormbolt_growth_seen], rax
    mov eax, 1
stormbolt_growth_test_post_effect_finish:
    mov qword ptr [stormbolt_growth_seen+8], rcx
    mov qword ptr [stormbolt_growth_seen+16], rdx
    mov qword ptr [stormbolt_growth_seen+24], r8
    mov qword ptr [stormbolt_growth_seen+32], r9
    mov qword ptr [stormbolt_growth_seen+40], r10
    mov qword ptr [stormbolt_growth_seen+48], r11
    movdqu xmmword ptr [stormbolt_growth_seen+64], xmm0
    movdqu xmmword ptr [stormbolt_growth_seen+80], xmm1
    movdqu xmmword ptr [stormbolt_growth_seen+96], xmm2
    movdqu xmmword ptr [stormbolt_growth_seen+112], xmm3
    movdqu xmmword ptr [stormbolt_growth_seen+128], xmm4
    movdqu xmmword ptr [stormbolt_growth_seen+144], xmm5
    pushfq
    pop qword ptr [stormbolt_growth_seen+56]
    add rsp, 140h
    pop r14
    pop rsi
    pop rdi
    ret
stormbolt_growth_test_post_effect ENDP
EXTERN stormbolt_growth_remaining_entry:PROC
PUBLIC stormbolt_growth_test_remaining
stormbolt_growth_test_remaining PROC FRAME
    push rdi
    .pushreg rdi
    push rsi
    .pushreg rsi
    push r14
    .pushreg r14
    sub rsp, 140h
    .allocstack 140h
    .endprolog
    mov rdi, rcx
    mov r14, r8
    mov esi, r9d
    mov [rsp+118h], rdx
    mov byte ptr [rsp+128h], 0
    mov rax, 11
    mov rcx, 12
    mov rdx, 13
    mov r8, 14
    mov r9, 15
    mov r10, 16
    mov r11, 17
    movq xmm0, rax
    movdqa xmm1, xmm0
    movdqa xmm2, xmm0
    movdqa xmm3, xmm0
    movdqa xmm4, xmm0
    movdqa xmm5, xmm0
    call stormbolt_growth_remaining_entry
stormbolt_growth_test_remaining_return:
    mov qword ptr [stormbolt_growth_seen], rax
    mov eax, 0
    jmp stormbolt_growth_test_remaining_finish
    DB (071h-($-stormbolt_growth_test_remaining_return)) DUP(90h)
stormbolt_growth_test_remaining_branch:
    mov qword ptr [stormbolt_growth_seen], rax
    mov eax, 1
stormbolt_growth_test_remaining_finish:
    mov qword ptr [stormbolt_growth_seen+8], rcx
    mov qword ptr [stormbolt_growth_seen+16], rdx
    mov qword ptr [stormbolt_growth_seen+24], r8
    mov qword ptr [stormbolt_growth_seen+32], r9
    mov qword ptr [stormbolt_growth_seen+40], r10
    mov qword ptr [stormbolt_growth_seen+48], r11
    movdqu xmmword ptr [stormbolt_growth_seen+64], xmm0
    movdqu xmmword ptr [stormbolt_growth_seen+80], xmm1
    movdqu xmmword ptr [stormbolt_growth_seen+96], xmm2
    movdqu xmmword ptr [stormbolt_growth_seen+112], xmm3
    movdqu xmmword ptr [stormbolt_growth_seen+128], xmm4
    movdqu xmmword ptr [stormbolt_growth_seen+144], xmm5
    pushfq
    pop qword ptr [stormbolt_growth_seen+56]
    add rsp, 140h
    pop r14
    pop rsi
    pop rdi
    ret
stormbolt_growth_test_remaining ENDP
PUBLIC stormbolt_growth_clobber_callback
stormbolt_growth_clobber_callback PROC FRAME
    sub rsp, 28h
    .allocstack 28h
    .endprolog
    call stormbolt_growth_observe
    add rsp, 28h
    mov rax, 0DEADBEEFh
    mov rcx, 0DEADBEEFh
    mov rdx, 0DEADBEEFh
    mov r8, 0DEADBEEFh
    mov r9, 0DEADBEEFh
    mov r10, 0DEADBEEFh
    mov r11, 0DEADBEEFh
    pcmpeqd xmm0, xmm0
    pcmpeqd xmm1, xmm1
    pcmpeqd xmm2, xmm2
    pcmpeqd xmm3, xmm3
    pcmpeqd xmm4, xmm4
    pcmpeqd xmm5, xmm5
    stc
    ret
stormbolt_growth_clobber_callback ENDP
PUBLIC stormbolt_growth_test_path
stormbolt_growth_test_path PROC FRAME
    push rdi
    .pushreg rdi
    push rsi
    .pushreg rsi
    push r14
    .pushreg r14
    sub rsp, 140h
    .allocstack 140h
    .endprolog
    mov rdi, rcx
    mov r14, r8
    mov esi, r9d
    mov [rsp+108h], rdx ; fragment CALL and SUB8 move native RSP down16
    mov byte ptr [rsp+118h], 0
    mov rax, [rsp+180h] ; fifth argument: allocated fragment entry
    call rax
    add rsp, 140h
    pop r14
    pop rsi
    pop rdi
    ret
stormbolt_growth_test_path ENDP
END
