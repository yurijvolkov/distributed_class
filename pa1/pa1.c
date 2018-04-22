#include "ipc.h"
#include "pa1.h"
#include "utils.h"
#include "common.h"

#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


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
    Message* msg = calloc(1, sizeof(Message));

    send_multicast(ipc, msg);
    for(int i = 1; i < ipc->num_workers + 1; i++){
        if(i == ipc->worker_id)
            continue;
        while(1){
            if (receive((void*)ipc, i, msg) == 0) {
                //printf("Received %d %d\n", ipc->worker_id, i);
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

int work(IPC* ipc) {
    close_unused_pipes(ipc);

    fprintf(ipc -> event_log, log_started_fmt, ipc -> worker_id,
            getpid(), getppid());
    sync_workers(ipc);
    
    fprintf(ipc -> event_log, log_received_all_started_fmt, ipc -> worker_id);

    fprintf(ipc -> event_log, log_done_fmt, ipc -> worker_id);

    sync_workers(ipc);

    fprintf(ipc -> event_log, log_received_all_done_fmt, ipc -> worker_id);

    return 0;
}

int main(int argc, char* argv[]){
    int num_process = atoi(argv[2]);
    
    IPC* ipc = calloc(1, sizeof(IPC));
    ipc -> num_workers = num_process;
    init_pipes(ipc);
    init_logs(ipc);
    for(int i = 0; i < num_process; i++) {
        int pid = fork();
        if(pid == 0){
            ipc -> worker_id = i + 1;
            work(ipc);
            exit(0);
        }
    }
    ipc -> worker_id = 0;
    close_unused_pipes(ipc);

    for(int i = 0; i < num_process; i++) {
        int t;
        wait(&t);
    }

}
