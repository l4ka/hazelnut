/* 
 * Copyright (c) 1996-1994 The University of Utah and
 * the Computer Systems Laboratory at the University of Utah (CSL).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the
 * Computer Systems Laboratory at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 */
#ifndef _FLUX_MC_STDIO_H
#define _FLUX_MC_STDIO_H

#include <flux/c/common.h>

/* This is a very naive standard I/O implementation
   which simply chains to the low-level I/O routines
   without doing any buffering or anything.  */

#ifndef NULL
#define NULL 0
#endif

typedef struct
{
	int fd;
} FILE;

#define fileno(__stream)	(__stream)->fd

extern FILE __stdin, __stdout, __stderr;
#define stdin	(&__stdin)
#define stdout	(&__stdout)
#define stderr	(&__stderr)

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

#ifndef EOF
#define EOF -1
#endif

__FLUX_BEGIN_DECLS

int putchar(int __c);
int puts(const char *__str);
int printf(const char *__format, ...);
int vprintf(const char *__format, __flux_va_list __vl);
int sprintf(char *__dest, const char *__format, ...);
int snprintf(char *__dest, int __size, const char *__format, ...);
int vsprintf(char *__dest, const char *__format, __flux_va_list __vl);
int vsnprintf(char *__dest, int __size, const char *__format, __flux_va_list __vl);
int getchar(void);
FILE *fopen(const char *__path, const char *__mode);
int fclose(FILE *__stream);
int fread(void *__buf, int __size, int __count, FILE *__stream);
int fwrite(void *buf, int __size, int __count, FILE *__stream);
int fputc(int c, FILE *__stream);
int fgetc(FILE *__stream);
int fprintf(FILE *__stream, const char *__format, ...);
int vfprintf(FILE *__stream, const char *__format, __flux_va_list __vl);
int fscanf(FILE *__stream, const char *__format, ...);
int fseek(FILE *__stream, long __offset, int __whence);
int feof(FILE *__stream);
long ftell(FILE *__stream);
void rewind(FILE *__stream);
int rename(const char *__from, const char *__to);
void hexdump(void *buf, int len);

#define putc(c, stream) fputc(c, stream)

__FLUX_END_DECLS

#endif _FLUX_MC_STDIO_H
