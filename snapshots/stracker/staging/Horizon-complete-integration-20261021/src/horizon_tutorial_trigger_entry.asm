; Original7B MOV at1F32167: only CALL5 is published. Return at1F3216E,
; replay RCX's original manager load, then let the native CALL execute once.
EXTERN horizon_tutorial_request_if_due:PROC
EXTERN horizon_tutorial_character_slot:QWORD
PUBLIC horizon_tutorial_trigger_entry
.code
horizon_tutorial_trigger_entry PROC FRAME
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
    mov rax, [rbp+8]
    lea rax, [rax+2]
    mov [rbp+8], rax
    call horizon_tutorial_request_if_due
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
    mov rcx, qword ptr [horizon_tutorial_character_slot]
    mov rcx, qword ptr [rcx]
    lea rsp, [rbp]
    pop rbp
    ret
horizon_tutorial_trigger_entry ENDP
END
