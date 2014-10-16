#ifndef PARALLEL_BENCHMARKS_H_
#define PARALLEL_BENCHMARKS_H_

#include <pthread.h>
#include <stdlib.h>
#include "tsc.h"
#include "prfcnt_snb.h"

typedef struct {
	
    unsigned tid;
	unsigned int cpu;
    char pad0[(64-sizeof(unsigned))/sizeof(char)];
	unsigned nr_operations;
    char pad1[(64-sizeof(unsigned))/sizeof(char)];

	unsigned operations, 
	         enqueues, 
	         dequeues;
    char pad2[(64-3*sizeof(unsigned))/sizeof(char)];
    long long int enq_sum;        
    char pad3[(64-sizeof(long long int))/sizeof(char)];
    long long int deq_sum;        
    char pad13[(64-sizeof(long long int))/sizeof(char)];
#ifdef WORKLOAD_TIME
	unsigned int time_to_leave;
    char pad4[(64-sizeof(unsigned int))/sizeof(char)];
#endif

#ifdef GLOBAL_LOCK
    double max_wait;
    char pad15[(64-sizeof(double))/sizeof(char)];
#endif

#ifdef MSQUEUE_ABA
    unsigned long long max_streak;
    unsigned long long curr_streak;
    int last_cas;
    char pad14[(64-2*sizeof(unsigned long long int) \
        - sizeof(int ))/sizeof(char)];
#endif

#if defined(FC_QUEUE) || defined(FC_ONE_WORD)
	tsc_t fc_pub_spin_tsc;
    char padFC[(64-sizeof(tsc_t))/sizeof(char)];
#endif

	tsc_t insert_lock_set_tsc;
    char pad9[(64-sizeof(tsc_t))/sizeof(char)];

	prfcnt_t prfcnt;
    char pad10[(64-sizeof(prfcnt_t))/sizeof(char)];
//	tsc_t operations_tsc;
} params_t;

double pthreads_benchmark();

#endif /* PARALLEL_BENCHMARKS_H_ */
