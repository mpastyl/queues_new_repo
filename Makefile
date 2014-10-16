#CC = /various/common_tools/gcc-4.8.2/bin/gcc-4.8.2
CC = gcc
CFLAGS = -Wall -Wextra -g

CFLAGS += -pthread
WORKLOAD = -DWORKLOAD_TIME
#WORKLOAD = -DWORKLOAD_FIXED
CFLAGS += $(WORKLOAD)
CFLAGS += -DCPU_MHZ_SH=\"./cpu_mhz.sh\"

#CFLAGS += -DVERBOSE_STATISTICS

#LDFLAGS = -L/home/users/jimsiak/local/lib/ -ltcmalloc

PROGS =  main.global_lock main.msqueue_noaba main.msqueue_aba main.optimistic main.fc_queue main.fc_dedicated main.fc_hybrid main.fc_one_word

all: $(PROGS)

SRC = main.c clargs.c parallel_benchmarks.c aff.c

main.global_lock: $(SRC)
	$(CC) $(CFLAGS) -DGLOBAL_LOCK -o $@ $^

main.msqueue_noaba: $(SRC)
	$(CC) $(CFLAGS) -DMSQUEUE -o $@ $^

main.msqueue_aba: $(SRC)
	$(CC) $(CFLAGS) -mcx16 -DMSQUEUE_ABA -o $@ $^

main.optimistic: $(SRC)
	$(CC) $(CFLAGS) -mcx16 -DOPTIMISTIC -o $@ $^

main.fc_queue: $(SRC)
	$(CC) $(CFLAGS) -DFC_QUEUE -o $@ $^

main.fc_one_word: $(SRC)
	$(CC) $(CFLAGS) -DFC_ONE_WORD -o $@ $^

main.fc_dedicated: $(SRC)
	$(CC) $(CFLAGS) -DFC_DEDICATED -o $@ $^

main.fc_hybrid: $(SRC)
	$(CC) $(CFLAGS) -DFC_HYBRID -o $@ $^

clean:
	rm -f $(PROGS)
