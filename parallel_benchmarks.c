#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include "timers_lib.h"
#include "alloc.h"
#include "parallel_benchmarks.h"
#include "clargs.h"
#include "aff.h"

#ifdef GLOBAL_LOCK
#	include "global_lock.h"
#elif defined(MSQUEUE)
#	include "msqueue-noaba.h"
#elif defined(MSQUEUE_ABA)
#	include "msqueue.h"
#elif defined(OPTIMISTIC)
#	include "optimistic_queue.h"
#elif defined(FC_QUEUE)
#	include "fc_queue.h"
#elif defined(FC_DEDICATED)
#	include "fc_dedicated.h"
#elif defined(FC_HYBRID)
#	include "fc_hybrid.h"
#else
#	error "Please define  queue type\n"
#endif

/* The Hash Table */
struct queue_t Q;

static pthread_t *threads;
static params_t *params;
static pthread_barrier_t barrier;
int elements_count=0;
long long int total_enq_sum=0;
long long int total_deq_sum=0;


static void params_print()
{
	int i;

	int total_operations = 0, total_enqueues = 0, 
	    total_dequeues = 0;
	for (i=0; i < clargs.num_threads; i++) {
		printf("Thread %2d: %8d ( %8d %8d )\n", 
		       params[i].tid, params[i].operations,
		       params[i].enqueues, 
		       params[i].dequeues);
		total_operations += params[i].operations;
		total_enqueues += params[i].enqueues;
		total_dequeues += params[i].dequeues;
        
        
        if (clargs.verify == 1){
           //elements_count += params[i].enqueues
          //elements_count -= params[i].dequeues;
            total_enq_sum += params[i].enq_sum;
            total_deq_sum += params[i].deq_sum;
        }
        
	}
	printf("%10s %8d ( %8d %8d )\n", "TotalStat",
	       total_operations, total_enqueues, total_dequeues);
	printf("\n");

    
    if (clargs.verify==1){
        /*printf("\nTest1 start!\n");
        int expected_count = find_elements_count(&ht);
        if(expected_count == elements_count) printf("Test1 PASSED\n\n");
        else{
            printf("Test1 FAILED\n");
            printf(" elements in HT == %d , elements_counted == %d\n\n",expected_count,elements_count);
        }
        */
        printf("\nTest start!\n");
        long long int found_sum = find_element_sum(&Q);
        if((found_sum +total_deq_sum) == total_enq_sum) printf("Test PASSED\n\n");
        else{
            printf("Test FAILED\n");
            printf(" found sum == %lld +  total_deq %lld = %lld  VS  total_enq %lld\n",
            found_sum, total_deq_sum, found_sum+total_deq_sum ,total_enq_sum);
        }
    }
    


	printf("Verbose timers: insert_lock_set_tsc\n");
	for (i=0; i < clargs.num_threads; i++) {
		printf("  Thread %2d: %4.2lf\n", params[i].tid,
		       tsc_getsecs(&params[i].insert_lock_set_tsc));
	}
	printf("\n");


#ifdef WORKLOAD_TIME
	printf("\033[31mOps/usec:\033[0m %2.4lf\n",
	       (double) total_operations / (clargs.run_time_sec * 1000000));
	printf("\n");
#endif
}



timer_tt *wall_timer;

