.code
PUBLIC stormbolt_itemwindow_mode_entry
; Native site1E2D303: CMP DWORD PTR[R8+9F8],0C; JNE1E2D354.
; CALL return=site+5. Preserve the original CMP flags for every ordinary path.
; Target LBG selects AL from ammo40 and rejoins at1E2D320, before the original
; menu/context suppressions, change detection, layout refresh and animations.
stormbolt_itemwindow_mode_entry PROC
    cmp dword ptr [r8+9F8h],0Dh
    jne ordinary_mode
    cmp dword ptr [r8+9FCh],0EEh
    je thunder_mode
    cmp dword ptr [r8+9FCh],0EFh
    jne ordinary_mode
thunder_mode:
    add qword ptr [rsp],18h ; site+1D=1E2D320
    cmp dword ptr [r8+27C8h],28h
    sete al
    ret
ordinary_mode:
    add qword ptr [rsp],3 ; replay8 bytes, resume original JNE
    cmp dword ptr [r8+9F8h],0Ch
    ret
stormbolt_itemwindow_mode_entry ENDP
END
