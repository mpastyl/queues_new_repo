#ifndef __FC_HYBRID_H
#define __FC_HYBRID_H

#include <pthread.h> /* pthread_spinlock */
#include "tsc.h"
#include "parallel_benchmarks.h"

int MAX_VALUES=16;


unsigned long long total_combiners=1;
unsigned long long combiner_changed=1;

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
    pthread_spinlock_t lock;
    char pad3[(64-sizeof(int))/sizeof(char)];
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

int ERROR_VALUE=INT32_MAX;



//if op==1 we work on the enqueue locks otherwise on the dequeue locks
void lock_queue (struct queue_t * Q,int op){

        pthread_spin_lock(&Q->lock);
}

//if op==1 we work on the enqueue locks otherwise on the dequeue locks
void unlock_queue(struct queue_t * Q,int op){

    pthread_spin_unlock(&Q->lock);
}

// i need 4 locks but i do some padding so that each lock is on different line 
int locks_enq[8];
int locks_deq[8];

void _initialize(struct queue_t * Q,struct pub_record * pub0,struct pub_record * pub1,struct pub_record * pub2,struct pub_record * pub3,int n){//TODO: init count?
	int i;
    struct node_t * node = (struct node_t *) malloc(sizeof(struct node_t));
	node->next = NULL;
    for(i=0; i<MAX_VALUES;i++) node->value[i]=0;
    node->enq_count=0;
    node->deq_count=0;

    Q->Head= node;
    Q->Tail= node;
    pthread_spin_init(&Q->lock,PTHREAD_PROCESS_SHARED);
    //Q->lock = 0;
    for(i=0;i<8;i++) locks_enq[i]=0;
    for(i=0;i<8;i++) locks_deq[i]=0;
    for(i=0; i <n ;i++){
        pub0[i].pending = 0;
        pub0[i].response =0;
        pub1[i].pending = 0;
        pub1[i].response =0;
        pub2[i].pending = 0;
        pub2[i].response =0;
        pub3[i].pending = 0;
        pub3[i].response =0;

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
    if((head->deq_count>=head->enq_count)&&(next==NULL)){
        return 0;}//queue is empty
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


int try_access(struct queue_t * Q,struct pub_record *  pub,int operation, int val,int n,params_t * params){

    int tid=  params->tid%16;
    int i,res,count;
    // *2 because of the padding of locks
    int lock_indx=2*(params->tid/16);


    pub[tid].op = operation;
    pub[tid].val = val;
    pub[tid].pending=1;
    while (1){
        if(tid!=0){
            count=0;
            while((!pub[tid].response)&&(count<10000)) count++; //check periodicaly for lock
            if(pub[tid].response ==1){
                pub[tid].response=0;
                return (pub[tid].val);
            }
            if(params->time_to_leave ==1) {
                params->failed_to_do_op=1;
                return ERROR_VALUE;
            }
        }
        else{
            if(tid!=0) continue;// must spin backto response
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
                //locks_enq[lock_indx]=0;
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

struct pub_record * pub0;
struct pub_record * pub1;
struct pub_record * pub2;
struct pub_record * pub3;
int num_threads;
void initialize(struct queue_t *Q, int n){
    num_threads= n;
    pub0 = (struct pub_record *) malloc(sizeof(struct pub_record)*16);
    pub1 = (struct pub_record *) malloc(sizeof(struct pub_record)*16);
    pub2 = (struct pub_record *) malloc(sizeof(struct pub_record)*16);
    pub3 = (struct pub_record *) malloc(sizeof(struct pub_record)*16);
    _initialize(Q,pub0,pub1,pub2,pub3,16);
}


void enqueue(struct queue_t *Q, int val,params_t * params){
    
    int tid=params->tid;
    if(tid<16)
        try_access(Q,pub0,1,val,16,params);
    else if(tid<32)
        try_access(Q,pub1,1,val,16,params);
    else if(tid<48)
        try_access(Q,pub2,1,val,16,params);
    else
        try_access(Q,pub3,1,val,16,params);

}

int dequeue(struct queue_t *Q, int *val,params_t * params){
    
    int res;
    int tid=params->tid;
    if(tid<16)
        res = try_access(Q,pub0,0,9,16,params);
    else if(tid<32)
        res = try_access(Q,pub1,0,9,16,params);
    else if(tid<48)
        res = try_access(Q,pub2,0,9,16,params);
    else
        res = try_access(Q,pub3,0,9,16,params);
    *val=res;
    if (res==ERROR_VALUE) return 0;
    else return 1;


}
#endif /*__FC_HYBRID_H*/
