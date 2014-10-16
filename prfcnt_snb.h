/**
 * prfcnt_sandybridge.h -- prfcnt implementation for Intel Core
 *
 * Copyright (C) 2007-2011, Computing Systems Laboratory (CSLab), NTUA
 * Copyright (C) 2007-2011, Kornilios Kourtis
 * Copyright (C) 2011,      Vasileios Karakasis
 * All rights reserved.
 *
 * This file is distributed under the BSD License. See LICENSE.txt for details.
**/

#ifndef __PRFCNT_SANDYBRIDGE__
#define __PRFCNT_SANDYBRIDGE__

#include "prfcnt_common.h"

#define DESC_MAX_LENGTH 100

/**
 * for Intel processors.
 *
 * For now We place them in the same file with the non-architectural
 * FIXME: (Core - Specific) Events.
 *
 * NOTE: One can use the cpuid instruction to determine at
 * runtime things as number of counters per logical
 * processor, architectural events that are available
 * etc. Check cpuid.c.
 *
 * Here the definitions will be static.
**/

#define IA32_PERF_GLOBAL_CTRL 0x38f
#define IA32_FIXED_CTR_CTRL 0x38d
#define IA32_FIXED_CTR0_ENABLE_SHIFT 32

#define PRFCNT_EVNT_BADDR 0x186 /* event selectors base address */
#define PRFCNT_CNTR_BADDR 0x0c1 /* event counter base address */
#define PRFCNT_UNCORE_BADDR 0x107cc /* FIXME: uncore event counter base address (not used in SNB)*/
#define PRFCNT_FIXED_BADDR 0x309 /* fixed counters base address */

/**
 * selector registers fields
 **/
#define EVNTSEL_CNTMASK_SHIFT      24
#define EVNTSEL_INVERT_SHIFT       23
#define EVNTSEL_ENABLE_CNTR_SHIFT  22
#define EVNTSEL_ENABLE_APIC_SHIFT  20
#define EVNTSEL_PIN_CTRL_SHIFT     19
#define EVNTSEL_EDGE_DETECT_SHIFT  18
#define EVNTSEL_OS_SHIFT           17
#define EVNTSEL_USR_SHIFT          16
#define EVNTSEL_UNIT_SHIFT         8
#define EVNTSEL_EVENT_SHIFT        0

#define EVNTSEL_SET(what, val) ( val ## ULL << (EVNTSEL_ ## what ## _SHIFT) )

#define EVNT_UNIT_CORE_ALL       ((1<<6) | (1<<7))
#define EVNT_UNIT_CORE_THIS      ((1<<6) | (0<<7))
#define EVNT_UNIT_AGENT_ALL      (1<<5)
#define EVNT_UNIT_AGENT_THIS     (0<<5)
#define EVNT_UNIT_PFETCH_ALL     ((1<<4) | (1<<5))
#define EVNT_UNIT_PFETCH_HW      ((1<<4) | (0<<5))
#define EVNT_UNIT_PFETCH_NOHW    ((0<<4) | (0<<5))
#define EVNT_UNIT_MESI_INVALID   (1<<0)
#define EVNT_UNIT_MESI_SHARED    (1<<1)
#define EVNT_UNIT_MESI_EXCLUSIVE (1<<2)
#define EVNT_UNIT_MESI_MODIFIED  (1<<3)
#define EVNT_UNIT_MESI_ALL       ( EVNT_UNIT_MESI_INVALID   |\
		                                   EVNT_UNIT_MESI_SHARED    |\
		                                   EVNT_UNIT_MESI_EXCLUSIVE |\
		                                   EVNT_UNIT_MESI_MODIFIED )

/** 
 * FIXME: this is only needed in order to use prfcnt_simple.h.
 *        All events are TYPE_PROGRAMMABLE right now.         
 **/
enum {
	EVNT_TYPE_PROGRAMMABLE, /* programmable event */
	EVNT_TYPE_FIXED,        /* fixed event */
	EVNT_TYPE_UNCORE,       /* event of the uncore of the
	                         * Intel Netburst/Core architectures */
};

struct prfcnt_snb_event {
	__u64           evntsel_data;
	__u32           cntr_nr;
	unsigned long   flags;
	char            desc[DESC_MAX_LENGTH];
	int             type; /* FIXME: see comment above. */
};
typedef struct prfcnt_snb_event prfcnt_event_t;

struct prfcnt_snb_handle {
	int	            msr_fd;
	unsigned int    cpu;
	unsigned long   flags;
	prfcnt_event_t  **events;
	unsigned int    events_nr;
};
typedef struct prfcnt_snb_handle prfcnt_t;

