#ifndef __MSQUEUE_NOABA_H
#define __MSQUEUE_NOABA_H

#include <pthread.h> /* pthread_spinlock */
#include "tsc.h"
#include "parallel_benchmarks.h"

struct node_t{
	int value;//TODO: maybe change this
	struct node_t *next;
};

struct queue_t{
	struct node_t *Head;
    char pad1[(64-sizeof(struct node_t *))/sizeof(char)];
	struct node_t *Tail;
    char pad2[(64-sizeof(struct node_t *))/sizeof(char)];
}__attribute__ ((packed));

void initialize(struct queue_t * Q, int num_threads){
	struct node_t * node = (struct node_t *) malloc(sizeof(struct node_t));
	node->next = NULL;
	Q->Head = node; 
	Q->Tail = node;
}

void enqueue(struct queue_t * Q, int val,params_t *params){
	
	struct node_t *tail;
	struct node_t * node = (struct node_t *) malloc(sizeof(struct node_t));
	node->value = val;
	node->next = NULL;
	while (1) {
		tail = Q->Tail;
		struct node_t *next_p = tail->next;
		if (tail == Q->Tail) {
			if (next_p == NULL){
                if (__sync_bool_compare_and_swap(&tail->next, next_p, node))
                    break;
			}
			else{
                __sync_bool_compare_and_swap(&Q->Tail, tail, next_p);
			}
		}
	}

    __sync_bool_compare_and_swap(&Q->Tail, tail, node);
}

int dequeue(struct queue_t * Q,int * pvalue,params_t *params){

	struct node_t *head;
	struct node_t *tail;
	struct node_t *next;

	while(1){
		head = Q->Head;
		tail = Q->Tail;
		next = head->next;
		if (head == Q->Head){
			if (head == tail){
				if (next == NULL) 
					return 0;
                __sync_bool_compare_and_swap(&Q->Tail, tail, next);
			}
			else{
                //if ((head==Q->Head) &&(first_val!=((struct node_t *)get_pointer(head))->value)) printf("change detected!\n");
				*pvalue = next->value;
                if (__sync_bool_compare_and_swap(&Q->Head, head, next))
                    break;
			}
		}
	}

    //printf(" about to free %p \n",head);
//	free(get_pointer(head.both));
	return 1;
}

void printqueue(struct queue_t * Q){
	
	struct node_t *curr ;
	struct node_t *next ;
	
	curr = Q->Head;
    //printf(" in printqueue curr = %p\n",curr);
	next = Q->Head->next;
    //printf(" in printqueue next = %p\n",next);
	while (curr != Q->Tail && curr != NULL){
    //printf(" in printqueue curr = %p\n",curr);
    //printf(" in printqueue next = %p\n",next);
		printf("%d ",curr->value);
		curr = next;
		if (next) 
            next = curr->next;
	}
	printf("%d ",curr->value);
	printf("\n");
	
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
#endif /* __MSQUEUE_NOABA_H */
