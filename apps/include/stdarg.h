#ifndef L4__STDARG_H
#define L4__STDARG_H

#ifndef va_start

typedef unsigned long * va_list;


/*
 * Update ap to point to the parameter that follows lastarg.
 */
#define va_start(ap,larg) ap = (va_list) ((char *) &larg + sizeof(larg))


/* 
 * The following macro gets the size of ptype and rounds it up to the
 * nearest number modulo int (typically 4).  This is done because
 * arguments passed to functions will not occupy less space on the
 * stack than 4 bytes (the size of an int).
 */
#define __va_size_rounded(ptype) \
  ( ((sizeof(ptype) + sizeof(int) - 1) / sizeof(int)) * sizeof(int))


/*
 * First advance the parameter pointer to the next parameter on stack.
 * Then return the value of the parameter that we have poped.
 */
#define va_arg(ap,ptype)						\
({									\
    ap = (va_list) ((char *) (ap) + __va_size_rounded(ptype));		\
    *( (ptype *) (void *) ((char *) (ap) - __va_size_rounded(ptype)) );	\
})


/*
 * va_end should do nothing.
 */
#define va_end(ap)


#endif /* !va_start */

#endif /* !L4__STDARG_H */
