#ifndef __L4IO_H__
#define __L4IO_H__

#include "stdarg.h"

#ifdef __cplusplus
extern "C" {
#endif

int vsnprintf(char *str, int size, const char *fmt, va_list ap);
int printf(const char *fmt, ...);
void putc(int c);
int getc(void);


#ifdef __cplusplus
}
#endif


#endif /* !__L4IO_H__ */
