#include "ipc.h"
#include "banking.h"

#include <unistd.h>
#include <stdio.h>

#define MAX_WORKERS 10
#define READ_DESC 0
#define WRITE_DESC 1

typedef struct {
    int num_workers;
    local_id worker_id;
    FILE* event_log;
    int descs[MAX_WORKERS + 1][MAX_WORKERS + 1][2];
    BalanceHistory balance_history;
} IPC;

int init_pipes(IPC* ipc);
int init_logs(IPC* ipc);
int sync_workers(IPC* ipc);
int get_balances(IPC* ipc, AllHistory* all_history);
int close_unused_pipes(IPC* ipc);
void transfer(void* ipd, local_id src, local_id dst, balance_t amount);
void transfer_time(void* ipd, local_id src, local_id dst, balance_t amount, int time);
int send_stop(void* ipc);
int send_history(IPC* ipc);
int set_new_balance(BalanceHistory* history, int diff, int t, int pending);
int get_options(int argc, char** argv, int* num_process);
