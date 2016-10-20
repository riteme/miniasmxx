# miniasm++
The fake ASM language C++ version.

Just for fun.

## Supported syntax
`value`: A integer started with any number of `*`. A `*` means dereferencing once.
If marked as `index`, it means miniasm will access element at this index in memory.

```
value/index := [\*.][0-9]+
```

Instructions:

```
NOP
NOP index
MEM value
IN index
OUT value
SET value index
ADD value value index
SUB value value index
MUL value value index
DIV value value index
MOD value value index
INC value index
DEC value index
NEC value index
AND value value index
OR value value index
XOR value value index
FLIP value index
NOT value index
SHL value value index
SHR value value index
ROL value value index
ROR value value index
EQU value value index
GTER value value index
LESS value value index
GEQ value value index
LEQ value value index
JMP value
JMOV value
JIF value value
JIFM value value
# comments
```
