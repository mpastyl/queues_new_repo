#ifndef __FC_HYBRID_H
#define __FC_HYBRID_H

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
    pthread_spinlock_t lock_enq;
    char pad3[(64-sizeof(int))/sizeof(char)];
    pthread_spinlock_t lock_deq;
    char pad4[(64-sizeof(int))/sizeof(char)];
} __attribute__ ((packed));




struct pub_record{ //TODO: pad?? to avoid false sharing.
    int pending; //1 = waiting for op
    char pad1[(64-sizeof(int))/sizeof(char)];
    int op; // operation 1= enqueue , 0= dequeue
    char pad2[(64-sizeof(int))/sizeof(char)];
    int val;
    char pad3[(64-sizeof(int))/sizeof(char)];
    int response; //1 means reposponse is here!
    char pad4[(64-sizeof(int))/sizeof(char)];
} __attribute__ ((packed));

int ERROR_VALUE=5004;



//if op==1 we work on the enqueue locks otherwise on the dequeue locks
void lock_queue (struct queue_t * Q,int op){

    if (op==1){
        pthread_spin_lock(&Q->lock_enq);
    }   
    else{
        pthread_spin_lock(&Q->lock_deq);

    }
}

//if op==1 we work on the enqueue locks otherwise on the dequeue locks
void unlock_queue(struct queue_t * Q,int op){

    if (op==1) pthread_spin_unlock(&Q->lock_enq);
    else pthread_spin_unlock(&Q->lock_deq);
}

// i need 4 locks but i do some padding so that each lock is on different line 
int locks_enq[8];
int locks_deq[8];

void _initialize(struct queue_t * Q,struct pub_record * pub0_enq,struct pub_record * pub1_enq,struct pub_record * pub2_enq,struct pub_record * pub3_enq,struct pub_record * pub0_deq, struct pub_record * pub1_deq ,struct pub_record * pub2_deq, struct pub_record * pub3_deq,int n){//TODO: init count?
	int i;
    struct node_t * node = (struct node_t *) malloc(sizeof(struct node_t));
	node->next = NULL;

    Q->Head= node;
    Q->Tail= node;
    pthread_spin_init(&Q->lock_enq,PTHREAD_PROCESS_SHARED);
    pthread_spin_init(&Q->lock_deq,PTHREAD_PROCESS_SHARED);
    //Q->lock = 0;
    for(i=0;i<8;i++) locks_enq[i]=0;
    for(i=0;i<8;i++) locks_deq[i]=0;
    for(i=0; i <n ;i++){
        pub0_enq[i].pending = 0;
        pub0_enq[i].response =0;
        pub1_enq[i].pending = 0;
        pub1_enq[i].response =0;
        pub2_enq[i].pending = 0;
        pub2_enq[i].response =0;
        pub3_enq[i].pending = 0;
        pub3_enq[i].response =0;
        pub0_deq[i].pending = 0;
        pub0_deq[i].response =0;
        pub1_deq[i].pending = 0;
        pub1_deq[i].response =0;
        pub2_deq[i].pending = 0;
        pub2_deq[i].response =0;
        pub3_deq[i].pending = 0;
        pub3_deq[i].response =0;

    }
}

