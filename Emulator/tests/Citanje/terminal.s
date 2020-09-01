.section terminal:

.extern readFromTerminal
.extern broj
.extern podaci

cmp readFromTerminal, $0
jeq kraj

cmp $broj, %r2
jeq stopReading
mov DATA_IN, podaci(%r2)
add $1, %r2
cmp $broj, %r2
jeq stopReading
jmp kraj

.equ DATA_IN, 0xFF02
stopReading:
mov $0, readFromTerminal
kraj: 
iret
.end



