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

int is_stopped = 0;
int is_sync = 0;
int num_process = 0;
int stop_counter = 0;

int wait_msgs(IPC* ipc) {
    Message msg ;
    
    while(1) {
        if(stop_counter == 0)
            return 0;
        local_id from = receive_any(ipc, &msg);
        switch (msg.s_header.s_type) {
            case CS_REQUEST : {

                if( (msg.s_header.s_local_time < get_lamport_time()) ||
                        (msg.s_header.s_local_time == get_lamport_time() &&
                         from < ipc -> worker_id) || is_stopped)
                    send_reply(ipc, from);
                else
                    set_dr(ipc, from);
                break;
            }

            case CS_REPLY : {
                set_received(ipc, from);

                if(check_is_received_all(ipc))
                    return 0;
                break;
            }

            case ACK : {   
                stop_counter--;
                if(stop_counter == 0)
                    return 0;
            }
        } 

    }

}

int request_cs(const void* ipd) {
    IPC* ipc = (IPC*)ipd;
    inc_lamport();

    MessageHeader header = { .s_magic = MESSAGE_MAGIC,
                             .s_payload_len = 0,
                             .s_type = CS_REQUEST,
                             .s_local_time = get_lamport_time() };
    Message msg = { .s_header = header };
    send_multicast(ipc, &msg);
    wait_msgs(ipc);

    return 0;
}

int release_cs(const void* ipd) {
    IPC* ipc = (IPC*)ipd;
    inc_lamport();
 
    for(int i = 0; i < ipc -> num_workers; i++){
        if(ipc -> ra.deferred_reply[i])
            send_reply(ipc, i + 1);
    } 

    flush(ipc);
    return 0;
}

int work(IPC* ipc) {
    close_unused_pipes(ipc);
    int num_iterations = 5 * (ipc->worker_id);

    for(int i = 0; i < num_iterations; i++) {
        if(is_sync)
            request_cs(ipc);
        size_t len = snprintf(NULL,0, log_loop_operation_fmt,
                              ipc -> worker_id, i+1, num_iterations);
        char* s = (char*)alloca(len+1);
        snprintf(s, len+1, log_loop_operation_fmt, ipc->worker_id, i+1, num_iterations);
        print(s);
        if(is_sync)
            release_cs(ipc);
    }

    is_stopped = 1;
    send_stop(ipc);
    wait_msgs(ipc);

    return 0;
}

static inline int get_options(int argc, char* argv[]){
    int opt_id = 0;
    int c;
    static struct option opts[] = {
        {"mutexl", no_argument, 0, 'm'},
        {"proc", required_argument, 0, 'p'},
        {NULL, 0, 0, 0}
    };

    while((c = getopt_long(argc, argv, "p:m",
                    opts, &opt_id)) > 0) {
        switch(c) {
            case 'm': {
                is_sync = 1;
                break;
            }
            case 'p': {
                num_process = atoi(optarg);
                break;
            }
        }
    }

    return 0;
}

int main(int argc, char* argv[]){
    get_options(argc, argv);

    stop_counter = num_process - 1;

    IPC* ipc = calloc(1, sizeof(IPC));
    ipc -> num_workers = num_process;
    init_pipes(ipc);
    init_logs(ipc);
    for(int i = 0; i < num_process; i++) {
        int pid = fork();
        if(pid == 0){
            ipc -> worker_id = i + 1;
            (ipc -> queue).length = 0;
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
    
    return 0;
}
