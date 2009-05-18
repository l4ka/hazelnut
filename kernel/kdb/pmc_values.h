/*********************************************************************
 *                
 * Copyright (C) 2001,  Karlsruhe University
 *                
 * File path:     pmc_values.h
 * Description:   Definition macros for performance counters the x86
 *                PentiumPro family of processors.  (What's the point
 *                of this file?)
 *                
 * @LICENSE@
 *                
 * $Id: pmc_values.h,v 1.2 2001/11/22 12:13:54 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __pmc_values_h__
#define __pmc_values_h__

/* Performance counter registers */
#define PERFCTR0 193
#define PERFCTR1 194
#define PERFEVENTSEL0 390
#define PERFEVENTSEL1 391

/* 
 * All these #defines come from the 
 * Intel Archictecture Software Developer's Manual, Volume 3
 * available from http://developer.intel.com
 * which has more information about these counters.
*/

/* event masks */
#define PMC_USER_MASK (1 << 16)
#define PMC_OS_MASK (1 << 17)
#define PMC_OCCURENCE_MASK (1 << 18)
#define PMC_ENABLE_MASK ((1 << 22)|(1<<20))

/* P6 Family Processor performance-monitoring events */
/* (DCU) Unit */

/* All memory references, both cacheable and noncacheable. */
#define DATA_MEM_REFS (0x0043)

/* Total lines allocated in the DCU. */
#define DCU_LINES_IN (0x0045)

/* Number of M state lines allocated in the DCU. */
#define DCU_M_LINES_IN (0x0046) 

/* Number of M state lines evicted from the DCU. This includes evictions via 
   snoop HITM, intervention or replacement. */

#define DCU_M_LINES_OUT (0x0047) 

/*
Weighted number of cycles while a DCU miss is outstanding. An access
that also misses the L2 is short-changed by 2 cycles (i.e., if counts
N cycles, should be N+2 cycles). Subsequent loads to the same cache
line will not result in any additional counts. Count value not
precise, but still useful.  
*/
#define DCU_MISS_OUTSTANDING (0x0048)

/* Instruction Fetch Unit (IFU) */

/* Number of instruction fetches, both cacheable and noncacheable. */
#define IFU_IFETCH (0x0080) 

/* Number of instruction fetch misses. */
#define IFU_IFETCH_MISS (0x0081) 

/* Number of ITLB misses. */
#define ITLB_MISS (0x0085)

/* Number of cycles that the instruction fetch pipe stage is stalled, including cache misses, ITLB misses, ITLB faults, and victimcache evictions. */
#define IFU_MEM_STALL (0x0086) 

/* Number of cycles that the instruction length decoder is stalled. */
#define ILD_STALL (0x0087)

/* L2 Cache events */

/* Number of L2 instruction fetches. */
#define L2_IFETCH (0xf28) 

/* Number of L2 data loads. */
#define L2_LD (0x0f29)

/* Number of L2 data stores. */
#define L2_ST (0x0f2A)


/* Number of lines allocated in the L2. */
#define L2_LINES_IN (0x24)

/* Number of lines removed from the L2 for any reason. */
#define L2_LINES_OUT (0x26)

/* Number of modified lines allocated in the L2. */
#define L2_M_LINES_INM (0x25)

/* Number of modified lines removed from the L2 for any reason. */
#define L2_M_LINES_OUTM (0x27)

/* Number of L2 requests. */
#define L2_RQSTS (0x0f2E)

/* Number of L2 address strobes. */
#define L2_ADS (0x21)

/* Number of cycles during which the data bus was busy. */
#define L2_DBUS_BUSY (0x22)

/* Number of cycles during which the data bus was busy transferring data from L2 to the processor. */
#define L2_DBUS_BUSY_RD (0x23)

/* External Bus Logic (EBL) */

/* Number of clocks during which DRDY is asserted by CPU. */
#define BUS_DRDY_CLOCKS_SELF (0x62)

/* (Any) Number of clocks during which DRDY is asserted by any agent. */
#define BUS_DRDY_CLOCKS_ANY (0x2062)

/* Number of clocks during which LOCK is asserted. Always counts in processor clocks. */
#define BUS_LOCK_CLOCKS_SELF (0x63)

/* Number of clocks during which LOCK is asserted. Always counts in processor clocks.  */
#define BUS_LOCK_CLOCKS_ANY  (0x2063) 

/* Number of bus requests outstanding. Counts only DCU full- line cacheable reads, not RFOs, writes, instruction fetches, or anything else. Counts "waiting for bus to complete" (last data chunk received). 
 */
#define BUS_REQ_OUTSTANDING_SELF (0x60)

/* Number of bus requests outstanding. Counts only DCU full- line cacheable reads, not RFOs, writes, instruction fetches, or anything else. Counts "waiting for bus to complete" (last data chunk received). 
 */
