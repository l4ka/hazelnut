/*********************************************************************
 *                
 * Copyright (C) 2000, 2001,  Karlsruhe University
 *                
 * File path:     x86/tracebuffer.h
 * Description:   x86 tracebuffer handling.
 *                
 * @LICENSE@
 *                
 * $Id: tracebuffer.h,v 1.3 2001/11/22 14:56:37 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __X86__TRACEBUFFER_H__
#define __X86__TRACEBUFFER_H__

typedef struct {
                 dword_t cycles;
                 dword_t pmc0;
                 dword_t pmc1;
               } tracestatus_t;  
                 
typedef struct {
                 tracestatus_t status;
                 dword_t identifier;
                 dword_t data[4];
               } trace_t;

typedef struct {
                 dword_t current;
                 dword_t magic;
                 dword_t counter;
                 dword_t threshold;
                 dword_t pad[4];
                 trace_t trace[];
               } tracebuffer_t;  

extern tracebuffer_t *tracebuffer;

#define TBUF_MAGIC 0x20121977

#if defined(CONFIG_PERFMON)
  #define RDPMC "rdpmc\n"
#else
  #define RDPMC "xor %%eax, %%eax\n" 
#endif

#define TBUF_RECORD_EVENT(_a) \
	asm volatile ( \
			"movl	%%fs:0, %%edi	        \n" \
			"rdtsc				\n" \
			"movl 	%%eax, %%fs:32(%%edi)   \n" \
			"xor	%%ecx, %%ecx		\n" \
			RDPMC                               \
			"movl   %%eax, %%fs:36(%%edi)   \n" \
			"inc	%%ecx			\n" \
			RDPMC                               \
			"movl	%%eax, %%fs:40(%%edi)   \n" \
			"movl	%0,    %%fs:44(%%edi)   \n" \
			"addl	$32,   %%edi		\n" \
			"andl   $0x1fffff, %%edi	\n" \
			"movl	%%edi, %%fs:0     	\n" \
			:				    \
			: "g" (_a)			    \
			: "eax", "edx", "edi", "ecx"	    \
		     )

#define TBUF_RECORD_EVENT_AND_PAR(_a, _p0) \
	asm volatile ( \
			"movl	%%fs:0, %%edi	        \n" \
			"rdtsc				\n" \
			"movl 	%%eax, %%fs:32(%%edi)   \n" \
			"xor	%%ecx, %%ecx		\n" \
			RDPMC                               \
			"movl   %%eax, %%fs:36(%%edi)   \n" \
			"inc	%%ecx			\n" \
			RDPMC                               \
			"movl	%%eax, %%fs:40(%%edi)   \n" \
			"movl	%0,    %%fs:44(%%edi)   \n" \
                        "movl   %%ebx, %%fs:48(%%edi)   \n" \
			"addl	$32,   %%edi		\n" \
			"andl   $0x1fffff, %%edi	\n" \
			"movl	%%edi, %%fs:0     	\n" \
			:				    \
			: "g" (_a), "b"(_p0)    	    \
			: "eax", "edx", "edi", "ecx"	    \
		     )

#define TBUF_RECORD_EVENT_AND_TWO_PAR(_a, _p0, _p1) \
	asm volatile ( \
			"movl	%%fs:0, %%edi	        \n" \
			"rdtsc				\n" \
			"movl 	%%eax, %%fs:32(%%edi)   \n" \
			"xor	%%ecx, %%ecx		\n" \
			RDPMC                               \
			"movl   %%eax, %%fs:36(%%edi)   \n" \
			"inc	%%ecx			\n" \
			RDPMC                               \
			"movl	%%eax, %%fs:40(%%edi)   \n" \
			"movl	%0,    %%fs:44(%%edi)   \n" \
                        "movl   %%ebx, %%fs:48(%%edi)   \n" \
                        "movl   %%esi, %%fs:52(%%edi)   \n" \
			"addl	$32,   %%edi		\n" \
			"andl   $0x1fffff, %%edi	\n" \
			"movl	%%edi, %%fs:0     	\n" \
			:				    \
			: "g" (_a), "b"(_p0), "S" (_p1)	    \
			: "eax", "edx", "edi", "ecx"	    \
		     )

#define TBUF_INCREMENT_TRACE_COUNTER() \
	asm volatile ( \
			"movl	%%fs:8, %%edx	        \n" \
			"movl	%%fs:12, %%edi	        \n" \
			"inc	%%edx			\n" \
			"movl	%%edx, %%fs:8   	\n" \
			"cmpl	%%edx, %%edi		\n" \
			"jge	1f			\n" \
			"int3				\n" \
			"xor	%%edx, %%edx		\n" \
			"movl	%%edx, %%fs:8   	\n" \
			"1: 				\n" \
			:				    \
			: 				    \
			: "edx", "edi"		    	    \
		     );	

#define TBUF_CLEAR_BUFFER() \
	asm volatile ( \
			"movl	$0, %fs:0		\n" \
		     );	

#endif /* __X86__TRACEBUFFER_H__ */
