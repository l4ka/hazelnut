/*********************************************************************
 *                
 * Copyright (C) 2000, 2001, 2002,  University of Karlsruhe
 *                
 * File path:     l4/sigma0.h
 * Description:   Interface functions for new sigma0 protocol.
 *                
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 *                
 * $Id: sigma0.h,v 1.4 2002/05/07 19:36:50 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __L4__SIGMA0_H__
#define __L4__SIGMA0_H__


#ifdef __cplusplus
#define l4_sigma0_getpage_rcvpos	l4_sigma0_getpage
#define l4_sigma0_getany_rcvpos		l4_sigma0_getany
#define l4_sigma0_getkerninfo_rcvpos	l4_sigma0_getkerninfo
#endif

/*
 * Prototypes.
 */

L4_INLINE dword_t l4_sigma0_getpage(l4_threadid_t s0, l4_fpage_t fp);
L4_INLINE dword_t l4_sigma0_getpage_rcvpos(l4_threadid_t s0, l4_fpage_t fp,
					   l4_fpage_t rcv);
L4_INLINE dword_t l4_sigma0_getany(l4_threadid_t s0, dword_t size);
L4_INLINE dword_t l4_sigma0_getany_rcvpos(l4_threadid_t s0, dword_t size,
					  l4_fpage_t rcv);
L4_INLINE dword_t l4_sigma0_getkerninfo(l4_threadid_t s0);
L4_INLINE dword_t l4_sigma0_getkerninfo_rcvpos(l4_threadid_t s0,
					       l4_fpage_t rcv);
L4_INLINE int l4_sigma0_freepage(l4_threadid_t s0, l4_fpage_t fp);



/*
 * Implementation.
 */

L4_INLINE dword_t l4_sigma0_getpage(l4_threadid_t s0, l4_fpage_t fp)
{
    l4_msgdope_t result;
    dword_t dummy, addr;

    if ( s0.raw == 0 ) s0 = L4_SIGMA0_ID;

    l4_ipc_call(s0,
		NULL,
		0x3, 0, fp.raw | 0x3,
		(void *) l4_fpage(0, L4_WHOLE_ADDRESS_SPACE, 1, 0).raw,
		&addr, &dummy, &dummy,
		L4_IPC_NEVER, &result);

    if ( L4_IPC_ERROR(result) )
	return 0;

    return addr;
}

L4_INLINE dword_t l4_sigma0_getpage_rcvpos(l4_threadid_t s0, l4_fpage_t fp,
					   l4_fpage_t rcv)
{
    l4_msgdope_t result;
    dword_t dummy, addr;

    if ( s0.raw == 0 ) s0 = L4_SIGMA0_ID;

    l4_ipc_call(s0,
		NULL,
		0x3, 0, fp.raw | 0x3,
		(void *) rcv.raw,
		&addr, &dummy, &dummy,
		L4_IPC_NEVER, &result);

    if ( L4_IPC_ERROR(result) )
	return 0;

    return addr;
}


L4_INLINE dword_t l4_sigma0_getany(l4_threadid_t s0, dword_t size)
{
    l4_msgdope_t result;
    dword_t dummy, addr;

    if ( s0.raw == 0 ) s0 = L4_SIGMA0_ID;

    l4_ipc_call(s0,
		NULL,
		0x3, 0, l4_fpage(0, size, 0, 0).raw | 0x1,
		(void *) l4_fpage(0, L4_WHOLE_ADDRESS_SPACE, 1, 0).raw,
		&addr, &dummy, &dummy,
		L4_IPC_NEVER, &result);

    if ( L4_IPC_ERROR(result) )
	return 0;

    return addr;
}

L4_INLINE dword_t l4_sigma0_getany_rcvpos(l4_threadid_t s0, dword_t size,
					  l4_fpage_t rcv)
{
    l4_msgdope_t result;
    dword_t dummy, addr;

    if ( s0.raw == 0 ) s0 = L4_SIGMA0_ID;

    l4_ipc_call(s0,
		NULL,
		0x3, 0, l4_fpage(0, size, 0, 0).raw | 0x1,
		(void *) rcv.raw,
		&addr, &dummy, &dummy,
		L4_IPC_NEVER, &result);

    if ( L4_IPC_ERROR(result) )
	return 0;

    return addr;
}


L4_INLINE dword_t l4_sigma0_getkerninfo(l4_threadid_t s0)
{
    l4_msgdope_t result;
    dword_t dummy, addr;

    if ( s0.raw == 0 ) s0 = L4_SIGMA0_ID;

    l4_ipc_call(s0, NULL,
		0x1, 0, 0,
		(void *) l4_fpage(0, L4_WHOLE_ADDRESS_SPACE, 1, 0).raw,
		&addr, &dummy, &dummy,
		L4_IPC_NEVER, &result);

    if ( L4_IPC_ERROR(result) )
	return 0;

    return addr;
}

L4_INLINE dword_t l4_sigma0_getkerninfo_rcvpos(l4_threadid_t s0,
					       l4_fpage_t rcv)
{
    l4_msgdope_t result;
    dword_t dummy, addr;

    if ( s0.raw == 0 ) s0 = L4_SIGMA0_ID;

    l4_ipc_call(s0, NULL,
		0x1, 0, 0,
		(void *) (rcv.raw | 0x2),
		&addr, &dummy, &dummy,
		L4_IPC_NEVER, &result);

    if ( L4_IPC_ERROR(result) )
	return 0;

    return addr;
}

L4_INLINE int l4_sigma0_freepage(l4_threadid_t s0, l4_fpage_t fp)
{
    l4_msgdope_t result;

    if ( s0.raw == 0 ) s0 = L4_SIGMA0_ID;

    l4_ipc_send(s0,
		NULL,
		0, 0, (fp.raw & ~0x3) | 0x2,
		L4_IPC_NEVER, &result);

    if ( L4_IPC_ERROR(result) )
	return -1;

    return 0;
}

#endif /* !__L4__SIGMA0_H__ */
