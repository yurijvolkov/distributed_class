#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <alloca.h>
#include <getopt.h>

#include "ipc.h"
#include "pa2345.h"
#include "common.h"
#include "utils.h"
#include "banking.h"
#include "lamport.h"

#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>

int init_pipes(IPC* ipc) {
    for(int i = 0; i < ipc->num_workers + 1; i++)
        for(int j = 0; j < ipc->num_workers + 1; j++) {
            if(i == j)
                continue;
            pipe2(ipc -> descs[i][j], O_NONBLOCK   );

        }
    return 0;
}

int init_logs(IPC* ipc){
    ipc -> event_log = fopen(events_log, "w");

    return 0;
}

int sync_workers(IPC* ipc) {
    inc_lamport();

    MessageHeader header = { .s_magic = MESSAGE_MAGIC,
                             .s_payload_len = 0,
                             .s_type = STARTED,
                             .s_local_time = get_lamport_time() };
    Message msg = { .s_header = header };
 
    send_multicast(ipc, &msg);
    for(int i = 1; i < ipc->num_workers + 1; i++){
        if(i == ipc->worker_id)
            continue;
        while(1){
            if (receive((void*)ipc, i, &msg) == 0) {
                    break;
            }
        }
    }

    return 0;
}

int close_unused_pipes(IPC* ipc){ 
    for(int i = 0; i < ipc-> num_workers + 1; i++)
        for(int j = 0; j < ipc->num_workers + 1; j++) {
            if(i == j)
                continue;
            if( (ipc->worker_id != i) && (ipc->worker_id != j) ) {
                close(ipc->descs[i][j][READ_DESC]);
                close(ipc->descs[i][j][WRITE_DESC]);
            }
            else if( ipc->worker_id != i)
                close(ipc->descs[i][j][WRITE_DESC]);
            else if( ipc->worker_id != j)
                close(ipc->descs[i][j][READ_DESC]);
        }
    return 0;
}

int send_stop(void* ipc) {
    inc_lamport();

    MessageHeader header = { .s_magic = MESSAGE_MAGIC,
                             .s_payload_len = 0,
                             .s_type = ACK,
                             .s_local_time = get_lamport_time() };
    Message msg = { .s_header = header };
    send_multicast(ipc, &msg);

    return 0;
}

int send_reply(void* ipc, local_id to) {

    MessageHeader header = { .s_magic = MESSAGE_MAGIC,
                             .s_payload_len = 0,
                             .s_type = CS_REPLY,
                             .s_local_time = get_lamport_time() };
    Message msg = { .s_header = header };
 
    send(ipc, to, &msg);

    return 0;
}

