; Synthetic native caller with the same argument/register contract as 214C5B9.
EXTERN stormbolt_test_create:PROC
EXTERN stormbolt_test_registers_intact:DWORD
PUBLIC stormbolt_test_shoot
PUBLIC stormbolt_test_shoot_return
.code
stormbolt_test_shoot PROC FRAME
    push r13
    .pushreg r13
    push r14
    .pushreg r14
    sub rsp, 28h
    .allocstack 28h
    .endprolog
    mov r13, qword ptr [rsp + 60h]
    mov r14d, dword ptr [rsp + 68h]
    call stormbolt_test_create
stormbolt_test_shoot_return LABEL BYTE
    mov r10d, 0
    cmp r13, qword ptr [rsp + 60h]
    jne checked_registers
    cmp r14d, dword ptr [rsp + 68h]
    jne checked_registers
    mov r10d, 1
checked_registers:
    mov dword ptr [stormbolt_test_registers_intact], r10d
    add rsp, 28h
    pop r14
    pop r13
    ret
stormbolt_test_shoot ENDP
END
