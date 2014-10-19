#ifndef __FC_QUEUE_H
#define __FC_QUEUE_H


#include <pthread.h> /* pthread_spinlock */
#include "tsc.h"
#include "parallel_benchmarks.h"

#define MAX_VALUES 16

struct node_t {
    int enq_count;
    int deq_count;
	int value[MAX_VALUES];//TODO: maybe change this
	struct node_t * next;
};

struct queue_t {
	struct node_t * Head;
    char pad1[(64-sizeof(struct node_t *))/sizeof(char)];
	struct node_t * Tail;
    char pad2[(64-sizeof(struct node_t *))/sizeof(char)];
    int lock;
    char pad3[(64-sizeof(int ))/sizeof(char)];
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

/* Number of threads and publication records table. */
int num_threads;
struct pub_record *pub_records;

void _initialize(struct queue_t * Q, int n){//TODO: init count?
	int i;
    struct node_t * node = (struct node_t *) malloc(sizeof(struct node_t));
    for (i=0;i<MAX_VALUES;i++) node->value[i]=0;
	node->next = NULL;
    node->enq_count=0;
    node->deq_count=0;
	Q->Head = node; //TODO: check this
	Q->Tail = node;
    Q->lock = 0;
    for(i=0; i <n ;i++)
		pub_records[i].request = PUB_RECORD_COMBINE(0,0,0);
}


void _enqueue(struct queue_t * Q, int val) {

    struct node_t *tail = Q->Tail;

    if(tail->enq_count!=MAX_VALUES) { // fat node not full
        tail->value[tail->enq_count]=val;
        tail->enq_count++;
    } else {
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


int _dequeue(struct queue_t * Q, int * val) {

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

unsigned long long total_combiners=0;
unsigned long long combiner_changed=0;
int last_combiner=-1;


void execute_operation(struct queue_t * Q, int n, params_t *params)
{
    int tid = params->tid;
    int i, res, count,j;
	struct pub_record *pub = &pub_records[tid];
    
    while (1) {
        if(Q->lock) { /* Lock is taken. */
			tsc_start(&params->fc_pub_spin_tsc);
            count=0;
            while((PUB_RECORD_TO_PENDING(pub) == 1) && (count<10000)) 
				count++; //check periodicaly for lock

            if(PUB_RECORD_TO_PENDING(pub) == 0) {
				tsc_pause(&params->fc_pub_spin_tsc);
                return;
            }
			tsc_pause(&params->fc_pub_spin_tsc);
        } else { /* Lock if free. */
            if(__sync_lock_test_and_set(&(Q->lock),1)) {
				continue;
			} else { /* We are the combiner. */

              /*
              total_combiners++;
              if (last_combiner!= tid) {
                    last_combiner = tid;
                    combiner_changed++;
              }
              */

              for(j=0;j<clargs.loops;j++){
                for (i=0; i<n; i++) {
					struct pub_record *curr_pub_p = &pub_records[i];
					struct pub_record curr_pub = pub_records[i];
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
              }

                Q->lock = 0;
                return;
            }
        }
   }
}


void initialize(struct queue_t *Q, int n){
    num_threads= n;
	pub_records = malloc(num_threads * sizeof(*pub_records));

    _initialize(Q,num_threads);
}


void enqueue(struct queue_t *Q, int val,params_t * params){
    
	int tid = params->tid;
	pub_records[tid].request = PUB_RECORD_COMBINE(1,1,val);

//	printf("Going to enqueue %16x and pub val is %16x and pub %16llx\n",
//	       val, PUB_RECORD_TO_VAL(&pub_records[tid]), pub_records[tid].request);
	execute_operation(Q, num_threads, params);
//    try_access_enq(Q,pub_enq,1,val,num_threads,params);
}

int dequeue(struct queue_t *Q, int *val,params_t * params){
    
	int tid = params->tid;
	pub_records[tid].request = PUB_RECORD_COMBINE(0,1,0);

	execute_operation(Q, num_threads, params);
//    try_access_deq(Q,pub_deq,0,9,num_threads,params);

//	printf("I return %8x\n", PUB_RECORD_TO_VAL(&pub_records[tid]));
	int dequeued_val = PUB_RECORD_TO_VAL(&pub_records[tid]);
	*val = dequeued_val;

	return (dequeued_val != ERROR_VALUE);
}
	
#endif /*_FC_QUEUE_H*/
