/*
 * prfcnt_simple.h -- common functions for Core and AMD Architectures
 *
 * Copyright (C) 2007-2011, Computing Systems Laboratory (CSLab), NTUA
 * Copyright (C) 2007-2011, Kornilios Kourtis
 * Copyright (C) 2011,      Vasileios Karakasis
 * All rights reserved.
 *
 * This file is distributed under the BSD License. See LICENSE.txt for details.
 */
#ifndef __PRFCNT_SIMPLE_H__
#define __PRFCNT_SIMPLE_H__

#include <stdint.h> /* uint64_t type */

/* quick n dirty solution to print cycles with other prfcnt metrices. */
static uint64_t prfcnt_ticks;

//static int level = -1;
//static int kk;
//#define func_trace(in) \
//	do {\
//		level += in; \
//		for (kk=0; kk < level; kk++) printf("    "); \
//		printf("---> %s %s\n", __func__, (in == 0) ? "leaving\n" : "entering"); \
//		if (in == 0) level--; \
//	} while(0)

/* Comment out for debuging. */
#define func_trace(in) \
	do {\
	} while (0)

static inline void prfcnt_cntr_setup(prfcnt_t *handle, prfcnt_event_t *event)
{
	__u64        evntsel_data;
	unsigned int counter_addr, evntsel_addr;

	func_trace(1);
	evntsel_data = event->evntsel_data;
	switch (event->type) {
	case EVNT_TYPE_PROGRAMMABLE:
		counter_addr = event->cntr_nr + PRFCNT_CNTR_BADDR;
		evntsel_addr = event->cntr_nr + PRFCNT_EVNT_BADDR;
//		evntsel_data &= ~(1<<EVNTSEL_ENABLE_CNTR_SHIFT);
		prfcnt_wrmsr(handle->msr_fd, evntsel_addr, evntsel_data);
		break;
	case EVNT_TYPE_FIXED:
		counter_addr = event->cntr_nr + PRFCNT_FIXED_BADDR;
//		evntsel_addr = IA32_PERF_GLOBAL_CTRL;
//		evntsel_data &= ~(1<<(IA32_FIXED_CTR0_ENABLE_SHIFT + event->cntr_nr));
//		prfcnt_wrmsr(handle->msr_fd, evntsel_addr, evntsel_data);
		break;
	case EVNT_TYPE_UNCORE:
		counter_addr = event->cntr_nr + PRFCNT_UNCORE_BADDR;
		break;
	default:
		fprintf(stderr, "prfcnt_cntr_setup(): unknown event type\n");
		exit(1);
	}

	prfcnt_wrmsr(handle->msr_fd, counter_addr, 0);
	func_trace(0);
}

static inline void prfcnt_cntr_start(prfcnt_t *handle, prfcnt_event_t *event)
{
	__u64        evntsel_data = event->evntsel_data;
	unsigned int evntsel_addr;

	func_trace(1);
	switch (event->type) {
	case EVNT_TYPE_PROGRAMMABLE:
		evntsel_addr = event->cntr_nr + PRFCNT_EVNT_BADDR;
		evntsel_data |= (1<<EVNTSEL_ENABLE_CNTR_SHIFT);
		prfcnt_wrmsr(handle->msr_fd, evntsel_addr, evntsel_data);
		break;
	case EVNT_TYPE_FIXED:
//		evntsel_addr = IA32_PERF_GLOBAL_CTRL;
//		evntsel_data |= 1<<(IA32_FIXED_CTR0_ENABLE_SHIFT + event->cntr_nr);
		break;
	case EVNT_TYPE_UNCORE:
//		  prfcnt_wrmsr(handle->msr_fd, 0x107d8, 0x4 | (1 << (16 + event->cntr_nr)));
		evntsel_addr = event->cntr_nr + PRFCNT_UNCORE_BADDR;
		prfcnt_wrmsr(handle->msr_fd, evntsel_addr, evntsel_data);
		break;
	default:
		fprintf(stderr, "prfcnt_cntr_start(): unknown event type\n");
		exit(1);
	}

	func_trace(0);
}

