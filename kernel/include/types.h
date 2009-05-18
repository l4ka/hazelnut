/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     types.h
 * Description:   Global type definitions.
 *                
 * @LICENSE@
 *                
 * $Id: types.h,v 1.9 2001/11/22 14:56:35 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __TYPES_H__
#define __TYPES_H__

/*
 * standard type definitions
 */
typedef unsigned long long	qword_t;
typedef unsigned int		dword_t;
typedef unsigned short		word_t;
typedef unsigned char		byte_t;

typedef signed long long	sqword_t;
typedef signed int		sdword_t;
typedef signed short		sword_t;
typedef signed char		sbyte_t;

typedef dword_t*                ptr_t;

typedef	unsigned long long	uint64;
typedef	unsigned long		uint32;
typedef	unsigned short		uint16;
typedef	unsigned char		uint8;
typedef	long long		int64;
typedef	long			int32;
typedef	short			int16;
typedef	char			int8;



typedef struct tcb_t		tcb_t;


#ifndef NULL
#define NULL			(0)
#endif

#ifndef FALSE
#define FALSE	0
#endif

#ifndef TRUE
#define TRUE	(!FALSE)
#endif

#endif /* __TYPES_H__ */