void *thread_fn(void *args)
{
	unsigned int i;
	params_t *params = args;
	struct drand48_data drand_buffer;
	long int drand_res;
	int choice, key;

    int my_mask_sandman[64] = {0,1,2,3,4,5,6,7,32,33,34,35,36,37,38,39,
        8,9,10,11,12,13,14,15,40,41,42,43,44,45,46,47,16,17,18,19,
        20,21,22,23,48,49,50,51,52,53,54,55,24,25,26,27,28,29,30,
        31,56,57,58,59,60,61,63};
	
    //int my_mask_dunnington[24] = {0,1,2,12,13,14,3,4,5,15,16,17,6,7,8,18,
    //    19,20,9,10,11,21,23,23};

    //setaffinity_oncpu(my_mask_dunnington[(params->tid)%24]);
    //setaffinity_oncpu(my_mask_sandman[(params->tid)%64]);
    setaffinity_oncpu(params->tid);

	srand48_r(params->tid * clargs.thread_seed, &drand_buffer);
	tsc_init(&params->insert_lock_set_tsc);

    params->enq_sum=params->deq_sum=0;

	pthread_barrier_wait(&barrier);
	if (params->tid == 0)
		timer_start(wall_timer);
	pthread_barrier_wait(&barrier);

#ifdef FC_DEDICATED
    if (params->tid ==0) enqueue(&Q,0,params);
    else if (params->tid ==1) {
        int deq;
        dequeue(&Q,&deq,params);
    }
    else{

#endif
	for (i=0; i < params->nr_operations; i++) {
#if defined(WORKLOAD_TIME) || defined (WORKLOAD_ONE_THREAD)
		if (params->time_to_leave)
			break;
#endif
		params->operations++;

		lrand48_r(&drand_buffer, &drand_res);
		choice = drand_res % 100;
		lrand48_r(&drand_buffer, &drand_res);
		key = drand_res % clargs.max_key;

		if (choice < clargs.enqueue_frac) {         /* enqueue   */
			params->enqueues++;
			enqueue(&Q, key, params);
            params->enq_sum+=key;
            //params->value_sum += key;
            
		} else {                                   /* deletion */
			params->dequeues++;
			if (dequeue(&Q, &key, params)) params->deq_sum+=key;
            //params->value_sum -= key;
            
		}
	}
#ifdef FC_DEDICATED
    }
    if (params->tid > 1 ) __sync_fetch_and_add(&finished_flag,1);
#endif
	pthread_barrier_wait(&barrier);
	if (params->tid == 0)
		timer_stop(wall_timer);

	return NULL;
}

double pthreads_benchmark()
{
	params_t init_params;
	int nthreads = clargs.num_threads;
	unsigned nr_operations = 1000000; /* FIXME */
	int i;

	printf("Pthreads Benchmark...\n");
	printf("nthreads: %d nr_operations: %d ( %d %d)\n", clargs.num_threads, 
	       nr_operations, clargs.enqueue_frac,100 - clargs.enqueue_frac);
#ifdef WORKLOAD_TIME
	printf("run_time_sec: %d\n", clargs.run_time_sec);
#endif

	XMALLOC(threads, clargs.num_threads);
	XMALLOC(params, clargs.num_threads);

	/* Initialize the Hash Table */
	printf("Initializing \n");
	
    initialize(&Q, nthreads);
    
    init_params.tid=0;
    int res;
	for (i=0; i < clargs.init_insertions; i++) {
#ifdef FC_DEDICATED
        _enqueue(&Q, i);
#else
        enqueue(&Q, i, &init_params);
#endif
        total_enq_sum+=i;
        /*if(clargs.verify==1) {//FIXME
            if (res){
                elements_count++;
                total_value_sum+=i;
            }
        
        }
        */
	}

	wall_timer = timer_init();
	if (pthread_barrier_init(&barrier, NULL, nthreads)) {
		perror("pthread_barrier_init");
		exit(1);
	}

	for (i=0; i < nthreads; i++) {
		memset(&params[i], 0, sizeof(*params));
		params[i].tid = i;
#ifdef WORKLOAD_FIXED
    #ifdef FC_DEDICATED
		params[i].nr_operations = nr_operations / (nthreads-2);
    #else
		params[i].nr_operations = nr_operations / nthreads;
    #endif
#elif WORKLOAD_TIME
		params[i].nr_operations = UINT32_MAX;
#else
		params[i].nr_operations = nr_operations;
#endif
	}

	for (i=0; i < nthreads; i++)
		pthread_create(&threads[i], NULL, thread_fn, &params[i]);

#ifdef WORKLOAD_TIME
	sleep(clargs.run_time_sec);
	for (i=0; i < nthreads; i++)
		params[i].time_to_leave = 1;
#endif
	
	for (i=0; i < nthreads; i++)
		pthread_join(threads[i], NULL);

	params_print();

#ifdef WORKLOAD_FIXED
	printf("\033[31mOps/usec:\033[0m %2.4lf\n",
	       (double) nr_operations / (timer_report_sec(wall_timer) * 1000000));
	printf("\n");
#endif
    

	if (pthread_barrier_destroy(&barrier)) {
		perror("pthread_barrier_destroy");
		exit(1);
	}

	//print_set_length(&Q);
	
    return timer_report_sec(wall_timer);
        }
