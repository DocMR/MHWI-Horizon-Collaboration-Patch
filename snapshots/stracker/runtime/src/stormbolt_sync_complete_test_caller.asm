PUBLIC stormbolt_sync_complete_test_call
PUBLIC stormbolt_sync_complete_test_patch_site
PUBLIC stormbolt_sync_complete_test_dispatch
EXTERN stormbolt_sync_complete_test_frame:QWORD
EXTERN stormbolt_sync_complete_test_seen_frame:QWORD
EXTERN stormbolt_remote_test_dispatch:PROC
.code
stormbolt_sync_complete_test_dispatch PROC
    mov stormbolt_sync_complete_test_seen_frame, rdx
    jmp stormbolt_remote_test_dispatch
stormbolt_sync_complete_test_dispatch ENDP
stormbolt_sync_complete_test_call PROC FRAME
    push rsi
    .pushreg rsi
    push rbx
    .pushreg rbx
    push r13
    .pushreg r13
    sub rsp, 0D0h
    .allocstack 0D0h
    .endprolog
    mov rsi, rcx
    mov rbx, rdx
    mov qword ptr [rsp+0B8h], 12345678h
    mov r13, -1
    mov stormbolt_sync_complete_test_frame, rsp
    stc
stormbolt_sync_complete_test_patch_site LABEL BYTE
    mov r13, qword ptr [rsp+0B8h]
    mov [rbx], r13
    pushfq
    pop rax
    mov [rbx+8], rax
    add rsp, 0D0h
    pop r13
    pop rbx
    pop rsi
    ret
stormbolt_sync_complete_test_call ENDP
END
