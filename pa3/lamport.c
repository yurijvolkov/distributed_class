#include "ipc.h"

int lamport_time = 0;

int inc_lamport(){
    lamport_time += 1;

    return 0;
}

timestamp_t get_lamport_time() {
    return lamport_time;
}

int set_lamport(int val) {
    lamport_time = lamport_time > val ? lamport_time : val;
    lamport_time++;

    return 0;
}