/*
 * Pre-define Architectural (cross-platform) Performance Eventsvailable Events:
 * To add an Event you must first add a symbolic name
 * for it in the enum structure and then add the appropriate
 * details in __evnts[] array.
 * Events Reference:
 *  IA32 Software Developer's Manual: Volume 3
 *  FIXME: Performance Monitoring Events for INTEL CORE and INTEL CORE
 *  DUO PROCESSORS
 *  (Tables A-{8,9} )
 */

enum {
	/* Fixed Counters */
	EVENT_FIXED_INSTR_RETIRED = 0,
	EVENT_FIXED_UNHLT_CORE_CYCLES,
	EVENT_FIXED_UNHLT_CORE_CYCLES_REF,

	/* Architectural Events */
	EVENT_INSTR_RETIRED,  
	EVENT_UNHLT_CORE_CYCLES,
	EVENT_UNHLT_CORE_CYCLES_REF_XCLK,
	EVENT_LLC_REFERENCE,
	EVENT_LLC_MISSES,
	EVENT_BR_RETIRED,
	EVENT_MISPRED_BR_RETIRED,

	/* Non-Architectural Events */ 
	EVENT_MEM_LOAD_UOPS_LLC_MISS_RETIRED_LOCAL_DRAM,
	EVENT_MEM_LOAD_UOPS_LLC_MISS_RETIRED_REMOTE_DRAM,
	EVENT_OFFCORE_RQSTS_DEMAND_DATA_RD,
	EVENT_OFFCORE_RQSTS_DEMAND_ALL_DATA_RD,
	EVENT_MEM_LOAD_UOPS_MISC_RETIRED_LLC_MISS,
	EVENT_FP_COMP_OPS_EXE_X87,
	EVENT_FP_COMP_OPS_EXE_SSE_PACKED_DOUBLE,
	EVENT_FP_COMP_OPS_EXE_SSE_SCALAR_SINGLE,
	EVENT_FP_COMP_OPS_EXE_SSE_PACKED_SINGLE,
	EVENT_FP_COMP_OPS_EXE_SSE_SCALAR_DOUBLE,
	EVENT_SIMD_FP_256_PACKED_SINGLE,
	EVENT_SIMD_FP_256_PACKED_DOUBLE,
	EVENT_L1D_EVICTION,
	EVENT_L2_TRANS_L1D_WB,

	L1D_WB_L2_MESI,
	L2_DATA_RQSTS_ANY,
	L2_TRANSACTIONS_L1D_WB,

