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

