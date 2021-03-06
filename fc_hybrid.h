#ifndef __FC_HYBRID_H
#define __FC_HYBRID_H

#include <pthread.h> /* pthread_spinlock */
#include "tsc.h"
#include "parallel_benchmarks.h"


#define MAX_VALUES 16

struct node_t{
    int enq_count;
    int deq_count;
    int value[MAX_VALUES];//TODO: maybe change this
    struct node_t * next;
};

struct queue_t{
    struct node_t * Head;
    char pad1[(64-sizeof(struct node_t *))/sizeof(char)];
    struct node_t * Tail;
    char pad2[(64-sizeof(struct node_t *))/sizeof(char)];
    pthread_spinlock_t lock;
    char pad3[(64-sizeof(pthread_spinlock_t))/sizeof(char)];
} __attribute__ ((packed));




struct pub_record{ //TODO: pad?? to avoid false sharing.
	/* request -> [ reserved(29bits) | operation(2bits) | pending(1bit) | val(32bits) ] */
	/*volatile*/ long long request; 
    char pad1[(64-sizeof(long long))/sizeof(char)];
} __attribute__ ((packed));



#define LSB_32BIT(num)  ( (num) & 0x00000001 )
#define LSB2_32BIT(num) ( (num) & 0x00000003 )

#define PUB_RECORD_COMBINE_OP_PEND(op,pend) \
	( (LSB2_32BIT(op) << 1) | LSB_32BIT(pend) ) 
#define PUB_RECORD_COMBINE(op,pend,val) \
	( ((long long)(PUB_RECORD_COMBINE_OP_PEND(op,pend)) << 32) | (long long)(val) )

/* Getters */
#define PUB_RECORD_TO_OP(pub)      LSB2_32BIT(((pub)->request) >> 33)
#define PUB_RECORD_TO_PENDING(pub) LSB_32BIT(((pub)->request) >> 32)
#define PUB_RECORD_TO_VAL(pub)     ((int)(((pub)->request) & 0x00000000ffffffff))

/* Setters */
#define PUB_RECORD_ZERO_PENDING(pub) \
	( ((pub)->request) &= 0xfffffffeffffffff )
#define PUB_RECORD_SET_PENDING(pub) \
	( ((pub)->request) &= 0x0000000100000000 )

int ERROR_VALUE = INT32_MAX;

//if op==1 we work on the enqueue locks otherwise on the dequeue locks
void lock_queue (struct queue_t * Q,int op){

        pthread_spin_lock(&Q->lock);

}

//if op==1 we work on the enqueue locks otherwise on the dequeue locks
void unlock_queue(struct queue_t * Q,int op){

    pthread_spin_unlock(&Q->lock);
}

// i need 4 locks but i do some padding so that each lock is on different line 
//int locks[64*4];


struct pub_record *pub_records_0;
struct pub_record *pub_records_1;
struct pub_record *pub_records_2;
struct pub_record *pub_records_3;


