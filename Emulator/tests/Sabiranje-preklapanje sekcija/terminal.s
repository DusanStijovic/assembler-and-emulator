.section terminal:

.extern readFromTerminal

cmp readFromTerminal, $0
jeq kraj

.equ DATA_IN, 0xFF02
mov $0, %r2
mov $0, readFromTerminal
mov DATA_IN, %r2l

kraj: 
iret

.end