#define BUS_REQ_OUTSTANDING_ANY (0x2060)

/* Number of burst read transactions. */
#define BUS_TRAN_BRD_SELF (0x65)

/* Number of burst read transactions. */
#define BUS_TRAN_BRD_ANY (0x2065)

/* Number of read for ownership transactions. */
#define BUS_TRAN_RFO (0x66) 

/* Number of write back transactions. */
#define BUS_TRANS_WB (0x67) 

/* Number of instruction fetch transactions. */
#define BUS_TRAN_IFETCH (0x68) 

/* Number of invalidate transactions. */
#define BUS_TRAN_INVAL (0x69) 

/* Number of partial write transactions. */
#define BUS_TRAN_PWR (0x6A) 

/* Number of partial transactions. */
#define BUS_TRANS_P (0x6B) 

/* Number of I/O transactions. */
#define BUS_TRANS_IO (0x6C) 

/* Number of deferred transactions. */
#define BUS_TRAN_DEF (0x6D) 

/* Number of burst transactions. */
#define BUS_TRAN_BURST (0x6E) 

/* Number of all transactions. */
#define BUS_TRAN_ANY (0x70) 

/* Number of memory transactions. */
#define BUS_TRAN_MEM (0x6F) 

/* Number of bus clock cycles during which this processor is receiving data.  */
#define BUS_DATA_RCV (0x64)

/* Number of bus clock cycles during which this processor is driving the BNR pin.  */
#define BUS_BNR_DRV (0x61)

/* Number of bus clock cycles during which this processor is driving the HIT pin.  Includes cycles due to snoop stalls. */
#define BUS_HIT_DRV (0x7A)

/* Number of bus clock cycles during which this processor is driving the HITM pin. Includes cycles due to snoop stalls. */
#define BUS_HITM_DRV (0x7B)

/* (Self) Number of clock cycles during which the bus is snoop stalled. */
#define BUS_SNOOP_STALL (0x7E)

/* Floating- Point Unit */

/* Number of computational floating-point operations retired. Counter 0 only */
#define FLOPS (0xC1)

/* Number of computational floating-point operations executed. Counter 0 only. */
#define FP_COMP_OPS_EXE (0x10)

/* Number of floating- point exception cases handled by microcode. Counter 1 only. */
#define FP_ASSIST (0x11)

/* Number of multiplies. Counter 1 only. */
#define MUL (0x12)

/* Number of divides. Counter 1 only. */
#define DIV (0x13)

/*  Number of cycles during which the divider is busy. Counter 0 only. */
#define CYCLES_DIV_BUSY (0x14)

/* Memory Ordering */

/* Number of store buffer blocks. */
#define LD_BLOCKS (0x03)

/* Number of store buffer drain cycles. */
#define SB_DRAINS (0x04)

/* Number of misaligned data memory references. */
#define MISALIGN_MEM_REF (0x05)

/* Instruction Decoding and Retirement */

/* Number of instructions retired. */
#define INST_RETIRED (0xC0)

/* Number of UOPs retired. */
#define UOPS_RETIRED (0xC2)

/* Number of instructions decoded. */
#define INST_DECODER (0xD0)

/*Interrupts */

/* Number of hardware interrupts received. */
#define HW_INT_RX (0xC8)

/* Number of processor cycles for which interrupts are disabled. */
#define CYCLES_INT_MASKED (0xC6)

/* Number of processor cycles for which interrupts are disabled and interrupts are pending. */
#define CYCLES_INT_PENDING_AND_MASKED (0xC7)

/* Branches */

 /* Number of branch instructions retired. */
#define BR_INST_RETIRED (0xC4)

 /* Number of mispredicted branches retired. */
#define BR_MISS_PRED_RETIRED (0xC5)

 /* Number of taken branches retired. */
#define BR_TAKEN_RETIRED (0xC9)

 /* Number of taken mispredictions branches retired. */
#define BR_MISS_PRED_NRET (0xCA)

 /* Number of branch instructions decoded. */
#define BR_INST_DECODED (0xE0)

 /* Number of branches that miss the BTB. */
#define BTB_MISSES (0xE2)

 /* Number of bogus branches. */
#define BR_BOGUS (0xE4)

 /* Number of time BACLEAR is asserted. */
#define BACLEARS (0xE6)

/* Stalls */

 /* Number of cycles during which there are resource related stalls. */
#define RESOURCE_STALLS (0xA2)

 /* Number of cycles or events for partial stalls. */
#define PARTIAL_RAT_STALLS (0xD2)

/* Segment Register Loads */

/* Number of segment register loads. */
#define SEGMENT_REG_LOADS (0x06)

/*Clocks */

/* Number of cycles during which the processor is not halted. */
#define CPU_CLK_UNHALTED (0x79)

#endif /* __pmc_values_h__ */
