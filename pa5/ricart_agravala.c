#include "utils.h"

int flush(IPC* ipc){
    for(int i = 0; i < ipc -> num_workers; i++) {
        ipc -> ra.deferred_reply[i] = 0;
        ipc -> ra.received_reply[i] = 0;
    }

    return 0;
}

int set_dr(IPC* ipc, int worker_id){
    ipc -> ra.deferred_reply[worker_id - 1] = 1;

    return 0;
}

int set_received(IPC* ipc, int worker_id){
    ipc -> ra.received_reply[worker_id - 1] = 1;

    return 0;
}

int check_is_received_all(IPC* ipc) {
    for(int i = 0; i < ipc -> num_workers; i++)
        if( (ipc -> ra.received_reply[i] == 0) && (i != ipc -> worker_id - 1))
            return 0;
    return 1;
}
