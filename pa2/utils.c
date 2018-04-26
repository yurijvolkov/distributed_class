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
#include <getopt.h>


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
            if (receive((void*)ipc, i, msg) == 0) 
                    break;
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
