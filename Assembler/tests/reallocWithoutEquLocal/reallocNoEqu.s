.section proba:

.skip 20 
a:

push $a
mov $4, a
int a(%r1)
int a(%pc)

jmp a
jmp *a
jmp *a(%r1)
jmp *a(%pc)

.section proba2:

push $a
mov $4, a
int a(%r1)
int a(%pc)

jmp a
jmp *a
jmp *a(%r1)
jmp *a(%pc)


.section proba3: 
int a(%pc)
jmp *a(%pc)

.section proba4:
.skip 9
b: int b(%pc)
jmp *b(%pc)
.end