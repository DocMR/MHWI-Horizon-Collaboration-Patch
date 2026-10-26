EXTERN stormbolt_growth_dispatch_target:QWORD
.code
GROWTH_NEGATIVE MACRO entryName, returnSkip
    LOCAL stormbolt_growth_negative_entry_skip, stormbolt_growth_negative_entry_branch, stormbolt_growth_negative_entry_done
entryName PROC FRAME
    push rbp
    .pushreg rbp
    lea rsp, [rsp-0D0h]
    .allocstack 0D0h
    lea rbp, [rsp+0D0h]
    .setframe rbp, 0D0h
    .endprolog
    mov [rsp+20h], rax
IF returnSkip
    mov rax, [rbp+8]
    lea rax, [rax+returnSkip]
    mov [rbp+8], rax
ENDIF
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
    test esi, esi
    jns stormbolt_growth_negative_entry_skip
    mov rcx, rdi
    mov rdx, [rbp+128h] ; native RSP+118
    mov r8, r14
    call qword ptr [stormbolt_growth_dispatch_target]
stormbolt_growth_negative_entry_skip:
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
    test esi, esi
    jns stormbolt_growth_negative_entry_done
stormbolt_growth_negative_entry_branch:
    mov rax, [rbp+8]
    lea rax, [rax+099h-returnSkip]
    mov [rbp+8], rax
    mov rax, [rsp+20h]
stormbolt_growth_negative_entry_done:
    lea rsp, [rbp]
    pop rbp
    ret
entryName ENDP
ENDM
PUBLIC stormbolt_growth_negative_entry
PUBLIC stormbolt_growth_negative_entry_span
GROWTH_NEGATIVE stormbolt_growth_negative_entry, 0
GROWTH_NEGATIVE stormbolt_growth_negative_entry_span, 3

GROWTH_POST_EFFECT MACRO entryName, returnSkip
    LOCAL stormbolt_growth_post_effect_entry_invoke, stormbolt_growth_post_effect_entry_skip, stormbolt_growth_post_effect_entry_branch, stormbolt_growth_post_effect_entry_done
entryName PROC FRAME
    push rbp
    .pushreg rbp
    lea rsp, [rsp-0D0h]
    .allocstack 0D0h
    lea rbp, [rsp+0D0h]
    .setframe rbp, 0D0h
    .endprolog
    mov [rsp+20h], rax
IF returnSkip
    mov rax, [rbp+8]
    lea rax, [rax+returnSkip]
    mov [rbp+8], rax
ENDIF
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
    test esi, esi
    jz stormbolt_growth_post_effect_entry_invoke
    cmp esi, 64h
    jne stormbolt_growth_post_effect_entry_skip
stormbolt_growth_post_effect_entry_invoke:
    mov rcx, rdi
    mov rdx, [rbp+128h] ; native RSP+118
    mov r8, r14
    call qword ptr [stormbolt_growth_dispatch_target]
stormbolt_growth_post_effect_entry_skip:
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
    test esi, esi
    jz stormbolt_growth_post_effect_entry_branch
    cmp esi, 64h
    jne stormbolt_growth_post_effect_entry_done
stormbolt_growth_post_effect_entry_branch:
    mov rax, [rbp+8]
    lea rax, [rax+07Ah-returnSkip]
    mov [rbp+8], rax
    mov rax, [rsp+20h]
stormbolt_growth_post_effect_entry_done:
    lea rsp, [rbp]
    pop rbp
    ret
entryName ENDP
ENDM
PUBLIC stormbolt_growth_post_effect_entry
PUBLIC stormbolt_growth_post_effect_entry_span
GROWTH_POST_EFFECT stormbolt_growth_post_effect_entry, 0
GROWTH_POST_EFFECT stormbolt_growth_post_effect_entry_span, 4

GROWTH_REMAINING MACRO entryName, returnSkip
    LOCAL stormbolt_growth_remaining_entry_skip, stormbolt_growth_remaining_entry_branch, stormbolt_growth_remaining_entry_done
entryName PROC FRAME
    push rbp
    .pushreg rbp
    lea rsp, [rsp-0D0h]
    .allocstack 0D0h
    lea rbp, [rsp+0D0h]
    .setframe rbp, 0D0h
    .endprolog
    mov [rsp+20h], rax
IF returnSkip
    mov rax, [rbp+8]
    lea rax, [rax+returnSkip]
    mov [rbp+8], rax
ENDIF
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
    mov rcx, rdi
    mov rdx, [rbp+128h] ; native RSP+118
    mov r8, r14
    call qword ptr [stormbolt_growth_dispatch_target]
stormbolt_growth_remaining_entry_skip:
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
    test esi, esi
    js stormbolt_growth_remaining_entry_branch
    cmp byte ptr [rbp+138h], 0 ; native RSP+128
    jmp stormbolt_growth_remaining_entry_done
stormbolt_growth_remaining_entry_branch:
    mov rax, [rbp+8]
    lea rax, [rax+071h-returnSkip]
    mov [rbp+8], rax
    mov rax, [rsp+20h]
stormbolt_growth_remaining_entry_done:
    lea rsp, [rbp]
    pop rbp
    ret
entryName ENDP
ENDM
PUBLIC stormbolt_growth_remaining_entry
PUBLIC stormbolt_growth_remaining_entry_span
GROWTH_REMAINING stormbolt_growth_remaining_entry, 0
GROWTH_REMAINING stormbolt_growth_remaining_entry_span, 7

END
