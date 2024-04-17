#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

// circular array
typedef struct _queue {
    int size;
    int used;
    int first;
    void **data;
    pthread_mutex_t mutex;
    pthread_cond_t queue_full;
    pthread_cond_t queue_empty;
} _queue;

#include "queue.h"

queue q_create(int size) {
    queue q = malloc(sizeof(_queue));

    q->size  = size;
    q->used  = 0;
    q->first = 0;
    q->data  = malloc(size*sizeof(void *));

    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->queue_full, NULL);
    pthread_cond_init(&q->queue_empty, NULL);

    return q;
}

int q_elements(queue q) {
    pthread_mutex_lock(&q->mutex);
    int a = q->used;
    pthread_mutex_unlock(&q->mutex);
    return a;
}

int q_insert(queue q, void *elem) {
    pthread_mutex_lock(&q->mutex);
    while(q->size == q->used) {
        pthread_cond_wait(&q->queue_full, &q->mutex);
    }

    q->data[(q->first+q->used) % q->size] = elem;
    q->used++;

    if(q->used == 1) pthread_cond_broadcast(&q->queue_empty);
    
    pthread_mutex_unlock(&q->mutex);

    return 1;
}

void *q_remove(queue q) {
    void *res;

    pthread_mutex_lock(&q->mutex);
    while(q->used==0) {
        pthread_cond_wait(&q->queue_empty, &q->mutex);
    }
    res = q->data[q->first];

    q->first = (q->first+1) % q->size;
    q->used--;

    if(q->used == q->size-1) pthread_cond_broadcast(&q->queue_full);

    pthread_mutex_unlock(&q->mutex);

    return res;
}

void q_destroy(queue q) {
    free(q->data);
    free(q);
}
