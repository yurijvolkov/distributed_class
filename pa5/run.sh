export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/pa5/lib64"

clang -std=c99 -Wall -pedantic -g -L ./lib64 -lruntime *.c

LD_PRELOAD=/home/yvolkov/IFMO/Distributed/pa4/lib64/libruntime.so  ./a.out -p 3 --mutexl
