.section timer:

.extern timerSet 
.equ DATA_OUT, 0xFF00

.equ too_much_time, 9

.equ newTimer, 0 - timerSet
.global newTimer

cmp newTimer, $0
jeq *podloga(%pc)

mov newTimer(%pc), %r1 
shl $1, %r1
mov %r1, newTimer(%pc)

cmp $too_much_time, %r1 
jgt zaustaviProcesor
jmp kraj

podloga: 
.word kraj


zaustaviProcesor:
add newTimer, vrednost(%pc)
add josmalo(%pc), vrednost
mov vrednost(%pc), josmalo(%pc)
sub $6, josmalo(%pc)
mov josmalo, DATA_OUT 
halt



kraj:
iret

josmalo: .word 20

.section dodati:
.skip 260
vrednost: .byte 33


.end
