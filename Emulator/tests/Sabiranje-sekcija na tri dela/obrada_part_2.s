.section obrada:

.extern podaci
.extern brojPodataka
.equ koliko, 0 - brojPodataka + 12 + 10



.equ DATA_OUT, 0xFF00
.extern _start
.extern sabiraj
.extern readFromTerminal
.global continue

jmp _start
continue:
mov %r4h, %r5h
mov %r4l, %r5l
mov %r5h, %r4h 
mov %r5l, %r4l

add $1, %r3
jmp sabiraj




finish:mov %r4, DATA_OUT
halt
.global finish 
.end
