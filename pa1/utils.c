#include "ipc.h"
#include "pa1.h"
#include "utils.h"
#include "common.h"

#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>


int init_pipes(IPC* ipc) {
    for(int i = 0; i < ipc->num_workers + 1; i++)
        for(int j = 0; j < ipc->num_workers + 1; j++) {
            if(i == j)
                continue;
            pipe(ipc -> descs[i][j]);
        }

    return 0;
}

int init_logs(IPC* ipc) {
    ipc -> event_log = fopen(events_log, "w"); 

    return 0;
}

int sync_workers(IPC* ipc) {
    MessageHeader header = { .s_magic = MESSAGE_MAGIC,
                             .s_payload_len = 48,
                             .s_type = STARTED };
    Message msg = { .s_header = header };

    send_multicast(ipc, &msg);
    for(int i = 1; i < ipc->num_workers + 1; i++){
        if(i == ipc->worker_id)
            continue;
        while(1){
            if (receive((void*)ipc, i, &msg) == 0) 
                break;
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

int get_options(int argc, char** argv, int* num_process){
    int opt_id = 0;
    int c;
    static struct option opts[] = {
        {"proc", required_argument, 0, 'p'},
        {NULL, 0, 0, 0}
    };

    while((c = getopt_long(argc, argv, "p:",
                    opts, &opt_id)) > 0) {
        switch(c) {
            case 'p': {
                *num_process = atoi(optarg);
                break;
            }
        }
    }

    return 0;
}
