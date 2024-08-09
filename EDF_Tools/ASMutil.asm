.data

.code

ASMReadInt32 proc

    	test edx, edx
        jne BEdata
        mov eax, [rcx]
        ret
    BEdata:
        movbe eax, [rcx]
        ret
        int 3

ASMReadInt32 ENDP

END