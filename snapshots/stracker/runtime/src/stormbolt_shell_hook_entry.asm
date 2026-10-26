; Windows x64 wrapper hook. Only the verified caller assigns meaning to R13/R14.
; Capture them without modifying caller-owned nonvolatile registers. The C++
; dispatcher rejects other return addresses before interpreting these values.
EXTERN stormbolt_shell_dispatch_target:QWORD
PUBLIC stormbolt_shell_hook_entry
.code
stormbolt_shell_hook_entry PROC FRAME
    sub rsp, 38h
    .allocstack 38h
    .endprolog
    mov rax, qword ptr [rsp + 38h]
    mov qword ptr [rsp + 20h], rax
    mov qword ptr [rsp + 28h], r13
    mov dword ptr [rsp + 30h], r14d
    call qword ptr [stormbolt_shell_dispatch_target]
    add rsp, 38h
    ret
stormbolt_shell_hook_entry ENDP
END
