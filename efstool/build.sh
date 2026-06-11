cc -g -std=c99 ./*.c ../libefs/source/*.c -O3 -I ../libefs/include -Wpedantic -Wall -Wextra -Wreserved-identifier -Wtautological-unsigned-zero-compare -fsanitize=address,undefined
