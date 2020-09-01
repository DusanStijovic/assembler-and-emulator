.section obrada:

.extern podaci
.extern brojPodataka
.equ koliko, 0 - brojPodataka + 12 + 10



.equ DATA_OUT, 0xFF00
.extern _start
.extern sabiraj
.extern readFromTerminal
.global kraj


jmp _start

kraj:
cekaj: 
cmp readFromTerminal, $1
jeq cekaj 
mul %r2, %r4
jmp *finish_address(%pc)
 
 finish_address:
 	.word finish
 .extern finish	
.end
