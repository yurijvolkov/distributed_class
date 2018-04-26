#include "ipc.h"
#include "unistd.h"
#include "utils.h"

#include <stdio.h>

int send(void* ipd, local_id dst, const Message* msg) {
    IPC* ipc = ipd;
    write(ipc -> descs[ipc -> worker_id][dst][WRITE_DESC], 
          msg, 
          sizeof(MessageHeader) + msg->s_header.s_payload_len);

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
    if(ipc -> worker_id == from)
        return -1;
    int r = read(ipc -> descs[from][ipc -> worker_id][READ_DESC],
                 &msg->s_header, 
                 sizeof(MessageHeader));
    if( r <= 0 )
        return -1;

    r = read(ipc -> descs[from][ipc -> worker_id][READ_DESC],
             &msg->s_payload,
             msg->s_header.s_payload_len);

    return 0; 
}

int receive_any(void* ipd, Message* msg) {
    IPC* ipc = ipd;
    while(1) {
        for(int i = 0; i < ipc -> num_workers + 1; i++) {
            int r = receive(ipd, i, msg);
            if (r == 0)
                return i;
        }
    }
}
