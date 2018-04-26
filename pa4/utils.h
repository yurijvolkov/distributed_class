#ifndef __UTILS_H
#define __UTILS_H

#include "ipc.h"
#include "banking.h"

#include <unistd.h>
#include <stdio.h>

#define MAX_WORKERS 10
#define READ_DESC 0
#define WRITE_DESC 1

typedef struct {
timestamp_t time;
local_id worker_id;
} QueueAtomic;

typedef struct {
QueueAtomic elements[1000];
int length;
} Queue;

typedef struct {
    int num_workers;
    local_id worker_id;
    int descs[MAX_WORKERS + 1][MAX_WORKERS + 1][2];
    FILE* event_log;
    Queue queue;
} IPC;

int find_min(IPC* ipc);
int push(IPC* ipc, timestamp_t time, local_id worker_id);
int pop(IPC* ipc);
QueueAtomic head(IPC* ipc);
int init_pipes(IPC* ipc);
int init_logs(IPC* ipc);
int sync_workers(IPC* ipc);
int close_unused_pipes(IPC* ipc);
int send_stop(void* ipc);
int send_reply(void* ipc, local_id to);
static inline int get_options(int argc, char* argv[]);


#endif //__UTILS_H
