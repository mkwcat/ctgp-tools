


.globl call_original_fopen
call_original_fopen:

lis r9,fopen_thing_ha@ha
lhz r9,fopen_thing_ha@l(r9)
slwi r9,r9,16
lis r5,fopen@ha
lwz r5,fopen@l(r5)
addi r5,r5,4
mtctr r5
bctr


.globl call_fwrite
call_fwrite:

lis r9,fwrite@ha
lwz r9,fwrite@l(r9)
mtctr r9
mr r6,r3
mr r3,r4
mr r4,r5
li r5,1
bctr







