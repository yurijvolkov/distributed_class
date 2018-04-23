rm a.out
clang -std=c99 -Wall -pedantic -g -L ./lib64 -lruntime *.c

LD_PRELOAD=/home/yvolkov/IFMO/Distributed/pa2/lib64/libruntime.so  ./a.out -p 3 10 20 30