void _initialize(struct queue_t * Q,int n){//TODO: init count?
	int i;
    struct node_t * node = (struct node_t *) malloc(sizeof(struct node_t));
	node->next = NULL;
    for (i=0;i<MAX_VALUES;i++) node->value[i]=0;
    node->enq_count=0;
    node->deq_count=0;

    Q->Head= node;
    Q->Tail= node;
    pthread_spin_init(&Q->lock,PTHREAD_PROCESS_SHARED);
    //Q->lock = 0;
    //for(i=0;i<(64*4);i++) locks[i]=0;
    for(i=0; i <n ;i++){

		pub_records_0[i].request = PUB_RECORD_COMBINE(0,0,0);
		pub_records_1[i].request = PUB_RECORD_COMBINE(0,0,0);
		pub_records_2[i].request = PUB_RECORD_COMBINE(0,0,0);
		pub_records_3[i].request = PUB_RECORD_COMBINE(0,0,0);
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

    if(head->enq_count==head->deq_count) 
		return 0;

    if((head->deq_count>=head->enq_count)&&(next==NULL)) {
		printf("i used this\n");return 0; //queue is empty
	}

    if(head->deq_count<MAX_VALUES){
        *val=head->value[head->deq_count];
        head->deq_count++;
    } else{
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


unsigned long long total_combiners=0;
unsigned long long combiner_changed=1;
int last_combiner=-1;

void execute_operation(struct queue_t * Q,struct pub_record *  _pub,int n,params_t * params){

    int tid=  params->tid%16;
    int i,res,count;
    // *64 because of the padding of locks
    //int lock_indx=64*(params->tid/16);

    //printf("thread id %d, core id %d , local tid %d  lock_indx %d \n",params->tid,params->cpu,tid,lock_indx);

	struct pub_record *my_pub = &_pub[tid];

    while (1){
        if(tid!=0){
			tsc_start(&params->fc_pub_spin_tsc);
            count=0;
            while((PUB_RECORD_TO_PENDING(my_pub) == 1) && (count<10000)) 
				count++; //check periodicaly for lock

            if(PUB_RECORD_TO_PENDING(my_pub) == 0) {
				tsc_pause(&params->fc_pub_spin_tsc);
                return;
            }
			tsc_pause(&params->fc_pub_spin_tsc);
            if (params->time_to_leave){
                //params->failed_to_do_op=1;
                return;
            }
        }
        else{
            if(tid!=0) continue;// must spin backto response
            else{
              
                /*
                total_combiners++;
                if (last_combiner!= tid) {
                    last_combiner = tid;
                    combiner_changed++;
                }
                */
                
                lock_queue(Q,1);
                
                
                for (i=0; i<n; i++) {
					struct pub_record *curr_pub_p = &_pub[i];
					struct pub_record curr_pub = _pub[i];
                    if(PUB_RECORD_TO_PENDING(&curr_pub) == 1) {
						int operation = PUB_RECORD_TO_OP(&curr_pub);
                        if (operation == 1) {
                            _enqueue(Q,PUB_RECORD_TO_VAL(&curr_pub));
                        } else if(operation == 0) {
							int temp = 0;
                           if (!_dequeue(Q,&temp))
							   temp = ERROR_VALUE;
						   curr_pub_p->request = PUB_RECORD_COMBINE(0,1,temp);
                        } else {
							printf("wtf!!  %d \n",operation);
						}
//						printf("request before is %16llx\n", curr_pub_p->request);
//						printf("pending before is %8llx\n", PUB_RECORD_TO_PENDING(curr_pub_p));
						PUB_RECORD_ZERO_PENDING(curr_pub_p);
//						printf("request after  is %16llx\n", curr_pub_p->request);
//						printf("pending after  is %8llx\n", PUB_RECORD_TO_PENDING(curr_pub_p));
                    }
                }
                unlock_queue(Q,1);
                //locks[lock_indx]=0;
                return ;
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

int num_threads;
void initialize(struct queue_t *Q, int n){
    num_threads= n;
    pub_records_0 = malloc(16 * sizeof(*pub_records_0));
    pub_records_1 = malloc(16 * sizeof(*pub_records_1));
    pub_records_2 = malloc(16 * sizeof(*pub_records_2));
    pub_records_3 = malloc(16 * sizeof(*pub_records_3));
    
    _initialize(Q,16);
}


void enqueue(struct queue_t *Q, int val,params_t * params){
    
    int tid=params->tid;
    int n;
    if(num_threads<16) n=num_threads;
    else n=16;
    int local_tid=(params->tid) % 16;
    if(tid<16){
	    pub_records_0[local_tid].request = PUB_RECORD_COMBINE(1,1,val);
	    execute_operation(Q, pub_records_0, n, params);
    }
    else if(tid<32){
	    pub_records_1[local_tid].request = PUB_RECORD_COMBINE(1,1,val);
	    execute_operation(Q, pub_records_1, n, params);
    }
    else if(tid<48){
	    pub_records_2[local_tid].request = PUB_RECORD_COMBINE(1,1,val);
	    execute_operation(Q, pub_records_2, n, params);
    }
    else{
	    pub_records_3[local_tid].request = PUB_RECORD_COMBINE(1,1,val);
	    execute_operation(Q, pub_records_3, n, params);
    }

}

int dequeue(struct queue_t *Q, int *val,params_t * params){
    
    int n;
    int res;
    int dequeued_val;
    int tid=params->tid;
    int local_tid=(params->tid) % 16;
    if(num_threads<16) n=num_threads;
    else n=16;
    if(tid<16){
	    pub_records_0[local_tid].request = PUB_RECORD_COMBINE(0,1,0);
	    execute_operation(Q, pub_records_0, n, params);
	    dequeued_val = PUB_RECORD_TO_VAL(&pub_records_0[local_tid]);
	    *val = dequeued_val;
    }
    else if(tid<32){
	    pub_records_1[local_tid].request = PUB_RECORD_COMBINE(0,1,0);
	    execute_operation(Q, pub_records_1, n, params);
	    dequeued_val = PUB_RECORD_TO_VAL(&pub_records_1[local_tid]);
	    *val = dequeued_val;
    }
    else if(tid<48){
	    pub_records_2[local_tid].request = PUB_RECORD_COMBINE(0,1,0);
	    execute_operation(Q, pub_records_2, n, params);
	    dequeued_val = PUB_RECORD_TO_VAL(&pub_records_2[local_tid]);
	    *val = dequeued_val;
    }
    else{
	    pub_records_3[local_tid].request = PUB_RECORD_COMBINE(0,1,0);
	    execute_operation(Q, pub_records_3, n, params);
	    dequeued_val = PUB_RECORD_TO_VAL(&pub_records_3[local_tid]);
	    *val = dequeued_val;
    }

	return (dequeued_val != ERROR_VALUE);

}
#endif /*__FC_HYBRID_H*/
