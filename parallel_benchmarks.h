#ifndef PARALLEL_BENCHMARKS_H_
#define PARALLEL_BENCHMARKS_H_

#include <pthread.h>
#include <stdlib.h>
#include "tsc.h"

typedef struct {
	
    unsigned tid;
    char pad0[(64-sizeof(unsigned))/sizeof(char)];
	unsigned nr_operations;
    char pad1[(64-sizeof(unsigned))/sizeof(char)];

	unsigned operations, 
	         enqueues, 
	         dequeues;
    char pad2[(64-3*sizeof(unsigned))/sizeof(char)];
    long long int value_sum;        
    char pad3[(64-sizeof(long long int))/sizeof(char)];
#ifdef WORKLOAD_TIME
	unsigned int time_to_leave;
    char pad4[(64-sizeof(unsigned int))/sizeof(char)];
#endif


	tsc_t insert_lock_set_tsc;
    char pad9[(64-sizeof(tsc_t))/sizeof(char)];

//	tsc_t operations_tsc;
} params_t;

double pthreads_benchmark();

#endif /* PARALLEL_BENCHMARKS_H_ */
