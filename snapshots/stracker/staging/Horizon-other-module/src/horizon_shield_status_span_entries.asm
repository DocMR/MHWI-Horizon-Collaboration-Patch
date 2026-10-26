; Shared replay publication changes CALL5 only, leaving original tail intact.
; Both existing replay entries reproduce the full7-byte span. Skip the2-byte
; native tail exactly once without changing registers or incoming flags.
EXTERN stormbolt_status_update_entry:PROC
EXTERN stormbolt_timer_update_entry:PROC
PUBLIC horizon_status_update_span_entry
PUBLIC horizon_timer_update_span_entry
.code
horizon_status_update_span_entry PROC FRAME
    push rax
    .allocstack 8
    .endprolog
    mov rax, [rsp+8]
    lea rax, [rax+2]
    mov [rsp+8], rax
    pop rax
    jmp stormbolt_status_update_entry
horizon_status_update_span_entry ENDP
horizon_timer_update_span_entry PROC FRAME
    push rax
    .allocstack 8
    .endprolog
    mov rax, [rsp+8]
    lea rax, [rax+2]
    mov [rsp+8], rax
    pop rax
    jmp stormbolt_timer_update_entry
horizon_timer_update_span_entry ENDP
EXTERN horizon_shield_multiplier_entry:PROC
PUBLIC horizon_shield_multiplier_span_entry
horizon_shield_multiplier_span_entry PROC FRAME
    push rax
    .allocstack 8
    .endprolog
    mov rax, [rsp+8]
    lea rax, [rax+2]
    mov [rsp+8], rax
    pop rax
    jmp horizon_shield_multiplier_entry
horizon_shield_multiplier_span_entry ENDP
EXTERN horizon_shield_reaction_entry:PROC
PUBLIC horizon_shield_reaction_span_entry
horizon_shield_reaction_span_entry PROC FRAME
    push rax
    .allocstack 8
    .endprolog
    mov rax, [rsp+8]
    lea rax, [rax+1]
    mov [rsp+8], rax
    pop rax
    jmp horizon_shield_reaction_entry
horizon_shield_reaction_span_entry ENDP
END
