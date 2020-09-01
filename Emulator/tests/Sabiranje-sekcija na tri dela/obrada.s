.section obrada:

.extern podaci
.extern brojPodataka
.equ koliko, 0 - brojPodataka + 12 + 10
.global koliko


.equ DATA_OUT, 0xFF00
.global _start

inicijalizuj:
.equ readFromTerminal, 6000
.equ timerSet1, 6001
.equ timer_cfg, 0xFF10 

mov $0x1, timer_cfg
mov $1, readFromTerminal(%pc)
mov $1, timerSet1

.global readFromTerminal
.equ timerSet, -6001
.global timerSet
ret


.equ nesto, 60
mov $nesto, %r1l

_start:
mov $0, %r3
mov $0, %r4
call inicijalizuj
sabiraj: cmp $koliko, %r3l
jgt kraj
mov podaci(%r3), %r5l
add $1, %r3
mov podaci(%r3), %r5h
add %r5, %r4 

jmp continue
.extern continue 
.extern kraj
.global sabiraj
.end
