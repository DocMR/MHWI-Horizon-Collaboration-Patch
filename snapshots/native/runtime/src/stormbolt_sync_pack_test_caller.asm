PUBLIC stormbolt_sync_pack_test_call
PUBLIC stormbolt_sync_pack_test_patch_site
PUBLIC stormbolt_sync_pack_test_dispatch
EXTERN stormbolt_sync_pack_test_frame:QWORD
EXTERN stormbolt_sync_pack_test_seen_frame:QWORD
EXTERN stormbolt_sync_pack_test_seen_controller:QWORD
EXTERN stormbolt_sync_pack_test_seen_word:DWORD
.code
stormbolt_sync_pack_test_dispatch PROC
    mov stormbolt_sync_pack_test_seen_controller, rcx
    mov stormbolt_sync_pack_test_seen_frame, rdx
    mov stormbolt_sync_pack_test_seen_word, r8d
    xor ecx, ecx
    xor edx, edx
    xor r8d, r8d
    xor r9d, r9d
    xor r10d, r10d
    xor r11d, r11d
    pxor xmm0, xmm0
    mov eax, 0AABBCCDDh
    ret
stormbolt_sync_pack_test_dispatch ENDP
stormbolt_sync_pack_test_call PROC FRAME
    push rbp
    .pushreg rbp
    push rdi
    .pushreg rdi
    push rsi
    .pushreg rsi
    push rbx
    .pushreg rbx
    sub rsp, 0E8h
    .allocstack 0E8h
    lea rbp, [rsp+80h]
    .setframe rbp,80h
    .endprolog
    mov rsi, rcx
    mov rbx, rdx
    mov edi, 11h
    mov rax, 1234h
    mov r8, 2345h
    mov r9, 4567h
    pcmpeqd xmm0, xmm0
    mov stormbolt_sync_pack_test_frame, rsp
stormbolt_sync_pack_test_patch_site LABEL BYTE
    mov dword ptr [rbp-59h], edi
    sub r8, rax
    mov [rbx], rdi
    mov [rbx+8], rax
    mov [rbx+10h], r8
    mov [rbx+18h], r9
    mov eax, dword ptr [rbp-59h]
    mov [rbx+20h], rax
    movdqu [rbx+28h], xmm0
    lea rsp, [rbp+68h]
    pop rbx
    pop rsi
    pop rdi
    pop rbp
    ret
stormbolt_sync_pack_test_call ENDP
END
