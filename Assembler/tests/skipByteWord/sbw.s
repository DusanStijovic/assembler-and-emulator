.section proba:

.skip 10

a: .byte a
   .word b

.skip 30 

b: 
.global a
.section proba2:


.byte c
.word c
.extern c
.byte c 
.word c


.end
