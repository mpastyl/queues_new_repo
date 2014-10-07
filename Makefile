CC = gcc
CFLAGS = -Wall -Wextra -g 

CFLAGS += -pthread
WORKLOAD = -DWORKLOAD_TIME
#WORKLOAD = -DWORKLOAD_FIXED
CFLAGS += $(WORKLOAD)
CFLAGS += -DCPU_MHZ_SH=\"./cpu_mhz.sh\"

#CFLAGS += -DVERBOSE_STATISTICS

#LDFLAGS = -L/home/users/jimsiak/local/lib/ -ltcmalloc

PROGS =  main.global_lock

all: $(PROGS)

SRC = main.c clargs.c parallel_benchmarks.c aff.c

main.global_lock: $(SRC)
	$(CC) $(CFLAGS) -DGLOBAL_LOCK -o $@ $^


clean:
	rm -f $(PROGS)
