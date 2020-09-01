.section proba:

.equ a, 5
.equ b, 5
.global b
.byte a
.byte b 
.skip 10
c:


.section proba1:
.equ aa, a
.equ bb, b
.global bb 
.byte aa 
.byte bb

.equ rrr, 0 - bb

.section proba2:
.equ k, c + a - 2
.equ e, c + a  - 2
.global e
.byte k
.byte e


.section proba3:
.equ r, 4 - n
.equ q, 4 - n
.global q
.extern n
.byte r
.byte q


.end