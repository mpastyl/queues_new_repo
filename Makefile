CC = /various/common_tools/gcc-4.8.2/bin/gcc-4.8.2
CFLAGS = -Wall -Wextra -g 

CFLAGS += -pthread
WORKLOAD = -DWORKLOAD_TIME
#WORKLOAD = -DWORKLOAD_FIXED
CFLAGS += $(WORKLOAD)
CFLAGS += -DCPU_MHZ_SH=\"./cpu_mhz.sh\"

#CFLAGS += -DVERBOSE_STATISTICS

#LDFLAGS = -L/home/users/jimsiak/local/lib/ -ltcmalloc

PROGS =  main.global_lock main.msqueue main.optimistic

all: $(PROGS)

SRC = main.c clargs.c parallel_benchmarks.c aff.c

main.global_lock: $(SRC)
	$(CC) $(CFLAGS) -DGLOBAL_LOCK -o $@ $^

main.msqueue: $(SRC)
	$(CC) $(CFLAGS) -DMSQUEUE -o $@ $^

main.optimistic: $(SRC)
	$(CC) $(CFLAGS) -mcx16 -DOPTIMISTIC -o $@ $^

clean:
	rm -f $(PROGS)
