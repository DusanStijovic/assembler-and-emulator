.extern _start
.section startRoutine:
or $0x2000, %psw
jmp _start
.end