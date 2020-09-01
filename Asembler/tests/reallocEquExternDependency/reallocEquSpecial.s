.section proba:
.equ a,5
.skip 9
c:

.section proba2:
.equ b, a

.section proba3:
.equ k, c + b + a - 1

.section proba4:

.equ e, 0 - r 
.equ w, r 

.section probaa:
.equ aa, 5
.skip 9
cc:

.section probaa2:
.equ bb, aa


.section probaa3:
.equ kk, cc + bb + aa - 1

.section probaa4:
.equ ee, 0-r
.equ ww, r

.global aa,bb,kk,cc,ee,ww
.extern r

.end