void _enqueue(struct queue_t * Q, int val){

    struct node_t *  tail=Q->Tail;
    if(tail->enq_count!=MAX_VALUES){// fat node not full
        tail->value[tail->enq_count]=val;
        tail->enq_count++;
    }
    else{
        struct node_t * node = (struct node_t *) malloc(sizeof(struct node_t));
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
    struct node_t * next=head->next;

    /*if(head->enq_count==head->deq_count){
        if(__sync_bool_compare_and_swap(&head->enq_count,head->deq_count,head->deq_count)) {
            return 0;
        }
    }
    */
    if (head->enq_count==head->deq_count) return 0;
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


int try_access_enq(struct queue_t * Q,struct pub_record *  pub,int operation, int val,int n,params_t * params){

    int tid=  params->tid%16;
    int i,res,count;
    // *2 because of the padding of locks
    int lock_indx=2*(params->tid/16);


    pub[tid].op = operation;
    pub[tid].val = val;
    pub[tid].pending=1;
    while (1){
        if(locks_enq[lock_indx]){
            count=0;
            while((!pub[tid].response)&&(count<10000)) count++; //check periodicaly for lock
            if(pub[tid].response ==1){
                pub[tid].response=0;
                return (pub[tid].val);
            }
        }
        else{
            if(__sync_lock_test_and_set(&(locks_enq[lock_indx]),1)) continue;// must spin backto response
            else{
                lock_queue(Q,1);
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
                unlock_queue(Q,1);
                locks_enq[lock_indx]=0;
                return temp_val;
            }
        }
   }
}

int try_access_deq(struct queue_t * Q,struct pub_record *  pub,int operation, int val,int n,params_t * params){

    int tid=  params->tid%16;
    int i,res,count;
    //1
    int lock_indx=2*(params->tid/16);


    pub[tid].op = operation;
    pub[tid].val = val;
    pub[tid].pending=1;
    while (1){
        if(locks_deq[lock_indx]){
            count=0;
            while((!pub[tid].response)&&(count<10000)) count++; //check periodicaly for lock
            if(pub[tid].response ==1){
                pub[tid].response=0;
                return (pub[tid].val);
            }
        }
        else{
            if(__sync_lock_test_and_set(&(locks_deq[lock_indx]),1)) continue;// must spin backto response
            else{
                lock_queue(Q,0);
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
                unlock_queue(Q,0);
                locks_deq[lock_indx]=0;
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
/*
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
*/

struct pub_record * pub0_enq;
struct pub_record * pub1_enq;
struct pub_record * pub2_enq;
struct pub_record * pub3_enq;
struct pub_record * pub0_deq;
struct pub_record * pub1_deq;
struct pub_record * pub2_deq;
struct pub_record * pub3_deq;
int num_threads;
void initialize(struct queue_t *Q, int n){
    num_threads= n;
    pub0_enq = (struct pub_record *) malloc(sizeof(struct pub_record)*num_threads);
    pub0_deq = (struct pub_record *) malloc(sizeof(struct pub_record)*num_threads);
    pub1_enq = (struct pub_record *) malloc(sizeof(struct pub_record)*num_threads);
    pub1_deq = (struct pub_record *) malloc(sizeof(struct pub_record)*num_threads);
    pub2_enq = (struct pub_record *) malloc(sizeof(struct pub_record)*num_threads);
    pub2_deq = (struct pub_record *) malloc(sizeof(struct pub_record)*num_threads);
    pub3_enq = (struct pub_record *) malloc(sizeof(struct pub_record)*num_threads);
    pub3_deq = (struct pub_record *) malloc(sizeof(struct pub_record)*num_threads);
    _initialize(Q,pub0_enq,pub1_enq,pub2_enq,pub3_enq,pub0_deq,pub1_deq,pub2_deq,pub3_deq,16);
}


void enqueue(struct queue_t *Q, int val,params_t * params){
    
    int tid=params->tid;
    if(tid<16)
        try_access_enq(Q,pub0_enq,1,val,16,params);
    else if(tid<32)
        try_access_enq(Q,pub1_enq,1,val,16,params);
    else if(tid<48)
        try_access_enq(Q,pub2_enq,1,val,16,params);
    else
        try_access_enq(Q,pub3_enq,1,val,16,params);

}

int dequeue(struct queue_t *Q, int *val,params_t * params){
    
    int res;
    int tid=params->tid;
    if(tid<16)
        res = try_access_deq(Q,pub0_deq,0,9,num_threads,params);
    else if(tid<32)
        res = try_access_deq(Q,pub1_deq,0,9,num_threads,params);
    else if(tid<48)
        res = try_access_deq(Q,pub2_deq,0,9,num_threads,params);
    else
        res = try_access_deq(Q,pub3_deq,0,9,num_threads,params);
    *val=res;
    if (res==ERROR_VALUE) return 0;
    else return 1;


}
#endif /*__FC_HYBRID_H*/
