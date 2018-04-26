#include "utils.h"
int find_min(IPC* ipc) {
    Queue queue = ipc->queue;

    int min_time = 666;
    int min_id = 999;
    int min_i = -1;

    for(int i = 0; i < queue.length; i++) {
        QueueAtomic cur_element = queue.elements[i];

        if((cur_element.time < min_time) | 
                (cur_element.time == min_time & cur_element.worker_id < min_id))
        {
            min_time = cur_element.time;
            min_id = cur_element.worker_id;
            min_i = i;
        } 
        
    }

    return min_i;
}


int push(IPC* ipc, timestamp_t time, local_id worker_id) {
    QueueAtomic new_element = { .time = time,
                                .worker_id = worker_id };
    int cur_length = ipc->queue.length;
    ipc -> queue.elements[cur_length] = new_element;
    ipc -> queue.length ++;

    return 0;
}


int pop(IPC* ipc) {
    Queue* queue = &ipc->queue;
    int min_i = find_min(ipc);

    for(int i=min_i; i < queue->length-1; i++)
        queue->elements[i] = queue->elements[i+1];

    queue->length--;

    return 0;
}

QueueAtomic head(IPC* ipc) {
    return ipc->queue.elements[find_min(ipc)];
}


