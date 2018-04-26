#include "ipc.h"
#include "pa1.h"
#include "utils.h"
#include "common.h"

#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


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
    int num_process;
    IPC* ipc = calloc(1, sizeof(IPC));

    get_options(argc, argv, &num_process);
    
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
        wait(NULL);
    }

}
