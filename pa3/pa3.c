#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "ipc.h"
#include "pa1.h"
#include "utils.h"
#include "common.h"
#include "banking.h"
#include "lamport.h"

#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>


int wait_msgs(IPC* ipc) {
    Message msg ;
    
    while(1) {
        receive_any(ipc, &msg);

        TransferOrder* t = (TransferOrder*)msg.s_payload;

        switch (msg.s_header.s_type) {
            case TRANSFER : {
                set_lamport(msg.s_header.s_local_time);
                int pending = t -> s_amount > 0 ? t -> s_amount : 0;
                set_new_balance(&ipc->balance_history, t->s_amount, msg.s_header.s_local_time + 1, pending );

                if(t -> s_amount > 0) {
                    transfer(ipc, t->s_dst, ipc->worker_id, -(t->s_amount));
                }
                continue;
            }
            case STOP : {
                return 0;
            }
        } 
        
    }
}

int work(IPC* ipc) {
    close_unused_pipes(ipc);
    fprintf(ipc -> event_log, log_started_fmt, ipc -> worker_id,
            getpid(), getppid());
    sync_workers(ipc);
    
    fprintf(ipc -> event_log, log_received_all_started_fmt, ipc -> worker_id);

    wait_msgs(ipc);

    fprintf(ipc -> event_log, log_done_fmt, ipc -> worker_id);

    send_history(ipc);
//    sync_workers(ipc);

    fprintf(ipc -> event_log, log_received_all_done_fmt, ipc -> worker_id);

    return 0;
}

int main(int argc, char* argv[]){
    int num_process;
    get_options(argc, argv, &num_process);
    IPC* ipc = calloc(1, sizeof(IPC));
    AllHistory history = {.s_history_len = num_process};

    for(int i = 0; i < num_process; i++) {
        BalanceHistory h = {.s_id = i+1,
                            .s_history_len = 0 };
        history.s_history[i] = h;
    }

    ipc -> num_workers = num_process;

    init_pipes(ipc);
    init_logs(ipc);
    
    for(int i = 0; i < num_process; i++) {
        int pid = fork();
        if(pid == 0){
            ipc -> worker_id = i + 1;
            ipc -> balance_history = history.s_history[i];
            work(ipc);
            history.s_history[i] = ipc -> balance_history;
            exit(0);
        }
    }
    
    ipc -> worker_id = 0;
    close_unused_pipes(ipc);

    bank_robbery(ipc, num_process);

    sleep(1);
    send_stop(ipc);
    get_balances(ipc, &history);


    for(int i = 0; i < num_process; i++) 
        wait(NULL);

//Whole trash happens downwards
    int maxtime = -1;

    for(int i = 0; i < num_process; i++){
        BalanceHistory cur = history.s_history[i];
        int len = cur.s_history_len;
        for(int j = 0; j < len; j++){
            if(maxtime < cur.s_history[j].s_time)
                maxtime = cur.s_history[j].s_time;
        }
    }

    AllHistory final_history = {.s_history_len = num_process};

    for(int i = 0; i < num_process; i++) {
        BalanceHistory h = {.s_id = i+1,
                            .s_history_len = maxtime + 1 };
        for(int j = 0; j < maxtime+1; j++) {
            BalanceState s = { .s_balance = atoi(argv[i + 3]),
                               .s_time = j };
            h.s_history[j] = s;
        }
            
        final_history.s_history[i] = h;
    }

   for(int i = 0; i < num_process; i++){
        BalanceHistory cur = history.s_history[i];
        int len = cur.s_history_len;

        for(int j = 0; j < len; j++){
            for(int k = cur.s_history[j].s_time; k <= maxtime; k++)
                final_history.s_history[i].s_history[k].s_balance -= cur.s_history[j].s_balance;
            for(int k = 0; k < 2; k++)
             final_history.s_history[i].s_history[cur.s_history[j].s_time + k].s_balance +=
                  cur.s_history[j].s_balance_pending_in;
        }
    }

    print_history(&final_history);

}
