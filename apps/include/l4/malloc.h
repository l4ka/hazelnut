/*********************************************************************
 *                
 * Copyright (C) 2000,  University of Karlsruhe
 *                
 * Filename:      malloc.h
 * Description:   Interface for malloc(3) and friends.
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
 * $Log: malloc.h,v $
 * Revision 1.1  2000/09/26 10:14:06  skoglund
 * Interface defs for malloc(3) and friends.
 *
 *                
 ********************************************************************/
#ifndef __L4__MALLOC_H__
#define __L4__MALLOC_H__

#ifdef __cplusplus
extern "C" {
#endif


void *malloc(int __size);
void free(void *__ptr);


#ifdef __cplusplus
}
#endif

#endif /* !__L4__MALLOC_H__ */
