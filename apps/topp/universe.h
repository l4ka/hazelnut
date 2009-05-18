#ifndef __universe_h
#define __universe_h

#include "kip.h"
//#include "tcb.h"

/*
 * percentage of physical memory used for persistent threads
 */

#define PERCENTAGE 10
#define DEBUG 1

//some basics

#define INFO_BASE 0x7FFFF000
#define MTAB_BASE 0x80000000
#define PAGE_BITS 12
#define PAGE_SIZE (1 << PAGE_BITS)
#define PAGE_MASK (~(PAGE_SIZE-1))
#define BLOCK_SIZE 512
#define WORD_SIZE 32
#define STACK_SIZE 1024
#define INVALID_P 0xFFFFFFFF
#define TCB_AREA 0xe0000000
#define L4_NUMBITS_TCBS 10
#define TCB_SIZE (1 << L4_NUMBITS_TCBS)
#define TCB_AREA_SIZE 0x04000000

#define iskernelthread(x) IS_KERNEL_ID(x)

#define MAX_MEM (128*1024*1024)



/*
 * decoding Pagefaults
 */

#define WRITE_FAULT(y) (y&0x2)

/*
 * page array
 *
 * lock=1 page is locked for I/O or flushing
 * present=0 page not in memory
 * present=1 page in memory
 * taskid=00 kernel
 * taskid=ff unused
 * unused should be 0
 */

typedef union pa_t
{
  dword_t vmem;
  struct addr
  {
    unsigned taskid:8;
    unsigned present:1;
    unsigned unused:2;
    unsigned lock:1;
    unsigned vmem:20;
  }
  addr;
}
pa_t;


typedef struct partition_entry_t partition_t;
struct partition_entry_t
{
  byte_t boot;			/* Boot flag: 0 = not active, 0x80 active */
  byte_t bhead;			/* Begin: Head number */
  word_t bsecsyl;		/* Begin: Sector and cylinder numb of boot sector */
  byte_t sys;			/* System code: 0x83 linux, 0x82 Linux swap etc. */
  byte_t ehead;			/* End: Head number */
  word_t esecsyl;		/* End: Sector and cylinder number of last sector */
  dword_t start;		/* Relative sector number of start sector */
  dword_t numsec;		/* Number of sectors in the partition */
}
__attribute__ ((packed));

typedef struct partition_header_t
{
  dword_t used_entries;
  dword_t addr_first_block;
  dword_t magic;
}
__attribute ((packed));

#define TOPP_SYS_CODE 0x90

#define HEADER_MAGIC 0x01281977

#define HEADER_LENGTH 2048

#endif
