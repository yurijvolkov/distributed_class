#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "ipc.h"
#include "pa1.h"
#include "utils.h"
#include "common.h"
#include "banking.h"

#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>


int init_pipes(IPC* ipc) {
    for(int i = 0; i < ipc->num_workers + 1; i++)
        for(int j = 0; j < ipc->num_workers + 1; j++) {
            if(i == j)
                continue;
            pipe2(ipc -> descs[i][j], O_NONBLOCK | O_DIRECT);

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
        //        if(msg->s_header.s_type == DONE)
                    break;
            }
        }
    }

    return 0;
}

int get_balances(IPC* ipc, AllHistory* all_history) {
    Message* msg = calloc(1, sizeof(Message));

    for(int i = 1; i < ipc->num_workers + 1; i++){ 
        while(1) {
            if(receive((void*)ipc, i, msg) == 0) {
                if(msg->s_header.s_type != BALANCE_HISTORY) {
                    continue;
                }
                BalanceHistory* history = (BalanceHistory*)msg->s_payload;
                all_history->s_history[i-1] = *history;
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



void transfer(void* ipd, local_id src, local_id dst, balance_t amount) {
    TransferOrder order = { .s_src = src,
                            .s_dst = dst,
                            .s_amount = amount };
    
    MessageHeader header = { .s_magic = MESSAGE_MAGIC,
                             .s_payload_len = sizeof(TransferOrder),
                             .s_type = TRANSFER,
                             .s_local_time = get_physical_time() };
    Message msg = { .s_header = header };
    memcpy(msg.s_payload, &order, sizeof(TransferOrder));

    send(ipd, src, &msg);
}

void transfer_time(void* ipd, local_id src, local_id dst, balance_t amount, int time) {
    TransferOrder order = { .s_src = src,
                            .s_dst = dst,
                            .s_amount = amount };
    
    MessageHeader header = { .s_magic = MESSAGE_MAGIC,
                             .s_payload_len = sizeof(TransferOrder),
                             .s_type = TRANSFER,
                             .s_local_time = time };
    Message msg = { .s_header = header };
    memcpy(msg.s_payload, &order, sizeof(TransferOrder));

    send(ipd, src, &msg);
}



int send_stop(void* ipc) {
    MessageHeader header = { .s_magic = MESSAGE_MAGIC,
                             .s_payload_len = 0,
                             .s_type = STOP,
                             .s_local_time = get_physical_time() };
    Message msg = { .s_header = header };
    send_multicast(ipc, &msg);

    return 0;
}

int send_history(IPC* ipc) {
    MessageHeader header = { .s_magic = MESSAGE_MAGIC,
                             .s_payload_len = sizeof(BalanceHistory),
                             .s_type = BALANCE_HISTORY,
                             .s_local_time = get_physical_time() };
    Message msg = { .s_header = header };
    memcpy(msg.s_payload, &ipc->balance_history, sizeof(BalanceHistory));

    send(ipc, 0, &msg);

    return 0;
}

int set_new_balance(BalanceHistory* history, int diff, int t) {
    for(int i = t; i < history->s_history_len+1; i++) {
        BalanceState last = history->s_history[i];
        BalanceState cur = { .s_balance = last.s_balance - diff,
                             .s_time = i };
        history->s_history[i] = cur;
    }
    

        return 0;
}


int wait_msgs(IPC* ipc) {
    Message msg ;
    
    while(1) {
        receive_any(ipc, &msg);

        TransferOrder* t = (TransferOrder*)msg.s_payload;

        switch (msg.s_header.s_type) {
            case TRANSFER : {
                set_new_balance(&ipc->balance_history, t->s_amount, msg.s_header.s_local_time + 1 );

                if(t -> s_amount > 0) {
                    transfer_time(ipc, t->s_dst, ipc->worker_id, -(t->s_amount), 
                                  msg.s_header.s_local_time );
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
    int num_process = atoi(argv[2]);
    AllHistory history = {.s_history_len = num_process};

    for(int i = 0; i < num_process; i++) {
        BalanceHistory h = {.s_id = i+1,
                            .s_history_len = num_process + 1 };
        for(int j = 0; j < num_process + 1; j++)
        {
            BalanceState state = {.s_balance = atoi(argv[i + 3]),
                                   .s_time = j};
            h.s_history[j] = state;
        }
        history.s_history[i] = h;
        
    }

    IPC* ipc = calloc(1, sizeof(IPC));
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


    for(int i = 0; i < num_process; i++) {
        int t;
        wait(&t);
    }

    print_history(&history);

}
