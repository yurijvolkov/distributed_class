#include "ipc.h"

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
} IPC;

int init_pipes(IPC* ipc);
int init_logs(IPC* ipc);
int sync_workers(IPC* ipc);
int close_unused_pipes(IPC* ipc);
int get_options(int argc, char* argv[], int* num_process);