	EVENT_END
};
static prfcnt_event_t __evnts[]={

	/**
	 * Fixed Counters
	 **/
	{	
		0,
		0, 0,
		"Instructions Retired",
		EVNT_TYPE_FIXED
	},
	{	
		0,
		0, 0,
		"Unhalted Core Cycles",
		EVNT_TYPE_FIXED
	},
	{	
		0,
		0, 0,
		"Unhalted Reference Core Cycles",
		EVNT_TYPE_FIXED
	},

	/**
	 * Architectural Events
	 **/
	//EVENT_INSTR_RETIRED
	{
		EVNTSEL_SET(EVENT, 0xc0)    |
		EVNTSEL_SET(UNIT, 0x00)     |
		EVNTSEL_SET(USR, 1),
		0, 0,
		"Instructions Retired",
		EVNT_TYPE_PROGRAMMABLE
	},
	//EVENT_UNHLT_CORE_CYCLES
	{	
		EVNTSEL_SET(EVENT, 0x3c)    |
		EVNTSEL_SET(UNIT, 0x00)     |
		EVNTSEL_SET(USR, 1),
		0, 0,
		"Unhalted Core Cycles",
		EVNT_TYPE_PROGRAMMABLE
	},
	//EVENT_UNHLT_CORE_CYCLES_REF_XCLK
	{	
		EVNTSEL_SET(EVENT, 0x3c)    |
		EVNTSEL_SET(UNIT, 0x01)     |
		EVNTSEL_SET(USR, 1),
		0, 0,
		"Unhalted Reference Core Cycles",
		EVNT_TYPE_PROGRAMMABLE
	},
	//EVENT_LLC_REFERENCE
	{
		EVNTSEL_SET(EVENT, 0x2e)    |
		EVNTSEL_SET(UNIT, 0x4f)     |
		EVNTSEL_SET(USR, 1),
		0, 0,
		"Last Level Cache references",
		EVNT_TYPE_PROGRAMMABLE
	},
	//EVENT_LLC_MISSES
	{
		EVNTSEL_SET(EVENT, 0x2e)    |
		EVNTSEL_SET(UNIT, 0x41)     |
		EVNTSEL_SET(USR, 1),
		0, 0,
		"Last Level cache misses",
		EVNT_TYPE_PROGRAMMABLE
	},
	//EVENT_BR_RETIRED
	{
		EVNTSEL_SET(EVENT, 0xc4)    |
		EVNTSEL_SET(UNIT, 0x00)     |
		EVNTSEL_SET(USR, 1),
		0, 0,
		"Branch Instruction Retired",
		EVNT_TYPE_PROGRAMMABLE
	},
	//EVENT_MISPRED_BR_RETIRED
	{
		EVNTSEL_SET(EVENT, 0xc5)    |
		EVNTSEL_SET(UNIT, 0x00)     |
		EVNTSEL_SET(USR, 1),
		0, 0,
		"Mispredicted Branch Instruction Retired",
		EVNT_TYPE_PROGRAMMABLE
	},

	/**
	 * Non-Architectural Events
	 **/
	//EVENT_MEM_LOAD_UOPS_LLC_MISS_RETIRED_LOCAL_DRAM
	{
		EVNTSEL_SET(EVENT, 0xd3)    |
		EVNTSEL_SET(UNIT, 0x01)     |
		EVNTSEL_SET(USR, 1),
		0, 0, 
		"LLC miss but serviced by Local DRAM",
		EVNT_TYPE_PROGRAMMABLE
	},
	//EVENT_MEM_LOAD_UOPS_LLC_MISS_RETIRED_REMOTE_DRAM
	{
		EVNTSEL_SET(EVENT, 0xd3)    |
		EVNTSEL_SET(UNIT, 0x04)     |
		EVNTSEL_SET(USR, 1),
		0, 0,
		"LLC miss but serviced by Remote DRAM",
		EVNT_TYPE_PROGRAMMABLE
	},
	//EVENT_OFFCORE_RQSTS_DEMAND_DATA_RD
	{
		EVNTSEL_SET(EVENT, 0xb0)    |
		EVNTSEL_SET(UNIT, 0x01)     |
		EVNTSEL_SET(USR, 1),
		0, 0,
		"Demand data read requests sent to uncore",
		EVNT_TYPE_PROGRAMMABLE
	},
	//EVENT_OFFCORE_RQSTS_DEMAND_ALL_DATA_RD
	{
		EVNTSEL_SET(EVENT, 0xb0)    |
		EVNTSEL_SET(UNIT, 0x08)     |
		EVNTSEL_SET(USR, 1),
		0, 0,
		"Data read requests sent to uncore (demand and prefetch)",
		EVNT_TYPE_PROGRAMMABLE
	},
	//EVENT_MEM_LOAD_UOPS_MISC_RETIRED_LLC_MISS
	{
        EVNTSEL_SET(EVENT, 0xd4)    |
        EVNTSEL_SET(UNIT, 0x02)     |
        EVNTSEL_SET(USR, 1),
        0, 0,
        "FIXME", //FIXME
        EVNT_TYPE_PROGRAMMABLE
    },
	//EVENT_FP_COMP_OPS_EXE_X87,
	{
        EVNTSEL_SET(EVENT, 0x10)    |
        EVNTSEL_SET(UNIT, 0x01)     |
        EVNTSEL_SET(USR, 1),
        0, 0,
        "Number of x87 uops executed",
        EVNT_TYPE_PROGRAMMABLE
	},
	//EVENT_FP_COMP_OPS_EXE_SSE_PACKED_DOUBLE,
	{
        EVNTSEL_SET(EVENT, 0x10)    |
        EVNTSEL_SET(UNIT, 0x10)     |
        EVNTSEL_SET(USR, 1),
        0, 0,
        "Number of SSE* double precision FP packed uops executed",
        EVNT_TYPE_PROGRAMMABLE
	},
	//EVENT_FP_COMP_OPS_EXE_SSE_SCALAR_SINGLE,
	{
        EVNTSEL_SET(EVENT, 0x10)    |
        EVNTSEL_SET(UNIT, 0x20)     |
        EVNTSEL_SET(USR, 1),
        0, 0,
        "Number of SSE* single precision FP scalar uops executed",
        EVNT_TYPE_PROGRAMMABLE
	},
	//EVENT_FP_COMP_OPS_EXE_SSE_PACKED_SINGLE,
	{
        EVNTSEL_SET(EVENT, 0x10)    |
        EVNTSEL_SET(UNIT, 0x40)     |
        EVNTSEL_SET(USR, 1),
        0, 0,
        "Number of SSE* single precision FP packed uops executed",
        EVNT_TYPE_PROGRAMMABLE
    },
	//EVENT_FP_COMP_OPS_EXE_SSE_SCALAR_DOUBLE,
	{
        EVNTSEL_SET(EVENT, 0x10)    |
        EVNTSEL_SET(UNIT, 0x80)     |
        EVNTSEL_SET(USR, 1),
        0, 0,
        "Number of SSE* double precision FP scalar uops executed",
        EVNT_TYPE_PROGRAMMABLE
    },
	//EVENT_SIMD_FP_256_PACKED_SINGLE,
	{
        EVNTSEL_SET(EVENT, 0x11)    |
        EVNTSEL_SET(UNIT, 0x01)     |
        EVNTSEL_SET(USR, 1),
        0, 0,
        "256-bit packed single-precision floating-point instructions",
        EVNT_TYPE_PROGRAMMABLE
    },
	//EVENT_SIMD_FP_256_PACKED_DOUBLE,
	{
        EVNTSEL_SET(EVENT, 0x11)    |
        EVNTSEL_SET(UNIT, 0x02)     |
        EVNTSEL_SET(USR, 1),
        0, 0,
        "256-bit packed double-precision floating-point instructions",
        EVNT_TYPE_PROGRAMMABLE
    },
	//EVENT_L1D_EVICTION,
	{
        EVNTSEL_SET(EVENT, 0x51)    |
        EVNTSEL_SET(UNIT, 0x04)     |
        EVNTSEL_SET(USR, 1),
        0, 0,
		"Number of modified lines evicted from the L1 data cache due to "
		"replacement", 
        EVNT_TYPE_PROGRAMMABLE
    },
	//EVENT_L2_TRANS_L1D_WB,
	{
        EVNTSEL_SET(EVENT, 0xf0)    |
        EVNTSEL_SET(UNIT, 0x10)     |
        EVNTSEL_SET(USR, 1),
        0, 0,
		"L1D writebacks that access L2 cache",
        EVNT_TYPE_PROGRAMMABLE
    },
	//L1D_WB_L2_MESI
	{
        EVNTSEL_SET(EVENT, 0x28)    |
        EVNTSEL_SET(UNIT, 0x0f)     |
        EVNTSEL_SET(USR, 1),
        0, 0,
		"L1D_WB_L2_MESI",
        EVNT_TYPE_PROGRAMMABLE
    },
	//L2_DATA_RQSTS_ANY
	{
        EVNTSEL_SET(EVENT, 0x26)    |
        EVNTSEL_SET(UNIT, 0xff)     |
        EVNTSEL_SET(USR, 1),
        0, 0,
		"L2_DATA_RQSTS_ANY",
        EVNT_TYPE_PROGRAMMABLE
    },
	//L2_TRANSACTIONS_L1D_WB
	{
        EVNTSEL_SET(EVENT, 0xf6)    |
        EVNTSEL_SET(UNIT, 0x10)     |
        EVNTSEL_SET(USR, 1),
        0, 0,
		"L2_TRANSACTIONS_L1D_WB",
        EVNT_TYPE_PROGRAMMABLE
    },
};