static inline void prfcnt_cntr_pause(prfcnt_t *handle, prfcnt_event_t *event)
{
	__u64        evntsel_data = event->evntsel_data;
	unsigned int evntsel_addr;

	func_trace(1);
	switch (event->type) {
	case EVNT_TYPE_PROGRAMMABLE:
		evntsel_addr = event->cntr_nr + PRFCNT_EVNT_BADDR;
		evntsel_data &= ~(1<<EVNTSEL_ENABLE_CNTR_SHIFT);
		prfcnt_wrmsr(handle->msr_fd, evntsel_addr, evntsel_data);
		break;
	case EVNT_TYPE_FIXED:
//		evntsel_addr = IA32_PERF_GLOBAL_CTRL;
//		evntsel_data &= ~(1<<(IA32_FIXED_CTR0_ENABLE_SHIFT + event->cntr_nr));
		break;
	case EVNT_TYPE_UNCORE:
		evntsel_addr = 0x107d8;
		evntsel_data = 0x1 | (1 << (16 + event->cntr_nr));
/*		   evntsel_addr = event->cntr_nr + PRFCNT_UNCORE_BADDR; */
/*		   evntsel_data &= 0xffffffffL; */
		prfcnt_wrmsr(handle->msr_fd, evntsel_addr, evntsel_data);
		break;
	default:
		fprintf(stderr, "prfcnt_cntr_pause(): unknown event type\n");
		exit(1);
	}

	func_trace(0);
}

static inline void prfcnt_init(prfcnt_t *handle, int cpu, unsigned long flags)
{
	int i;
	int nr_fixed, nr_programmable;

	func_trace(1);
	handle->cpu       = cpu;
	handle->flags     = flags;
	handle->events_nr = sizeof(__evnts_selected)/sizeof(int);
	handle->events    = (prfcnt_event_t **)malloc(handle->events_nr*sizeof(prfcnt_event_t *));
	if ( !handle->events ){
		perror("prfcnt_init");
		exit(1);
	}

	nr_fixed = nr_programmable = 0;
	for (i=0; i < handle->events_nr ; i++){
		prfcnt_event_t *event;
		event             = &__evnts[__evnts_selected[i]];
		switch (event->type) {
		case EVNT_TYPE_PROGRAMMABLE:
			event->cntr_nr = nr_programmable++;
			break;
		case EVNT_TYPE_FIXED:
			event->cntr_nr = nr_fixed++;
			break;
		case EVNT_TYPE_UNCORE:
			/* leave event counter number as set by user */
			break;
		default:
			fprintf(stderr, "prfcnt_cntr_pause(): unknown event type\n");
			exit(1);
		}

		handle->events[i] = event;
	}

	__prfcnt_init(cpu, &handle->msr_fd);

	/* initialize registers */
	prfcnt_wrmsr(handle->msr_fd, IA32_PERF_GLOBAL_CTRL, 0x0ULL);
	prfcnt_wrmsr(handle->msr_fd, IA32_FIXED_CTR_CTRL, 0x0ULL);
//	prfcnt_wrmsr(handle->msr_fd, IA32_FIXED_CTR_CTRL, 0x222ULL);
	for (i=0; i < nr_fixed; i++)
		prfcnt_wrmsr(handle->msr_fd, PRFCNT_FIXED_BADDR + i, 0x0ULL);

	for (i=0; i < nr_programmable; i++) {
		prfcnt_wrmsr(handle->msr_fd, PRFCNT_EVNT_BADDR + i, 0x0ULL);
		prfcnt_wrmsr(handle->msr_fd, PRFCNT_CNTR_BADDR + i, 0x0ULL);
		prfcnt_wrmsr(handle->msr_fd, PRFCNT_EVNT_BADDR + i, 1<<EVNTSEL_USR_SHIFT);
	}

	for (i=0; i < handle->events_nr; i++){
		prfcnt_cntr_setup(handle, handle->events[i]);
	}
	func_trace(0);
}

static inline void prfcnt_shut(prfcnt_t *handle)
{
	func_trace(1);
	__prfcnt_shut(handle->msr_fd);
	func_trace(0);
}

