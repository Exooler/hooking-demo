; Windows x64: first integer argument = ECX, return value = EAX.
; A known 14-byte patch area keeps this demo independent of a disassembler.
.code
PUBLIC Calculate
Calculate PROC
    REPT 14
        nop
    ENDM
    lea eax, [rcx+1]
    ret
Calculate ENDP
END
