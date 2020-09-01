.section obrada:
.extern podaci


.equ DATA_OUT, 0xFF00
.global _start

inicijalizuj:
.equ readFromTerminal, 6000
.equ timerSet1, 6001
.equ timer_cfg, 0xFF10 

mov $0, %r2 
mov $0x1, timer_cfg
mov $1, readFromTerminal(%pc)
mov $1, timerSet1

.global readFromTerminal
.equ timerSet, -6001
.global timerSet
ret


.equ broj, 3
.global broj

_start:

call inicijalizuj
cekaj: cmp readFromTerminal, $1
jeq cekaj 
mov $0, %r3
mov $0, %r4

ovde:
cmp %r3, $broj
jeq kraj 
add podaci(%r3), %r4l
add $1, %r3
jmp *loop(%pc)


kraj:mov %r4, DATA_OUT
halt
loop: .word ovde
.end
