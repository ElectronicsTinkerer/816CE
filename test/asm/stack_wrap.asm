;;; TEST for the ZEDIAC system
;;; Returns in A the DBR after a COP/BRK in emulation mode
;;;
    
    .as

    .org 0
    lda #3                      ; Set the DBR to a known value
    pha
    plb
    .al
    rep #$20                    ; Save the stack so we can return to MONTIOR
    tsc
    sta >$20
    .as
    sec                         ; Emulation mode
    xce
    brk                         ; "Test subject"
    .byte 0

    ;; BRK/IRQ/COP vector
    .org $80                    
    pla                         ; Remove COP/BRK from stack
    pla
    pla
    phb                         ; Get the data bank
    plx
    clc                         ; Back to Native mode
    xce
    .al
    rep #$20
    lda >$20                    ; Restore stack
    tcs
    .as
    sep #$20
    txa
    rtl                         ; MONITOR prints A on return
    
