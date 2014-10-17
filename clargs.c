#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h> /* INT32_MAX */
#include "clargs.h"

/* Default command line arguments */
#define ARGUMENT_DEFAULT_NUM_THREADS 1
#define ARGUMENT_DEFAULT_INIT_INSERTIONS 64
#define ARGUMENT_DEFAULT_MAX_KEY INT32_MAX
#define ARGUMENT_DEFAULT_ENQUEUE_FRAC 50
#define ARGUMENT_DEFAULT_INIT_SEED 1024
#define ARGUMENT_DEFAULT_THREAD_SEED 128
#define ARGUMENT_DEFAULT_VERIFY 0
#define ARGUMENT_DEFAULT_BACKOFF 0
#ifdef WORKLOAD_TIME
#  define ARGUMENT_DEFAULT_RUN_TIME_SEC 5
#endif
#if defined(FC_QUEUE) ||defined(FC_ONE_WORD) ||defined(FC_DEDICATED) ||defined(FC_HYBRID)
#define ARGUMENT_DEFAULT_LOOP 1
#endif

static char *opt_string = "ht:s:m:l:r:e:j:z:v:b:o:";
static struct option long_options[] = {
	{ "help",            no_argument,       NULL, 'h' },
	{ "num-threads",     required_argument, NULL, 't' },
	{ "init-insertions",  required_argument, NULL, 'z' },
	{ "max-key",         required_argument, NULL, 'm' },
	{ "enqueue-frac",     required_argument, NULL, 'l' },
	/* FIXME better short options for these, or no short */
	{ "init-seed",       required_argument, NULL, 'e' },
	{ "thread-seed",     required_argument, NULL, 'j' },
    { "verify",          required_argument, NULL, 'v' },
	{ "backoff",         required_argument, NULL, 'b' },
#ifdef WORKLOAD_TIME
	{ "run-time-sec",    required_argument, NULL, 'r' },
#endif
#if defined(FC_QUEUE) ||defined(FC_ONE_WORD) ||defined(FC_DEDICATED) ||defined(FC_HYBRID)
	{ "loops",         required_argument, NULL, 'o' },
#endif
	{ NULL, 0, NULL, 0 }
};

clargs_t clargs = {
	ARGUMENT_DEFAULT_NUM_THREADS,
	ARGUMENT_DEFAULT_INIT_INSERTIONS,
	ARGUMENT_DEFAULT_MAX_KEY,
	ARGUMENT_DEFAULT_ENQUEUE_FRAC,
	ARGUMENT_DEFAULT_INIT_SEED,
	ARGUMENT_DEFAULT_THREAD_SEED,
    ARGUMENT_DEFAULT_VERIFY,
	ARGUMENT_DEFAULT_BACKOFF,
#ifdef WORKLOAD_TIME
	ARGUMENT_DEFAULT_RUN_TIME_SEC,
#endif
#if defined(FC_QUEUE) ||defined(FC_ONE_WORD) ||defined(FC_DEDICATED) ||defined(FC_HYBRID)
    ARGUMENT_DEFAULT_LOOP,
#endif   
};

static void clargs_print_usage(char *progname)
{
	printf("usage: %s [options]\n"
	       "  possible options:\n"
	       "    -h,--help  print this help message\n"
	       "    -t,--num-threads  number of threads [%d]\n"
		   "    -z,--init-insertions number of insertions to be performed for initialization [%d]\n"
		   "    -m,--max-key  max key to lookup,insert,delete [%d]\n"
	       "    -l,--enqueue-frac  enqueue fraction of operations [%d%%]\n"
	       "    -e,--init-seed    the seed that is used for the hash initializion [%d]\n"
	       "    -j,--thread-seed  the seed that is used for the thread operations [%d]\n"
           "    -v,--verify verify results at end of run [%d]\n",
           "    -b,--backoff [%d]\n",
	       progname, ARGUMENT_DEFAULT_NUM_THREADS,
		   ARGUMENT_DEFAULT_INIT_INSERTIONS,
	       ARGUMENT_DEFAULT_MAX_KEY, ARGUMENT_DEFAULT_ENQUEUE_FRAC,
		   ARGUMENT_DEFAULT_INIT_SEED, ARGUMENT_DEFAULT_THREAD_SEED,
           ARGUMENT_DEFAULT_VERIFY, ARGUMENT_DEFAULT_BACKOFF);

#ifdef WORKLOAD_TIME
	printf("    -r,--run-time-sec execution time [%d sec]\n",
			ARGUMENT_DEFAULT_RUN_TIME_SEC);
#endif
#if defined(FC_QUEUE) ||defined(FC_ONE_WORD) ||defined(FC_DEDICATED) ||defined(FC_HYBRID)
    printf("    -o,--loops number of loops when combining [%d]\n",
            ARGUMENT_DEFAULT_LOOP);
#endif 

}

void clargs_init(int argc, char **argv)
{
	char c;
	int i;

	while (1) {
		i = 0;
		c = getopt_long(argc, argv, opt_string, long_options, &i);
		if (c == -1)
			break;

		switch(c) {
		case 'h':
			clargs_print_usage(argv[0]);
			exit(1);
		case 't':
			clargs.num_threads = atoi(optarg);
			break;
		case 'z':
			clargs.init_insertions = atoi(optarg);
            break;
		case 'm':
			clargs.max_key = atoi(optarg);
			break;
		case 'l':
			clargs.enqueue_frac = atoi(optarg);
			break;
		case 'e':
			clargs.init_seed = atoi(optarg);
			break;
		case 'j':
			clargs.thread_seed = atoi(optarg);
			break;
        case 'v':
            clargs.verify = atoi(optarg);
            break;
		case 'b':
			clargs.backoff = atoi(optarg);
			break;
#ifdef WORKLOAD_TIME
		case 'r':
			clargs.run_time_sec = atoi(optarg);
			break;
#endif
#if defined(FC_QUEUE) ||defined(FC_ONE_WORD) ||defined(FC_DEDICATED) ||defined(FC_HYBRID)
		case 'o':
			clargs.loops = atoi(optarg);
			break;
#endif
		default:
			clargs_print_usage(argv[0]);
			exit(1);
		}
	}

	/* Sanity checks. */
	//assert(clargs.lookup_frac + clargs.insert_frac <= 100);
}

void clargs_print()
{
	printf("Inputs:\n"
	       "====================\n"
	       "  num_threads: %d\n"
		   "  init_insertions: %d\n"
	       "  max_key: %d\n"
	       "  enqueue_frac: %d\n"
	       "  init_seed: %d\n"
	       "  thread_seed: %d\n"
		   "  backof: %d\n",
	       clargs.num_threads, 
	       clargs.init_insertions, clargs.max_key,
	       clargs.enqueue_frac,
		   clargs.init_seed, clargs.thread_seed,
		   clargs.backoff);

#ifdef WORKLOAD_TIME
	printf("  run_time_sec: %d\n", clargs.run_time_sec);
#endif
#if defined(FC_QUEUE) ||defined(FC_ONE_WORD) ||defined(FC_DEDICATED) ||defined(FC_HYBRID)
    printf(" loops: %d\n",clargs.loops);    
#endif
}
