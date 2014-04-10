neg_off:
        .word 0x123

.globl _start
_start:
        mov r0, #0x31
        mov r1, #0x80000001
        mov r2, #1
        eor r3, r0, r1, lsl r2
        eor r4, r0, r1, lsr r2
        eor r5, r0, r1, asr r2
        eor r6, r0, r1, ror r2
        /*mov r2, r1, lsl r0*/
        ldr r7, pos_off
        ldr r8, neg_off
        add pc, #-4
        mov r2, #0
        nop
        mov r2, #0
pos_off:
        .word 0x100
