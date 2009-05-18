######################################################################
##                
## Copyright (C) 2000,  University of Karlsruhe
##                
## Filename:      crt0-x86.S
## Description:   Sets up a small stack and calls main with valid
##                multiboot paremeters.
##                
## This program is free software; you can redistribute it and/or
## modify it under the terms of the GNU General Public License
## as published by the Free Software Foundation; either version 2
## of the License, or (at your option) any later version.
## 
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
## 
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place - Suite 330,
## Boston, MA  02111-1307, USA.
##                
## $Log: crt0-x86.S,v $
## Revision 1.1  2000/09/21 15:59:00  skoglund
## A first version of an ide driver.  The implementation of a recoverable
## disk is not yet finished.
##
##                
######################################################################
	.text
	.global _start
	.global _stext
_stext:	
_start:
	leal	__stack, %esp
	pushl	%eax
	pushl	%ebx
	pushl	$___return_from_main
	jmp	main

	.align 16, 0x90
__mb_header:
	.long   0x1BADB002;
	.long   0x00010000;
	.long   - 0x00010000 - 0x1BADB002;
	.long   __mb_header;
	.long   _start;
	.long   _edata;
	.long   _end;
	.long   _start;
	
___return_from_main:
	int $3
	jmp 1f
	.ascii "System stopped."
1:	jmp ___return_from_main
		
	.bss
	.space	1024
__stack:
