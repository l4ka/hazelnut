/*********************************************************************
 *                
 * Copyright (C) 2000, 2001,  Karlsruhe University
 *                
 * File path:     tracebuffer.h
 * Description:   Macros recording tracebuffer events.
 *                
 * @LICENSE@
 *                
 * $Id: tracebuffer.h,v 1.2 2001/11/22 14:56:35 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __TRACEBUFFER_H__
#define __TRACEBUFFER_H__

#if defined(CONFIG_TRACEBUFFER)
#include INC_ARCH(tracebuffer.h)
#else
#define TBUF_RECORD_EVENT(_a)
#define TBUF_RECORD_EVENT_AND_PAR(_a, _p0)
#define TBUF_RECORD_EVENT_AND_TWO_PAR(_a, _p0, _p1)
#define TBUF_INCREMENT_TRACE_COUNTER()
#define TBUF_CLEAR_BUFFER()
#endif

#endif /* __TRACEBUFFER_H__ */
