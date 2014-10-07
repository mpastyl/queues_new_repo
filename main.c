#include <stdio.h>
#include "parallel_benchmarks.h"
#include "clargs.h"

int main(int argc, char **argv)
{
	clargs_init(argc, argv);
	clargs_print();

	double time = pthreads_benchmark();
	printf("Time: %4.2lf sec\n", time);
	
	return 0;
}
