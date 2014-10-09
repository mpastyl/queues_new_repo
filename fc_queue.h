#ifndef __FC_QUEUE_H
#define __FC_QUEUE_H


#include <pthread.h> /* pthread_spinlock */
#include "tsc.h"
#include "parallel_benchmarks.h"

int MAX_VALUES=16;

struct node_t{
    int enq_count;
    int deq_count;
	int value[16];//TODO: maybe change this
	struct node_t * next;
};

struct queue_t{
	struct node_t * Head;
    char pad1[(64-sizeof(struct node_t *))/sizeof(char)];
	struct node_t * Tail;
    char pad2[(64-sizeof(struct node_t *))/sizeof(char)];
    int lock_enq;
    char pad3[(64-sizeof(int ))/sizeof(char)];
    int lock_deq;
    char pad4[(64-sizeof(int ))/sizeof(char)];
} __attribute__ ((packed));


struct pub_record{ //TODO: pad?? to avoid false sharing.
    int pending; //1 = waiting for op
    char pad1[(64-sizeof(int ))/sizeof(char)];
    int op; // operation 1= enqueue , 0= dequeue
    char pad2[(64-sizeof(int ))/sizeof(char)];
    int val;
    char pad3[(64-sizeof(int ))/sizeof(char)];
    int response; //1 means reposponse is here!
    char pad4[(64-sizeof(int ))/sizeof(char)];
} __attribute__ ((packed));

int ERROR_VALUE=INT32_MAX;



void _initialize(struct queue_t * Q,struct pub_record * pub_enq,struct pub_record * pub_deq,int n){//TODO: init count?
	int i;
    struct node_t * node = (struct node_t *) malloc(sizeof(struct node_t));
    for (i=0;i<MAX_VALUES;i++) node->value[i]=0;
	node->next = NULL;
    node->enq_count=0;
    node->deq_count=0;
	Q->Head = node; //TODO: check this
	Q->Tail = node;
    Q->lock_enq = 0;
    Q->lock_deq = 0;
    for(i=0; i <n ;i++){
        pub_enq[i].pending = 0;
        pub_enq[i].response =0;
        pub_deq[i].pending = 0;
        pub_deq[i].response =0;
    }
}


void _enqueue(struct queue_t * Q, int val){

    struct node_t *  tail=Q->Tail;
    if(tail->enq_count!=MAX_VALUES){// fat node not full
        tail->value[tail->enq_count]=val;
        tail->enq_count++;
    }
    else{
        int i;
        struct node_t * node = (struct node_t *) malloc(sizeof(struct node_t));
        for (i=0;i<MAX_VALUES;i++) node->value[i]=0;
        node->deq_count=0;
        node->value[0]=val;
        node->next=NULL;
        node->enq_count=1;
        Q->Tail->next=node;
        Q->Tail=node;
        tail->enq_count++;
    }
}


int _dequeue(struct queue_t * Q, int * val){

    struct node_t * head=Q->Head; //TODO: fix empty queue!!
    
    struct node_t * next;
    if(head==NULL) return 0;
    else next=head->next;

    /*if(head->enq_count==head->deq_count){
        if(__sync_bool_compare_and_swap(&head->enq_count,head->deq_count,head->deq_count)) {
            return 0;
        }
    }
    */
    if(head->enq_count==head->deq_count) return 0;
    if((head->deq_count>=head->enq_count)&&(next==NULL)){printf("i used this\n");return 0;}//queue is empty
    if(head->deq_count<MAX_VALUES){
        *val=head->value[head->deq_count];
        head->deq_count++;
    }
    else{
        Q->Head=Q->Head->next;
        free(head);
        head=Q->Head;
        next=head->next;
        //if((next==NULL)&&(head->val_count==0)) return 0;
        head->deq_count=1;
        *val=head->value[0];
    }
    return 1;
}

