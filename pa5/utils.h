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
int deferred_reply[MAX_WORKERS];
int received_reply[MAX_WORKERS];
} RA;

typedef struct {
    int num_workers;
    local_id worker_id;
    int descs[MAX_WORKERS + 1][MAX_WORKERS + 1][2];
    FILE* event_log;
    Queue queue;
    RA ra;
} IPC;

int find_min(IPC* ipc);
int push(IPC* ipc, timestamp_t time, local_id worker_id);
int pop(IPC* ipc);
QueueAtomic head(IPC* ipc);

int flush(IPC* ipc);
int set_dr(IPC* ipc, int worker_id);
int set_received(IPC* ipc, int worker_id);
int check_is_received_all(IPC* ipc);
