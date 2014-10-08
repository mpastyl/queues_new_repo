#ifndef __GLQUEUE_H
#define __GLQUEUE_H

#include <pthread.h> /* pthread_spinlock */
#include "tsc.h"
#include "parallel_benchmarks.h"

struct node_t{
	int value;//TODO: maybe change this
	struct node_t * next;
};

struct queue_t{
	struct node_t * Head;
	struct node_t * Tail;
//    int lock;
    pthread_spinlock_t lock;
};

#define lock_init(Q)  pthread_spin_init(&Q->lock, PTHREAD_PROCESS_SHARED)
#define lock_queue(Q)   pthread_spin_lock(&Q->lock)
#define unlock_queue(Q) pthread_spin_unlock(&Q->lock)

void initialize(struct queue_t * Q,int nthreads){//TODO: init count?
	struct node_t * node = (struct node_t *) malloc(sizeof(struct node_t));
	node->next = NULL;
	node->value=0;
    Q->Head = node; //TODO: check this
	Q->Tail = node;
    lock_init(Q);
    //Q->lock = 0;
}

void _enqueue(struct queue_t * Q, int val){

    struct node_t * node = (struct node_t *) malloc(sizeof(struct node_t));
    node->value =  val;
    node->next = NULL;
    
    struct node_t * tail = Q->Tail;
    tail->next =node;
    Q->Tail = node;

}

void enqueue(struct queue_t * Q, int val,params_t * params)
{
    /*
    while(1) {
        if (Q->lock == 0) {
            if (!__sync_lock_test_and_set(&Q->lock,1))
                break;
         }
    }
    */
    
    lock_queue(Q);
        _enqueue(Q, val);
    unlock_queue(Q);

    //Q->lock = 0;
}

int _dequeue(struct queue_t * Q, int * val){
    
    
    struct node_t * node = Q->Head;
    struct node_t * next = node->next;
    //struct node_t * tail = Q->Tail;

    if(next == NULL) return 0;
    else{
        *val = next->value;
        Q->Head =next;
    }
    
//    free(node);
    
    return 1;
           
}

int ret;

int dequeue(struct queue_t * Q, int * val,params_t * params){

    /*
    while(1) {
        if (Q->lock == 0) {
            if (!__sync_lock_test_and_set(&Q->lock,1))
                break;
         }
    }
    */
    lock_queue(Q);

        ret = _dequeue(Q, val);
    
    unlock_queue(Q);
    //Q->lock = 0;

    return ret;
}

long long int find_element_sum(struct queue_t * Q){
    
    struct node_t * curr;
    long long res=0;
    curr= Q->Head;
    while(curr!=Q->Tail){
        res+=curr->value;
        curr=curr->next;
    }
    res+=curr->value;
    res-=Q->Head->value;
    return res;
}

void printqueue(struct queue_t * Q){
    
    struct node_t * curr ;
    struct node_t * next ;
    
    curr = Q->Head;
    next = Q->Head->next;
    while (curr != Q->Tail){
        printf("%d ",curr->value);
        curr = next;
        next = curr ->next;
    }
    printf("%d ",curr->value);
    printf("\n");
    
}

#endif /* __GLQUEUE_H */
