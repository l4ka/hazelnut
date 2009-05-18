/*********************************************************************
 *                
 * Copyright (C) 2000, 2001,  Karlsruhe University
 *                
 * File path:     l4/libide.h
 * Description:   Interface for the IDE driver.
 *                
 * @LICENSE@
 *                
 * $Id: libide.h,v 1.2 2001/12/13 08:18:58 uhlig Exp $
 *                
 ********************************************************************/

#ifndef __L4__LIBIDE_H__
#define __L4__LIBIDE_H__

#ifdef __cplusplus
extern "C" {
#endif


#define L4_IDE_SECTOR_SIZE 512

typedef union l4_idearg_t l4_idearg_t;
union l4_idearg_t {
    struct {
	unsigned pos	:24;
	unsigned length	:5;
	unsigned drive	:2;
	unsigned write	:1;
    } args;

    dword_t raw;
};

int ide_init(void);
int ide_read(dword_t __drive, dword_t __sec, void *__buf, dword_t __length);
int ide_write(dword_t __drive, dword_t __sec, void *__buf, dword_t __length);


#ifdef __cplusplus
}
#endif

#endif /* !__L4__LIBIDE_H__ */
