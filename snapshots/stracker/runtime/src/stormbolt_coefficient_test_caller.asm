PUBLIC stormbolt_test_coefficient_call
PUBLIC stormbolt_test_coefficient_return
.code
stormbolt_test_coefficient_call PROC FRAME
    sub rsp, 28h
    .allocstack 28h
    .endprolog
    mov rax, qword ptr [rcx]
    call qword ptr [rax + 3D8h]
stormbolt_test_coefficient_return LABEL BYTE
    add rsp, 28h
    ret
stormbolt_test_coefficient_call ENDP
END
