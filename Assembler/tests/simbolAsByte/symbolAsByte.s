.section symbolAsByte:
.equ a, 600 + b
.equ a1, -60 + c

.global a1

.extern c


.skip 9
b:

cmp $b, $a
cmpb $b, $a1 


.end