int try_access_enq(struct queue_t * Q,struct pub_record *  pub,int operation, int val,int n,params_t *params){

    int tid=  params->tid;
    int i,res,count;
    //1
    
    pub[tid].op = operation;
    pub[tid].val = val;
    pub[tid].pending=1;
    while (1){
        if(Q->lock_enq){
            count=0;
            while((!pub[tid].response)&&(count<10000)) count++; //check periodicaly for lock
            if(pub[tid].response ==1){
                pub[tid].response=0;
                return (pub[tid].val);
            }
        }
        else{
            if(__sync_lock_test_and_set(&(Q->lock_enq),1)) continue;// must spin backto response
            else{
                for(i=0 ;i<n; i++){
                    if(pub[i].pending){
                        if (pub[i].op ==1) {
                            _enqueue(Q,pub[i].val);
                        }
                        else if(pub[i].op==0){
                           res=_dequeue(Q,&pub[i].val);
                           if(!res) 
                                    pub[i].val = ERROR_VALUE;
                        }
                        else printf("wtf!!  %d \n",pub[i].op);
                        pub[i].pending = 0;
                        pub[i].response = 1;
                    }
                }
                int temp_val=pub[tid].val;
                pub[tid].response=0;
                Q->lock_enq=0;
                return temp_val;
            }
        }
   }
}

int try_access_deq(struct queue_t * Q,struct pub_record *  pub,int operation, int val,int n,params_t * params){

    int tid=  params->tid;
    int i,res,count;
    //1
    
    pub[tid].op = operation;
    pub[tid].val = val;
    pub[tid].pending=1;
    while (1){
        if(Q->lock_deq){
            count=0;
            while((!pub[tid].response)&&(count<10000)) count++; //check periodicaly for lock
            if(pub[tid].response ==1){
                pub[tid].response=0;
                return (pub[tid].val);
            }
        }
        else{
            if(__sync_lock_test_and_set(&(Q->lock_deq),1)) continue;// must spin backto response
            else{
                for(i=0 ;i<n; i++){
                    if(pub[i].pending){
                        if (pub[i].op ==1) {
                            _enqueue(Q,pub[i].val);
                        }
                        else if(pub[i].op==0){
                           res=_dequeue(Q,&pub[i].val);
                           if(!res) 
                                    pub[i].val = ERROR_VALUE;
                        }
                        else printf("wtf!!  %d \n",pub[i].op);
                        pub[i].pending = 0;
                        pub[i].response = 1;
                    }
                }
                int temp_val=pub[tid].val;
                pub[tid].response=0;
                Q->lock_deq=0;
                return temp_val;
            }
        }
   }
}

long long int find_element_sum(struct queue_t * Q){
    
    struct node_t * curr ;
    struct node_t * next ;
    long long int res=0;
    int i;
    curr = Q->Head;
    next = Q->Head->next;
    while (curr != Q->Tail){
//    while (curr){
        for(i=curr->deq_count;i<(curr->enq_count-1);i++){
//            printf("%d ",curr->value[i]);
            res+=curr->value[i];
        }
        curr = next;
        next = curr ->next;
    }
   for(i=curr->deq_count;i<(curr->enq_count);i++){
//      printf("%d ",curr->value[i]);
        res+=curr->value[i];
   }
    //printf("%d ",curr->value);
   return res; 
}

void printqueue(struct queue_t * Q){
    
    struct node_t * curr ;
    struct node_t * next ;
    int i;
    curr = Q->Head;
    next = Q->Head->next;
    while (curr != Q->Tail){
        for(i=curr->deq_count;i<(curr->enq_count-1);i++){
            printf("%d ",curr->value[i]);
        }
        printf("\n----------------------\n");
        curr = next;
        next = curr ->next;
    }
    //printf("%d ",curr->value);
    printf("\n");
    
}

int num_threads;
struct pub_record * pub_enq;
struct pub_record * pub_deq;

void initialize(struct queue_t *Q, int n){
    num_threads= n;
    pub_enq = (struct pub_record *) malloc(sizeof(struct pub_record)*num_threads);
    pub_deq = (struct pub_record *) malloc(sizeof(struct pub_record)*num_threads);
    _initialize(Q,pub_enq,pub_deq,num_threads);
}


void enqueue(struct queue_t *Q, int val,params_t * params){
    
    try_access_enq(Q,pub_enq,1,val,num_threads,params);
}

int dequeue(struct queue_t *Q, int *val,params_t * params){
    
    int res = try_access_deq(Q,pub_deq,0,9,num_threads,params);
    *val=res;
    if (res==ERROR_VALUE) return 0;
    else return 1;


}
	
#endif /*_FC_QUEUE_H*/
