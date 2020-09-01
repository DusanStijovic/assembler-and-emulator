.section proba:
.equ a, 5
push $a
mov $4, a
int a(%r1)
int a(%pc)

jmp a
jmp *a
jmp *a(%r1)
jmp *a(%pc)

.skip 9
c:


.section proba2:
.equ b, a
push $b
mov $4, b
int b(%r1)
int b(%pc)


jmp b
jmp *b
jmp *b(%r1)
jmp *b(%pc)

.section proba3:
.equ r, c
push $r
mov $4, r
int r(%r1)
int r(%pc)

jmp r
jmp *r
jmp *r(%r1)
jmp *r(%pc)

.section proba4: 
.skip 10
w:
.equ k, w
push $k
mov $4, k
int k(%r1)
int k(%pc)

jmp k
jmp *k
jmp *k(%r1)
jmp *k(%pc)

.section proba5:

.equ e, 2-n
push $e
mov $4, e
int e(%r1)
int e(%pc)

jmp e
jmp *e 
jmp *e(%r1)
jmp *e(%pc)

.extern n
.end