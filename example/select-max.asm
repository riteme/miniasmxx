MEM 1024

#
# Select maximum number
#

IN 1  # 1: n
SET 0 2  # 2: max
SET 0 3  # 3: i

NOP 499  # 499: begin

EQU *3 *1 4  # 4: i == n
JIF *4 *500

IN 5  # 5: v
GTER *5 *2 6  # 6: v > max
JIFM *6 2
JMOV 2
SET *5 2

INC *3 3
JMP *499

NOP 500  # 500: exit

OUT *2
