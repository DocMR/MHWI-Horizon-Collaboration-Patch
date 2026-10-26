; Entered by the existing owned replay CALL at image+1F6797B.
; Eight replaced bytes: CMP EAX,5 / JE +8 / ADD EAX,-28.
; Return initially points to image+1F67980. Other registers are untouched.
; Native image+1F67988 sets R12B=1 and copies XMM7 into XMM6.
.code
PUBLIC stormbolt_elemental_class48_entry
stormbolt_elemental_class48_entry PROC
    cmp eax, 48
    je selected
    cmp eax, 5
    je selected
    sub eax, 28
    ; Continue at the original CMP EAX,4, retaining its unsigned range gate.
    add qword ptr [rsp], 3
    ret
selected:
    add qword ptr [rsp], 8
    ret
stormbolt_elemental_class48_entry ENDP
; Native elemental skill calculation at 11BA5DB. PS 2C5FEFB accepts
; both 31 and 48: this flag selects DD parameter1 and the ammo cap table.
PUBLIC stormbolt_elemental_ammo_class48_entry
stormbolt_elemental_ammo_class48_entry PROC
    cmp eax, 31
    je ammo_selected
    cmp eax, 48
    je ammo_selected
    add qword ptr [rsp], 076h
    ret
ammo_selected:
    add qword ptr [rsp], 3
    test rsi, rsi
    ret
stormbolt_elemental_ammo_class48_entry ENDP
END