static inline void prfcnt_start(prfcnt_t *handle)
{
	int i;
//	__u64 flags = 0x7000000ff;
	__u64 flags = 0x70000000f;

	func_trace(1);
	for (i=0; i < handle->events_nr; i++){
		prfcnt_cntr_start(handle, handle->events[i]);
	}
	prfcnt_wrmsr(handle->msr_fd, IA32_FIXED_CTR_CTRL, 0x333ULL);
	prfcnt_wrmsr(handle->msr_fd, IA32_PERF_GLOBAL_CTRL, flags);
	func_trace(0);
}

static inline void prfcnt_pause(prfcnt_t *handle)
{
	int i;

	func_trace(1);
	for (i=0; i < handle->events_nr; i++){
		prfcnt_cntr_pause(handle, handle->events[i]);
	}
	prfcnt_wrmsr(handle->msr_fd, IA32_PERF_GLOBAL_CTRL, 0);
	func_trace(0);
}

static inline __u64 *prfcnt_readstats_rdmsr(prfcnt_t *handle, __u64 *stats)
{
	int i;
	unsigned int cntr_addr;

	func_trace(1);
	for (i=0; i < handle->events_nr; i++){
		int evnt_type = handle->events[i]->type;
		switch (evnt_type) {
		case EVNT_TYPE_PROGRAMMABLE:
			cntr_addr = handle->events[i]->cntr_nr + PRFCNT_CNTR_BADDR;
			break;
		case EVNT_TYPE_FIXED:
			cntr_addr = handle->events[i]->cntr_nr + PRFCNT_FIXED_BADDR;
			break;
		case EVNT_TYPE_UNCORE:
			cntr_addr = handle->events[i]->cntr_nr + PRFCNT_UNCORE_BADDR;
			break;
		default:
			fprintf(stderr, "prfcnt_readstats_rdmsr(): unknown event type\n");
			exit(1);
		}

		prfcnt_rdmsr(handle->msr_fd, cntr_addr, stats + i);
		if (evnt_type == EVNT_TYPE_UNCORE) {
			//printf("read: %#lx\n", stats[i]);
			stats[i] &= 0xffffffffL;
		}
	}
	func_trace(0);

	return stats;
}

static inline __u64  *prfcnt_readstats_rdpmc(prfcnt_t *handle, __u64 *stats)
{
	int i;

	func_trace(1);
	for (i=0; i < handle->events_nr; i++){
		if (handle->events[i]->type == EVNT_TYPE_PROGRAMMABLE)
			stats[i] = prfcnt_rdpmc(handle->events[i]->cntr_nr);
	}
	func_trace(0);

	return stats;
}

#define prfcnt_readstats prfcnt_readstats_rdmsr

static inline void prfcnt_report(prfcnt_t *handle)
{
	__u64 stats[EVENT_END];
	int   i;

	func_trace(1);
	prfcnt_readstats(handle, stats);

	printf("cpu %d:\n", handle->cpu);
	for (i=0; i < handle->events_nr; i++){
		printf("\t%50s:%12llu\n", handle->events[i]->desc, stats[i]);
	}
	func_trace(0);
}

static inline void prfcnt_freport_header(FILE *fp, prfcnt_t *handle)
{
	int i;

	func_trace(1);
	fprintf(fp, "cpu");
	fprintf(fp, " cycles"); /* cycles added */
	for (i=0; i < handle->events_nr; i++)
		fprintf(fp, " \"%s\"", handle->events[i]->desc);
	fprintf(fp, "\n");
	func_trace(0);
}

static inline void prfcnt_freport(FILE *fp, prfcnt_t *handle)
{
	__u64 stats[EVENT_END];
	int   i;

	func_trace(1);
	prfcnt_readstats(handle, stats);

	fprintf(fp, "%d", handle->cpu);
	fprintf(fp, " %lu", prfcnt_ticks); /* cycles added */
	for (i=0; i < handle->events_nr; i++){
		fprintf(fp, " %12llu", stats[i]);
	}
	fprintf(fp, "\n");
	func_trace(0);
}
#endif /* __PRFCNT_SIMPLE_H__ */