/**
 * Select Events Here
 * (The number of available events can be determined using the cpuid
 * intruction)
**/
static const int __evnts_selected[] = {
	/* Fixed Counters */
	EVENT_FIXED_INSTR_RETIRED,
	EVENT_FIXED_UNHLT_CORE_CYCLES,
	EVENT_FIXED_UNHLT_CORE_CYCLES_REF,

	/* Architectural */
//	EVENT_INSTR_RETIRED,  
//	EVENT_UNHLT_CORE_CYCLES,
//	EVENT_UNHLT_CORE_CYCLES_REF_XCLK,
	EVENT_LLC_REFERENCE,
	EVENT_LLC_MISSES,
//	EVENT_BR_RETIRED,
//	EVENT_MISPRED_BR_RETIRED,

	/* Non-Architectural Events */ 
//	EVENT_CPU_CLK_UNHALTED_THREAD_P,
//	EVENT_LONGEST_LAT_CACHE_MISS,
//	EVENT_MEM_LOAD_UOPS_LLC_MISS_RETIRED_LOCAL_DRAM,
//	EVENT_MEM_LOAD_UOPS_LLC_MISS_RETIRED_REMOTE_DRAM,
//	EVENT_OFFCORE_RQSTS_DEMAND_DATA_RD,
//	EVENT_OFFCORE_RQSTS_DEMAND_ALL_DATA_RD,
//	EVENT_FP_COMP_OPS_EXE_X87,
//	EVENT_FP_COMP_OPS_EXE_SSE_PACKED_DOUBLE,
//	EVENT_FP_COMP_OPS_EXE_SSE_SCALAR_SINGLE,
//	EVENT_FP_COMP_OPS_EXE_SSE_PACKED_SINGLE,
//	EVENT_FP_COMP_OPS_EXE_SSE_SCALAR_DOUBLE,
//	EVENT_SIMD_FP_256_PACKED_SINGLE,
//	EVENT_SIMD_FP_256_PACKED_DOUBLE,
//	EVENT_L1D_EVICTION,
//	EVENT_L2_TRANS_L1D_WB,
//	L1D_WB_L2_MESI,
//	L2_DATA_RQSTS_ANY,
//	L2_TRANSACTIONS_L1D_WB
};

#include "prfcnt_simple.h"

#endif /* __PRFCNT_SANDYBRIDGE__ */
