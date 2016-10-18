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
MEM value
IN index
OUT value
EXIT value
NOP
NOP index
SET index value
CPY index index
ADD index index index
SUB index index index
MUL index index index
DIV index index index
MOD index index index
INC index
DEC index
NEC index
AND index
OR index
XOR index
NOT index
SHL index value
SHR index value
ROL index value
ROR index value
EQU index index index
GTER index index index
LESS index index index
LEQ index index index
REQ index index index
JMP value
JMOV value
JIF value value
JIFM valule value
# comments
```
