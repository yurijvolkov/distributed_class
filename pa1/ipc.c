#include "ipc.h"
#include "utils.h"

#include <stdio.h>

int send(void* ipd, local_id dst, const Message* msg) {
    IPC* ipc = ipd;
    write(ipc -> descs[ipc -> worker_id][dst][WRITE_DESC], 
          msg, 
          sizeof(msg));

    return 0;
}

int send_multicast(void* ipd, const Message* msg) {
    IPC* ipc = ipd;
    for(int i = 0; i < ipc -> num_workers + 1; i++) {
        if(i == ipc->worker_id)
            continue;
        send(ipd, i, msg);
    }

    return 0;
}

int receive(void* ipd, local_id from, Message* msg) {
    IPC* ipc = ipd;
    int r = read(ipc -> descs[from][ipc -> worker_id][READ_DESC],
                 msg, 
                 sizeof(msg));

    return r > 0 ? 0 : -1;
}

int receive_any(void* ipd, Message* msg) {
    return -1;
}
